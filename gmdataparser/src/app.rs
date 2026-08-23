use std::collections::HashMap;
use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::sync::mpsc;
use std::thread;
use std::time::Duration;

use crate::audio::AudioPlayer;
use crate::bindings::*;
use crate::models::{
    build_chunk_info_list, matches_item_query, ChunkInfo, ChunkItem,
};
use crate::texture::{
    texture_page_item_preview_image, texture_preview_image,
};
use eframe::egui;

unsafe extern "C" {
    fn parse_form_chunks(dw: *mut DataWin) -> c_int;
}

#[derive(Clone, Debug)]
pub(crate) struct LoadingProgress {
    message: String,
    value: f32,
    total_chunks: usize,
    parsed_chunks: usize,
}

#[derive(Clone, Debug)]
pub(crate) enum LoadProgressEvent {
    Update { total: usize, parsed: usize, chunk_name: String },
    Complete,
}

pub(crate) struct ThreadSafeDataWin(DataWin);

unsafe impl Send for ThreadSafeDataWin {}
unsafe impl Sync for ThreadSafeDataWin {}

pub(crate) enum LoadResult {
    Loaded {
        version: String,
        dw: ThreadSafeDataWin,
        chunks: Vec<ChunkInfo>,
    },
    Failed(String),
}

fn chunk_bytes_for_view(dw: &DataWin, offset: u32, length: u32) -> &[u8] {
    if dw.file_data.is_null() || dw.file_size == 0 {
        return &[];
    }

    let start = offset as usize;
    if start >= dw.file_size {
        return &[];
    }

    let remaining = dw.file_size.saturating_sub(start);
    let chunk_len = length as usize;
    let view_len = remaining.min(chunk_len);

    unsafe { std::slice::from_raw_parts(dw.file_data.add(start), view_len) }
}

fn format_hex_dump(bytes: &[u8], start_offset: usize) -> String {
    const BYTES_PER_LINE: usize = 16;

    let mut output = String::new();
    for (line_index, line) in bytes.chunks(BYTES_PER_LINE).enumerate() {
        let offset = start_offset + line_index * BYTES_PER_LINE;
        let mut hex_bytes = String::new();
        let mut ascii = String::new();

        for (i, &byte) in line.iter().enumerate() {
            if i > 0 {
                hex_bytes.push(' ');
            }
            hex_bytes.push_str(&format!("{:02x}", byte));
            ascii.push(if byte.is_ascii_graphic() || byte == b' ' {
                byte as char
            } else {
                '.'
            });
        }

        if line.len() < BYTES_PER_LINE {
            let padding = (BYTES_PER_LINE - line.len()) * 3;
            hex_bytes.push_str(&" ".repeat(padding));
            ascii.push_str(&" ".repeat(BYTES_PER_LINE - line.len()));
        }

        output.push_str(&format!("{:08x}: {:<47} |{:<16}|\n", offset, hex_bytes, ascii));
    }

    if output.is_empty() {
        output.push_str("00000000: <empty chunk>\n");
    }

    output.trim_end().to_string()
}

pub struct App {
    pub version: String,
    pub file_path: String,
    pub chunks: Vec<ChunkInfo>,
    pub active_chunk: usize,
    pub dw: DataWin,
    pub texture_preview_cache: HashMap<String, Option<egui::ColorImage>>,
    pub texture_popup: Option<(String, usize)>,
    pub texture_popup_zoom: HashMap<(String, usize), f32>,
    pub audio_player: AudioPlayer,
    pub load_rx: Option<mpsc::Receiver<LoadResult>>,
    pub progress_rx: Option<mpsc::Receiver<LoadProgressEvent>>,
    pub loading: Option<LoadingProgress>,
    pub load_error: Option<String>,
    pub load_complete: bool,
}

fn load_data_win(
    path: &str,
    progress_tx: mpsc::Sender<LoadProgressEvent>,
) -> Result<(String, DataWin, Vec<ChunkInfo>), String> {
    let path_cstr = CString::new(path).map_err(|_| "File path contains a NUL byte".to_string())?;
    let mut dw: DataWin = unsafe { std::mem::zeroed() };

    unsafe {
        println!("Loading file: {:?}", path_cstr);

        if DataWin_loadFile(&mut dw, path_cstr.as_ptr()) != 0 {
            return Err("Failed to load file".to_string());
        }

        if parse_form_chunks(&mut dw) != 0 {
            return Err("Failed to enumerate file chunks".to_string());
        }

        let total = dw.chunks.count as usize;
        let callback_tx = progress_tx.clone();
        let callback_state = Box::into_raw(Box::new(callback_tx));

        let options = DataWinParserOptions {
            parseGen8: true,
            parseOptn: true,
            parseLang: true,
            parseExtn: true,
            parseSond: true,
            parseAgrp: true,
            parseSprt: true,
            parseBgnd: true,
            parsePath: true,
            parseScpt: true,
            parseGlob: true,
            parseShdr: true,
            parseFont: true,
            parseTmln: true,
            parseObjt: true,
            parseRoom: true,
            parseTpag: true,
            parseCode: true,
            parseVari: true,
            parseFunc: true,
            parseStrg: true,
            parseTgin: true,
            parseTxtr: true,
            parseAudo: true,
            parseAcrv: true,
            parseFeds: true,
            parseDafl: true,
            parseEmbi: true,
            parsePsys: true,
            parseFeat: true,
            parseGmen: true,
            parsePsem: true,
            parseSeqn: true,
            parseTags: true,
            parseUilr: true,
            parseStat: true,
            skipLoadingPreciseMasksForNonPreciseSprites: false,
            lazyLoadRooms: false,
            lazyLoadTextures: false,
            lazyLoadAudio: false,
            eagerlyLoadedRooms: std::ptr::null_mut(),
            loadType: DataWinLoadType_DATAWINLOADTYPE_LOAD_PER_CHUNK,
            progressCallback: Some(parse_progress_callback),
            progressCallbackUserData: callback_state as *mut c_void,
        };

        if DataWin_parseWithOptions(&mut dw, &options) != 0 {
            let _ = Box::from_raw(callback_state);
            return Err("Failed to parse file".to_string());
        }

        let _ = Box::from_raw(callback_state);
        let _ = progress_tx.send(LoadProgressEvent::Complete);

        println!("Detected {} chunks in file", total);
    }

    let version = format!(
        "{}.{}.{}.{}",
        dw.detectedFormat.major,
        dw.detectedFormat.minor,
        dw.detectedFormat.release,
        dw.detectedFormat.build
    );

    println!("Detected version: {}", version);

    let chunks = build_chunk_info_list(&dw);
    Ok((version, dw, chunks))
}

unsafe extern "C" fn parse_progress_callback(
    chunk_name: *const c_char,
    chunk_index: c_int,
    total_chunks: c_int,
    _dw: *mut DataWin,
    user_data: *mut c_void,
) {
    unsafe {
        let tx = &*(user_data as *const mpsc::Sender<LoadProgressEvent>);
        let chunk_name = CStr::from_ptr(chunk_name).to_string_lossy().to_string();
        let parsed = chunk_index as usize + 1;
        let total = total_chunks as usize;
        let _ = tx.send(LoadProgressEvent::Update {
            total,
            parsed,
            chunk_name,
        });
    }
}

fn embi_texture_page_index(dw: &DataWin, active_item_idx: usize) -> Option<usize> {
    let embi_entry_id = {
        let embi_items = if dw.embi.count == 0 || dw.embi.items.is_null() {
            return None;
        } else {
            unsafe { std::slice::from_raw_parts(dw.embi.items, dw.embi.count as usize) }
        };

        embi_items.get(active_item_idx)?.texture_page_entry_id
    } as usize;

    if dw.file_data.is_null() || dw.chunks.items.is_null() || dw.chunks.count == 0 {
        return None;
    }

    let chunk_table = unsafe { std::slice::from_raw_parts(dw.chunks.items, dw.chunks.count) };
    let tpag_chunk = chunk_table.iter().find(|chunk| {
        chunk.name[0] == b'T' as i8
            && chunk.name[1] == b'P' as i8
            && chunk.name[2] == b'A' as i8
            && chunk.name[3] == b'G' as i8
    })?;

    let payload_offset = tpag_chunk.offset as usize;
    let payload_len = tpag_chunk.length as usize;
    if payload_len < 4 {
        return None;
    }

    let payload = unsafe { std::slice::from_raw_parts(dw.file_data.add(payload_offset), payload_len) };
    let item_count = u32::from_le_bytes([payload[0], payload[1], payload[2], payload[3]]) as usize;

    for i in 0..item_count.min((payload_len.saturating_sub(4)) / 4) {
        let base = 4 + i * 4;
        let pointer = u32::from_le_bytes([
            payload[base],
            payload.get(base + 1).copied().unwrap_or(0),
            payload.get(base + 2).copied().unwrap_or(0),
            payload.get(base + 3).copied().unwrap_or(0),
        ]) as usize;

        if pointer == embi_entry_id {
            return Some(i);
        }
    }

    None
}

impl App {
    pub fn new(file_path: String) -> Self {
        let (result_tx, result_rx) = mpsc::channel();
        let (progress_tx, progress_rx) = mpsc::channel();
        let worker_path = file_path.clone();

        thread::spawn(move || {
            let result = match load_data_win(&worker_path, progress_tx) {
                Ok((version, dw, chunks)) => {
                    LoadResult::Loaded {
                        version,
                        dw: ThreadSafeDataWin(dw),
                        chunks,
                    }
                }
                Err(err) => LoadResult::Failed(err),
            };

            if result_tx.send(result).is_err() {
                eprintln!("Load result receiver dropped before completion");
            }
        });

        Self {
            version: String::new(),
            file_path,
            chunks: Vec::new(),
            active_chunk: 0,
            dw: unsafe { std::mem::zeroed() },
            texture_preview_cache: HashMap::new(),
            texture_popup: None,
            texture_popup_zoom: HashMap::new(),
            audio_player: AudioPlayer::new(),
            load_rx: Some(result_rx),
            progress_rx: Some(progress_rx),
            loading: Some(LoadingProgress {
                message: "Loading file...".to_string(),
                value: 0.0,
                total_chunks: 0,
                parsed_chunks: 0,
            }),
            load_error: None,
            load_complete: false,
        }
    }

    fn poll_load_result(&mut self) {
        if let Some(progress_rx) = self.progress_rx.as_ref() {
            while let Ok(event) = progress_rx.try_recv() {
                match event {
                    LoadProgressEvent::Update { total, parsed, chunk_name } => {
                        if let Some(loading) = self.loading.as_mut() {
                            loading.total_chunks = total;
                            loading.parsed_chunks = parsed;
                            loading.message = format!("Parsing {} ({}/{})", chunk_name, parsed, total);
                            loading.value = if total == 0 {
                                0.0
                            } else {
                                (parsed as f32 / total as f32).min(1.0)
                            };
                        }
                    }
                    LoadProgressEvent::Complete => {
                        if let Some(loading) = self.loading.as_mut() {
                            loading.message = "Finalizing...".to_string();
                            loading.value = 1.0;
                        }
                    }
                }
            }
        }

        let Some(rx) = self.load_rx.as_ref() else {
            return;
        };

        match rx.try_recv() {
            Ok(LoadResult::Loaded { version, dw, chunks }) => {
                self.version = version;
                self.dw = dw.0;
                self.chunks = chunks;
                self.active_chunk = 0;
                self.load_complete = true;
                self.loading = None;
                self.load_rx = None;
                self.progress_rx = None;
            }
            Ok(LoadResult::Failed(err)) => {
                self.load_error = Some(err);
                self.load_complete = true;
                self.loading = None;
                self.load_rx = None;
                self.progress_rx = None;
            }
            Err(mpsc::TryRecvError::Empty) => {}
            Err(mpsc::TryRecvError::Disconnected) => {
                self.load_error = Some("Failed to load file data".to_string());
                self.load_complete = true;
                self.loading = None;
                self.load_rx = None;
                self.progress_rx = None;
            }
        }
    }

    fn render_loading_ui(&mut self, ui: &mut egui::Ui) {
        let progress = self
            .loading
            .as_ref()
            .map(|state| state.value)
            .unwrap_or(0.0);
        let message = self
            .loading
            .as_ref()
            .map(|state| state.message.as_str())
            .unwrap_or("Loading...");

        ui.ctx().request_repaint_after(Duration::from_millis(60));

        egui::Frame::default()
            .inner_margin(egui::Margin::same(20))
            .show(ui, |ui| {
                ui.centered_and_justified(|ui| {
                    ui.vertical_centered(|ui| {
                        ui.heading("Loading data.win");
                        ui.add_space(12.0);
                        ui.label(format!("File: {}", self.file_path));
                        if let Some(state) = self.loading.as_ref() {
                            if state.total_chunks > 0 {
                                ui.label(format!(
                                    "Chunks: {}/{} parsed",
                                    state.parsed_chunks, state.total_chunks
                                ));
                            }
                        }
                        ui.add_space(18.0);
                        ui.add(egui::ProgressBar::new(progress).text(message));
                    });
                });
            });
    }

    fn texture_preview_size(&self, chunk_name: &str, active_item_idx: usize) -> Option<egui::Vec2> {
        if chunk_name != "TXTR" && chunk_name != "TPAG" && chunk_name != "EMBI" {
            return None;
        }

        let texture_slice = if self.dw.txtr.textures.is_null() || self.dw.txtr.count == 0 {
            &[][..]
        } else {
            unsafe {
                std::slice::from_raw_parts(
                    self.dw.txtr.textures,
                    self.dw.txtr.count as usize,
                )
            }
        };

        if chunk_name == "TXTR" {
            texture_slice.get(active_item_idx).map(|texture| {
                egui::vec2(
                    texture.textureWidth.max(1) as f32,
                    texture.textureHeight.max(1) as f32,
                )
            })
        } else {
            let page_data = if self.dw.tpag.items.is_null() || self.dw.tpag.count == 0 {
                &[][..]
            } else {
                unsafe {
                    std::slice::from_raw_parts(
                        self.dw.tpag.items,
                        self.dw.tpag.count as usize,
                    )
                }
            };

            let page_index = if chunk_name == "EMBI" {
                embi_texture_page_index(&self.dw, active_item_idx)?
            } else {
                active_item_idx
            };

            page_data.get(page_index).map(|page_item| {
                egui::vec2(
                    page_item.sourceWidth.max(1) as f32,
                    page_item.sourceHeight.max(1) as f32,
                )
            })
        }
    }

    fn render_audio_player_ui(&mut self, ui: &mut egui::Ui, active_item_idx: usize) {
        ui.separator();
        ui.horizontal(|ui| {
            if ui.button("Play").clicked() {
                if let Err(err) = self.audio_player.load_from_dw(&self.dw, active_item_idx) {
                    ui.ctx().data_mut(|d| {
                        d.insert_temp(
                            egui::Id::new("audio_error"),
                            err,
                        );
                    });
                } else if let Err(err) = self.audio_player.play() {
                    ui.ctx().data_mut(|d| {
                        d.insert_temp(
                            egui::Id::new("audio_error"),
                            err,
                        );
                    });
                }
            }

            if ui.button("Stop").clicked() {
                self.audio_player.stop();
            }
        });

        let status = if self.audio_player.is_playing() {
            "Playing"
        } else {
            "Ready"
        };
        ui.label(format!("Status: {status}"));
        ui.label(format!("Loaded bytes: {}", self.audio_player.bytes_len()));

        if let Some(err) = ui.ctx().data_mut(|d| d.get_temp::<String>(egui::Id::new("audio_error"))) {
            ui.colored_label(egui::Color32::LIGHT_RED, err);
        }
    }

    fn render_texture_preview(
        &mut self,
        ui: &mut egui::Ui,
        chunk_name: &str,
        active_item_idx: usize,
        popup_zoom: Option<f32>,
    ) {
        if chunk_name != "TXTR" && chunk_name != "TPAG" && chunk_name != "EMBI" {
            return;
        }

        let preview_key = format!("{}-{}", chunk_name, active_item_idx);

        let texture_slice = if self.dw.txtr.textures.is_null() || self.dw.txtr.count == 0 {
            &[][..]
        } else {
            unsafe {
                std::slice::from_raw_parts(
                    self.dw.txtr.textures,
                    self.dw.txtr.count as usize,
                )
            }
        };

        let gm2022_5 = self.dw.detectedFormat.major >= 2022 && self.dw.detectedFormat.minor >= 5;

        let preview_image = if chunk_name == "TXTR" {
            if let Some(texture) = texture_slice.get(active_item_idx) {
                self.texture_preview_cache
                    .entry(preview_key.clone())
                    .or_insert_with(|| texture_preview_image(texture, gm2022_5))
                    .clone()
            } else {
                None
            }
        } else {
            let page_data = if self.dw.tpag.items.is_null() || self.dw.tpag.count == 0 {
                &[][..]
            } else {
                unsafe {
                    std::slice::from_raw_parts(
                        self.dw.tpag.items,
                        self.dw.tpag.count as usize,
                    )
                }
            };

            let page_index = if chunk_name == "EMBI" {
                embi_texture_page_index(&self.dw, active_item_idx)
            } else {
                Some(active_item_idx)
            };

            page_index.and_then(|page_index| {
                page_data.get(page_index).and_then(|page_item| {
                    self.texture_preview_cache
                        .entry(preview_key.clone())
                        .or_insert_with(|| texture_page_item_preview_image(page_item, texture_slice, gm2022_5))
                        .clone()
                })
            })
        };

        let Some(image) = preview_image else {
            ui.label("No preview available for this item.");
            return;
        };

        let handle = ui.ctx().load_texture(
            format!("{}-preview-{}", chunk_name, active_item_idx),
            image,
            egui::TextureOptions::LINEAR,
        );

        let Some(original_size) = self.texture_preview_size(chunk_name, active_item_idx) else {
            return;
        };

        let zoom_scale = popup_zoom.unwrap_or(1.0);
        let display_size = if popup_zoom.is_some() {
            let max_width = ui.available_width().max(1.0);
            let max_height = ui.available_height().max(1.0);
            let fit_scale = (max_width / original_size.x)
                .min(max_height / original_size.y)
                .min(1.0);
            original_size * fit_scale * zoom_scale
        } else {
            let available_width = ui.available_width();
            let scale = if original_size.x > available_width {
                available_width / original_size.x
            } else {
                1.0
            };
            original_size * scale
        };

        let response = ui.add_sized(
            display_size,
            egui::Image::new(&handle)
                .fit_to_exact_size(display_size)
                .sense(egui::Sense::click()),
        );

        if response.clicked() {
            self.texture_popup = Some((chunk_name.to_string(), active_item_idx));
        }
    }
}

impl Drop for App {
    fn drop(&mut self) {
        if self.load_complete {
            unsafe {
                let _ = DataWin_free(&mut self.dw);
            }
        }
    }
}

impl eframe::App for App {
    fn ui(&mut self, ui: &mut egui::Ui, _frame: &mut eframe::Frame) {
        self.poll_load_result();

        if let Some(err) = self.load_error.clone() {
            ui.centered_and_justified(|ui| {
                ui.vertical_centered(|ui| {
                    ui.heading("Unable to load file");
                    ui.add_space(12.0);
                    ui.label(err);
                });
            });
            return;
        }

        if !self.load_complete {
            self.render_loading_ui(ui);
            return;
        }

        ui.ctx().set_visuals(egui::Visuals::dark());
        ui.ctx().set_theme(egui::ThemePreference::Dark);

        if let Some((chunk_name, item_idx)) = self.texture_popup.clone() {
            let mut open = true;
            let zoom_key = (chunk_name.clone(), item_idx);
            let current_zoom = self.texture_popup_zoom.get(&zoom_key).copied().unwrap_or(1.0);
            let mut zoom = current_zoom;

            egui::Window::new(format!("{} preview", chunk_name))
                .default_size([420.0, 420.0])
                .open(&mut open)
                .show(ui.ctx(), |ui| {
                    let pinch_delta = ui.ctx().input(|i| i.zoom_delta());
                    if pinch_delta != 1.0 {
                        zoom = (zoom * pinch_delta).clamp(0.25, 8.0);
                        self.texture_popup_zoom.insert(zoom_key.clone(), zoom);
                    }

                    let scroll_area = egui::ScrollArea::both().auto_shrink([false, false]);

                    let base_size = self
                        .texture_preview_size(&chunk_name, item_idx)
                        .unwrap_or(egui::vec2(1.0, 1.0));
                    let zoomed_size = base_size * zoom;

                    scroll_area.show(ui, |ui| {
                        ui.allocate_ui(zoomed_size, |ui| {
                            self.render_texture_preview(ui, &chunk_name, item_idx, Some(zoom));
                        });
                    });
                });

            if !open {
                self.texture_popup = None;
                self.texture_popup_zoom.remove(&zoom_key);
            }
        }

        egui::Frame::default()
            .inner_margin(egui::Margin::same(20))
            .show(ui, |ui| {
                ui.heading("gmdataparser");
                ui.separator();
                ui.label(format!("File: {}", self.file_path));
                ui.label(format!("GameMaker version: {}", self.version));

                if self.chunks.is_empty() {
                    ui.separator();
                    ui.label("No chunks were found in this file.");
                    return;
                }

                ui.separator();
                ui.horizontal_wrapped(|ui| {
                    for idx in 0..self.chunks.len() {
                        let chunk_name = match self.chunks[idx].name.as_str() {
                            "GEN8" => "Info",
                            "OPTN" => "Options",
                            "LANG" => "Languages",
                            "EXTN" => "Extensions",
                            "SOND" => "Sounds",
                            "AGRP" => "Audio Groups",
                            "SPRT" => "Sprites",
                            "BGND" => "Backgrounds",
                            "PATH" => "Paths",
                            "SCPT" => "Scripts",
                            "GLOB" => "Global Code IDs",
                            "GMEN" => "Game End",
                            "CODE" => "Code",
                            "VARI" => "Variables",
                            "FUNC" => "Functions",
                            "SHDR" => "Shaders",
                            "FONT" => "Fonts",
                            "TMLN" => "Timelines",
                            "TGIN" => "Texture Groups",
                            "TPAG" => "Texture Pages",
                            "OBJT" => "Objects",
                            "ROOM" => "Rooms",
                            "ACRV" => "Animation Curves",
                            "FEDS" => "Filter Effects",
                            "FEAT" => "Feature Flags",
                            "SEQN" => "Sequences",
                            "TAGS" => "Tags",
                            "STRG" => "Strings",
                            "UILR" => "UI Layers",
                            "DAFL" => "Data Files",
                            "EMBI" => "Embedded Images",
                            "PSEM" => "Particle Emitters",
                            "PSYS" => "Particle Systems",
                            "TXTR" => "Textures",
                            "AUDO" => "Audio",
                            "STAT" => "Event Manifests",
                            name => name,
                        };

                        let selected = self.active_chunk == idx;
                        if ui.selectable_label(selected, chunk_name).clicked() {
                            self.active_chunk = idx;
                            if let Some(chunk) = self.chunks.get_mut(idx) {
                                chunk.active_item = 0;
                            }
                        }
                    }
                });

                ui.separator();

                let active_chunk_idx = self.active_chunk;
                let active_chunk_name = self.chunks[active_chunk_idx].name.clone();
                let active_chunk_offset = self.chunks[active_chunk_idx].offset;
                let active_chunk_length = self.chunks[active_chunk_idx].length;
                let active_chunk_fields = self.chunks[active_chunk_idx].fields.clone();
                let active_chunk_items = self.chunks[active_chunk_idx].items.clone();

                if active_chunk_items.is_empty() {
                    ui.group(|ui| {
                        ui.label(format!("Chunk: {}", active_chunk_name));
                        ui.label(format!("Offset: {}", active_chunk_offset));
                        ui.label(format!("Length: {}", active_chunk_length));
                        ui.separator();

                        if active_chunk_fields.is_empty() {
                            let chunk_bytes = chunk_bytes_for_view(
                                &self.dw,
                                active_chunk_offset,
                                active_chunk_length,
                            );

                            ui.label("No structured fields are available for this chunk.");
                            ui.add_space(6.0);
                            ui.label("Raw bytes:");
                            ui.add_space(6.0);

                            egui::ScrollArea::both()
                                .max_height(320.0)
                                .show(ui, |ui| {
                                    ui.monospace(format_hex_dump(chunk_bytes, active_chunk_offset as usize));
                                });
                            return;
                        }

                        egui::ScrollArea::vertical().max_height(260.0).show(ui, |ui| {
                            egui::Grid::new(format!("chunk-fields-{}", active_chunk_name))
                                .num_columns(2)
                                .spacing([12.0, 4.0])
                                .show(ui, |ui| {
                                    for field in &active_chunk_fields {
                                        ui.label(&field.name);
                                        ui.label(&field.value);
                                        ui.end_row();
                                    }
                                });
                        });
                    });
                    return;
                }

                let active_item_idx = self.chunks[active_chunk_idx].active_item;
                let active_item = active_chunk_items
                    .get(active_item_idx)
                    .cloned()
                    .unwrap_or_else(|| {
                        active_chunk_items.first().cloned().unwrap_or_else(|| ChunkItem {
                            name: "Unknown item".to_string(),
                            fields: Vec::new(),
                        })
                    });

                ui.columns(2, |columns| {
                    columns[0].set_min_width(180.0);
                    columns[0].vertical(|ui| {
                        ui.heading("Items");
                        ui.separator();

                        let mut filter_text = self.chunks[active_chunk_idx].item_filter.clone();
                        ui.horizontal(|ui| {
                            ui.label("Search");
                            ui.add_sized([150.0, 0.0], egui::TextEdit::singleline(&mut filter_text));
                        });
                        if filter_text != self.chunks[active_chunk_idx].item_filter {
                            self.chunks[active_chunk_idx].item_filter = filter_text;
                            self.chunks[active_chunk_idx].visible_item_count = 0;
                        }

                        let query = self.chunks[active_chunk_idx].item_filter.trim().to_ascii_lowercase();
                        let filtered_indices: Vec<usize> = self.chunks[active_chunk_idx]
                            .items
                            .iter()
                            .enumerate()
                            .filter_map(|(idx, item)| {
                                if matches_item_query(item, &query) {
                                    Some(idx)
                                } else {
                                    None
                                }
                            })
                            .collect();

                        if filtered_indices.is_empty() {
                            ui.label("No matching items.");
                            return;
                        }

                        let item_height = 24.0;
                        let window_size = 100usize;
                        let step_size = 25usize;

                        egui::ScrollArea::vertical().max_height(340.0).show_viewport(ui, |ui, viewport| {
                            let active_chunk = &mut self.chunks[active_chunk_idx];
                            let total_height = filtered_indices.len() as f32 * item_height;
                            ui.set_min_height(total_height);

                            let scroll_index = (viewport.min.y / item_height).floor() as usize;
                            let mut window_start = (scroll_index / step_size) * step_size;
                            window_start = window_start.min(filtered_indices.len().saturating_sub(window_size));
                            active_chunk.visible_item_count = window_start;

                            let render_start = window_start;
                            let render_end = (render_start + window_size).min(filtered_indices.len());

                            ui.add_space(render_start as f32 * item_height);
                            for &idx in &filtered_indices[render_start..render_end] {
                                let item = &active_chunk.items[idx];
                                let selected = active_chunk.active_item == idx;
                                if ui.selectable_label(selected, item.name.clone()).clicked() {
                                    active_chunk.active_item = idx;
                                }
                            }
                            ui.add_space((filtered_indices.len() - render_end) as f32 * item_height);

                            if render_end < filtered_indices.len() {
                                ui.label("Scroll for more...");
                            }
                        });
                    });

                    columns[1].vertical(|ui| {
                        egui::ScrollArea::vertical()
                            .id_salt("chunk-item-scroll")
                            .auto_shrink([false, false])
                            .show(ui, |ui| {
                                ui.group(|ui| {
                                    ui.label(format!("Chunk: {}", active_chunk_name));
                                    ui.label(format!("Offset: {}", active_chunk_offset));
                                    ui.label(format!("Length: {}", active_chunk_length));
                                    ui.label(format!("Item: {}", active_item.name));
                                    ui.separator();

                                    egui::Grid::new(format!(
                                        "chunk-item-fields-{}-{}",
                                        active_chunk_name,
                                        active_item.name
                                    ))
                                    .num_columns(2)
                                    .spacing([12.0, 4.0])
                                    .show(ui, |ui| {
                                        for field in &active_item.fields {
                                            ui.label(&field.name);
                                            ui.label(&field.value);
                                            ui.end_row();
                                        }
                                    });

                                    if active_chunk_name == "TXTR" || active_chunk_name == "TPAG" || active_chunk_name == "EMBI" {
                                        ui.separator();
                                        self.render_texture_preview(ui, &active_chunk_name, active_item_idx, None);
                                    }

                                    if active_chunk_name == "AUDO" {
                                        self.render_audio_player_ui(ui, active_item_idx);
                                    }
                                });
                            });
                    });
                });
            });
    }
}

pub fn create_app(file_path: String) -> App {
    App::new(file_path)
}
