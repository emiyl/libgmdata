mod bindings;

use std::ffi::{CStr, CString, c_char};
use bindings::*;
use eframe::egui;

#[derive(Clone, Debug)]
struct ChunkField {
    name: String,
    value: String,
}

#[derive(Clone, Debug)]
struct ChunkItem {
    name: String,
    fields: Vec<ChunkField>,
}

#[derive(Clone, Debug)]
struct ChunkInfo {
    name: String,
    offset: u32,
    length: u32,
    fields: Vec<ChunkField>,
    items: Vec<ChunkItem>,
    active_item: usize,
    item_filter: String,
    visible_item_count: usize,
}

fn cstr_or_null(ptr: *const c_char) -> String {
    if ptr.is_null() {
        "<null>".to_string()
    } else {
        unsafe { CStr::from_ptr(ptr).to_string_lossy().into_owned() }
    }
}

fn push_field(fields: &mut Vec<ChunkField>, name: &str, value: impl ToString) {
    fields.push(ChunkField {
        name: name.to_string(),
        value: value.to_string(),
    });
}

fn add_count_field(fields: &mut Vec<ChunkField>, name: &str, ptr: *mut std::ffi::c_void, count: usize) {
    push_field(fields, name, if ptr.is_null() { "<null>".to_string() } else { format!("{} items", count) });
}

fn item_label(name: &str, index: usize) -> String {
    if name.is_empty() {
        format!("Item {}", index)
    } else {
        name.to_string()
    }
}

fn matches_item_query(item: &ChunkItem, query: &str) -> bool {
    if query.trim().is_empty() {
        return true;
    }

    let needle = query.trim().to_ascii_lowercase();
    if item.name.to_ascii_lowercase().contains(&needle) {
        return true;
    }

    item.fields.iter().any(|field| {
        field.name.to_ascii_lowercase().contains(&needle)
            || field.value.to_ascii_lowercase().contains(&needle)
    })
}

fn build_pointer_table_items(name: &str, dw: &DataWin) -> Vec<ChunkItem> {
    let mut items = Vec::new();

    match name {
        "OPTN" => {
            let count = dw.optn.constantCount as usize;
            if count == 0 || dw.optn.constants.is_null() {
                return items;
            }
            let constants = unsafe { std::slice::from_raw_parts(dw.optn.constants, count) };
            for (idx, constant) in constants.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "name", cstr_or_null(constant.name));
                push_field(&mut fields, "value", cstr_or_null(constant.value));
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(constant.name), idx),
                    fields,
                });
            }
        }
        "LANG" => {
            let count = dw.lang.languageCount as usize;
            if count == 0 || dw.lang.languages.is_null() {
                return items;
            }
            let languages = unsafe { std::slice::from_raw_parts(dw.lang.languages, count) };
            for (idx, language) in languages.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "name", cstr_or_null(language.name));
                push_field(&mut fields, "region", cstr_or_null(language.region));
                push_field(&mut fields, "entryCount", language.entryCount);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(language.name), idx),
                    fields,
                });
            }
        }
        "EXTN" => {
            let count = dw.extn.count as usize;
            if count == 0 || dw.extn.extensions.is_null() {
                return items;
            }
            let extensions = unsafe { std::slice::from_raw_parts(dw.extn.extensions, count) };
            for (idx, extension) in extensions.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "folderName", cstr_or_null(extension.folderName));
                push_field(&mut fields, "name", cstr_or_null(extension.name));
                push_field(&mut fields, "className", cstr_or_null(extension.className));
                push_field(&mut fields, "fileCount", extension.fileCount);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(extension.name), idx),
                    fields,
                });
            }
        }
        "SOND" => {
            let count = dw.sond.count as usize;
            if count == 0 || dw.sond.sounds.is_null() {
                return items;
            }
            let sounds = unsafe { std::slice::from_raw_parts(dw.sond.sounds, count) };
            for (idx, sound) in sounds.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", sound.present);
                push_field(&mut fields, "name", cstr_or_null(sound.name));
                push_field(&mut fields, "flags", sound.flags);
                push_field(&mut fields, "type", cstr_or_null(sound.type_));
                push_field(&mut fields, "file", cstr_or_null(sound.file));
                push_field(&mut fields, "effects", sound.effects);
                push_field(&mut fields, "volume", sound.volume);
                push_field(&mut fields, "pitch", sound.pitch);
                push_field(&mut fields, "pan", sound.pan);
                push_field(&mut fields, "audioGroup", sound.audioGroup);
                push_field(&mut fields, "audioFile", sound.audioFile);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(sound.name), idx),
                    fields,
                });
            }
        }
        "AGRP" => {
            let count = dw.agrp.count as usize;
            if count == 0 || dw.agrp.audioGroups.is_null() {
                return items;
            }
            let audio_groups = unsafe { std::slice::from_raw_parts(dw.agrp.audioGroups, count) };
            for (idx, group) in audio_groups.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", group.present);
                push_field(&mut fields, "name", cstr_or_null(group.name));
                push_field(&mut fields, "path", cstr_or_null(group.path));
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(group.name), idx),
                    fields,
                });
            }
        }
        "SPRT" => {
            let count = dw.sprt.count as usize;
            if count == 0 || dw.sprt.sprites.is_null() {
                return items;
            }
            let sprites = unsafe { std::slice::from_raw_parts(dw.sprt.sprites, count) };
            for (idx, sprite) in sprites.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", sprite.present);
                push_field(&mut fields, "name", cstr_or_null(sprite.name));
                push_field(&mut fields, "width", sprite.width);
                push_field(&mut fields, "height", sprite.height);
                push_field(&mut fields, "originX", sprite.originX);
                push_field(&mut fields, "originY", sprite.originY);
                push_field(&mut fields, "transparent", sprite.transparent);
                push_field(&mut fields, "smooth", sprite.smooth);
                push_field(&mut fields, "preload", sprite.preload);
                push_field(&mut fields, "bboxMode", sprite.bboxMode);
                push_field(&mut fields, "textureCount", sprite.textureCount);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(sprite.name), idx),
                    fields,
                });
            }
        }
        "BGND" => {
            let count = dw.bgnd.count as usize;
            if count == 0 || dw.bgnd.backgrounds.is_null() {
                return items;
            }
            let backgrounds = unsafe { std::slice::from_raw_parts(dw.bgnd.backgrounds, count) };
            for (idx, background) in backgrounds.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", background.present);
                push_field(&mut fields, "name", cstr_or_null(background.name));
                push_field(&mut fields, "transparent", background.transparent);
                push_field(&mut fields, "smooth", background.smooth);
                push_field(&mut fields, "preload", background.preload);
                push_field(&mut fields, "tpagIndex", background.tpagIndex);
                push_field(&mut fields, "tileWidth", background.gms2TileWidth);
                push_field(&mut fields, "tileHeight", background.gms2TileHeight);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(background.name), idx),
                    fields,
                });
            }
        }
        "PATH" => {
            let count = dw.path.count as usize;
            if count == 0 || dw.path.paths.is_null() {
                return items;
            }
            let paths = unsafe { std::slice::from_raw_parts(dw.path.paths, count) };
            for (idx, path) in paths.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", path.present);
                push_field(&mut fields, "name", cstr_or_null(path.name));
                push_field(&mut fields, "isSmooth", path.isSmooth);
                push_field(&mut fields, "isClosed", path.isClosed);
                push_field(&mut fields, "precision", path.precision);
                push_field(&mut fields, "pointCount", path.pointCount);
                push_field(&mut fields, "length", path.length);
                push_field(&mut fields, "exists", path.exists);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(path.name), idx),
                    fields,
                });
            }
        }
        "SCPT" => {
            let count = dw.scpt.count as usize;
            if count == 0 || dw.scpt.scripts.is_null() {
                return items;
            }
            let scripts = unsafe { std::slice::from_raw_parts(dw.scpt.scripts, count) };
            for (idx, script) in scripts.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", script.present);
                push_field(&mut fields, "name", cstr_or_null(script.name));
                push_field(&mut fields, "codeId", script.codeId);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(script.name), idx),
                    fields,
                });
            }
        }
        "GLOB" => {
            let count = dw.glob.count as usize;
            if count == 0 || dw.glob.codeIds.is_null() {
                return items;
            }
            let code_ids = unsafe { std::slice::from_raw_parts(dw.glob.codeIds, count) };
            for (idx, code_id) in code_ids.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "codeId", *code_id);
                items.push(ChunkItem {
                    name: format!("Code ID {}", idx),
                    fields,
                });
            }
        }
        "SHDR" => {
            let count = dw.shdr.count as usize;
            if count == 0 || dw.shdr.shaders.is_null() {
                return items;
            }
            let shaders = unsafe { std::slice::from_raw_parts(dw.shdr.shaders, count) };
            for (idx, shader) in shaders.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", shader.present);
                push_field(&mut fields, "name", cstr_or_null(shader.name));
                push_field(&mut fields, "type", shader.type_);
                push_field(&mut fields, "version", shader.version);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(shader.name), idx),
                    fields,
                });
            }
        }
        "FONT" => {
            let count = dw.font.count as usize;
            if count == 0 || dw.font.fonts.is_null() {
                return items;
            }
            let fonts = unsafe { std::slice::from_raw_parts(dw.font.fonts, count) };
            for (idx, font) in fonts.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", font.present);
                push_field(&mut fields, "name", cstr_or_null(font.name));
                push_field(&mut fields, "displayName", cstr_or_null(font.displayName));
                push_field(&mut fields, "emSize", font.emSize);
                push_field(&mut fields, "bold", font.bold);
                push_field(&mut fields, "italic", font.italic);
                push_field(&mut fields, "glyphCount", font.glyphCount);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(font.name), idx),
                    fields,
                });
            }
        }
        "TMLN" => {
            let count = dw.tmln.count as usize;
            if count == 0 || dw.tmln.timelines.is_null() {
                return items;
            }
            let timelines = unsafe { std::slice::from_raw_parts(dw.tmln.timelines, count) };
            for (idx, timeline) in timelines.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", timeline.present);
                push_field(&mut fields, "name", cstr_or_null(timeline.name));
                push_field(&mut fields, "momentCount", timeline.momentCount);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(timeline.name), idx),
                    fields,
                });
            }
        }
        "TPAG" => {
            let count = dw.tpag.count as usize;
            if count == 0 || dw.tpag.items.is_null() {
                return items;
            }
            let items_ptr = unsafe { std::slice::from_raw_parts(dw.tpag.items, count) };
            for (idx, item) in items_ptr.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", item.present);
                push_field(&mut fields, "sourceX", item.sourceX);
                push_field(&mut fields, "sourceY", item.sourceY);
                push_field(&mut fields, "sourceWidth", item.sourceWidth);
                push_field(&mut fields, "sourceHeight", item.sourceHeight);
                push_field(&mut fields, "targetX", item.targetX);
                push_field(&mut fields, "targetY", item.targetY);
                push_field(&mut fields, "targetWidth", item.targetWidth);
                push_field(&mut fields, "targetHeight", item.targetHeight);
                push_field(&mut fields, "boundingWidth", item.boundingWidth);
                push_field(&mut fields, "boundingHeight", item.boundingHeight);
                push_field(&mut fields, "texturePageId", item.texturePageId);
                items.push(ChunkItem {
                    name: format!("Texture page {}", idx),
                    fields,
                });
            }
        }
        "OBJT" => {
            let count = dw.objt.count as usize;
            if count == 0 || dw.objt.objects.is_null() {
                return items;
            }
            let objects = unsafe { std::slice::from_raw_parts(dw.objt.objects, count) };
            for (idx, object) in objects.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", object.present);
                push_field(&mut fields, "name", cstr_or_null(object.name));
                push_field(&mut fields, "spriteId", object.spriteId);
                push_field(&mut fields, "visible", object.visible);
                push_field(&mut fields, "solid", object.solid);
                push_field(&mut fields, "depth", object.depth);
                push_field(&mut fields, "persistent", object.persistent);
                push_field(&mut fields, "parentId", object.parentId);
                push_field(&mut fields, "textureMaskId", object.textureMaskId);
                push_field(&mut fields, "physicsVertexCount", object.physicsVertexCount);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(object.name), idx),
                    fields,
                });
            }
        }
        "ROOM" => {
            let count = dw.room.count as usize;
            if count == 0 || dw.room.rooms.is_null() {
                return items;
            }
            let rooms = unsafe { std::slice::from_raw_parts(dw.room.rooms, count) };
            for (idx, room) in rooms.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", room.present);
                push_field(&mut fields, "name", cstr_or_null(room.name));
                push_field(&mut fields, "caption", cstr_or_null(room.caption));
                push_field(&mut fields, "width", room.width);
                push_field(&mut fields, "height", room.height);
                push_field(&mut fields, "speed", room.speed);
                push_field(&mut fields, "persistent", room.persistent);
                push_field(&mut fields, "backgroundColor", room.backgroundColor);
                push_field(&mut fields, "drawBackgroundColor", room.drawBackgroundColor);
                push_field(&mut fields, "creationCodeId", room.creationCodeId);
                push_field(&mut fields, "flags", room.flags);
                push_field(&mut fields, "backgroundsFileOffset", room.backgroundsFileOffset);
                push_field(&mut fields, "viewsFileOffset", room.viewsFileOffset);
                push_field(&mut fields, "gameObjectsFileOffset", room.gameObjectsFileOffset);
                push_field(&mut fields, "tilesFileOffset", room.tilesFileOffset);
                push_field(&mut fields, "world", room.world);
                push_field(&mut fields, "top", room.top);
                push_field(&mut fields, "left", room.left);
                push_field(&mut fields, "right", room.right);
                push_field(&mut fields, "bottom", room.bottom);
                push_field(&mut fields, "gravityX", room.gravityX);
                push_field(&mut fields, "gravityY", room.gravityY);
                push_field(&mut fields, "metersPerPixel", room.metersPerPixel);
                push_field(&mut fields, "layersFileOffset", room.layersFileOffset);
                push_field(&mut fields, "payloadLoaded", room.payloadLoaded);
                push_field(&mut fields, "backgroundCount", if room.backgrounds.is_null() { 0usize } else { 8usize });
                push_field(&mut fields, "viewCount", if room.views.is_null() { 0usize } else { 8usize });
                push_field(&mut fields, "gameObjectCount", room.gameObjectCount);
                push_field(&mut fields, "tileCount", room.tileCount);
                push_field(&mut fields, "layerCount", room.layerCount);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(room.name), idx),
                    fields,
                });
            }
        }
        _ => {}
    }

    items
}

fn build_chunk_fields(name: &str, dw: &DataWin) -> Vec<ChunkField> {
    let mut fields = Vec::new();

    match name {
        "GEN8" => {
            let g = &dw.gen8;
            push_field(&mut fields, "isDebuggerDisabled", g.isDebuggerDisabled);
            push_field(&mut fields, "wadVersion", g.wadVersion);
            push_field(&mut fields, "fileName", cstr_or_null(g.fileName));
            push_field(&mut fields, "config", cstr_or_null(g.config));
            push_field(&mut fields, "lastObj", g.lastObj);
            push_field(&mut fields, "lastTile", g.lastTile);
            push_field(&mut fields, "gameID", g.gameID);
            push_field(&mut fields, "name", cstr_or_null(g.name));
            push_field(&mut fields, "major", g.major);
            push_field(&mut fields, "minor", g.minor);
            push_field(&mut fields, "release", g.release);
            push_field(&mut fields, "build", g.build);
            push_field(&mut fields, "defaultWindowWidth", g.defaultWindowWidth);
            push_field(&mut fields, "defaultWindowHeight", g.defaultWindowHeight);
            push_field(&mut fields, "info", g.info);
            push_field(&mut fields, "licenseCRC32", g.licenseCRC32);
            push_field(&mut fields, "displayName", cstr_or_null(g.displayName));
            push_field(&mut fields, "steamAppID", g.steamAppID);
            push_field(&mut fields, "debuggerPort", g.debuggerPort);
            push_field(&mut fields, "roomOrderCount", g.roomOrderCount);
            add_count_field(&mut fields, "roomOrder", g.roomOrder as *mut std::ffi::c_void, g.roomOrderCount as usize);
            push_field(&mut fields, "gms2FPS", g.gms2FPS);
        }
        "OPTN" => {
            let o = &dw.optn;
            push_field(&mut fields, "shaderExtensionFlag", o.shaderExtensionFlag);
            push_field(&mut fields, "shaderExtensionVersion", o.shaderExtensionVersion);
            push_field(&mut fields, "info", o.info);
            push_field(&mut fields, "scale", o.scale);
            push_field(&mut fields, "windowColor", o.windowColor);
            push_field(&mut fields, "colorDepth", o.colorDepth);
            push_field(&mut fields, "resolution", o.resolution);
            push_field(&mut fields, "frequency", o.frequency);
            push_field(&mut fields, "vertexSync", o.vertexSync);
            push_field(&mut fields, "priority", o.priority);
            push_field(&mut fields, "backImage", o.backImage);
            push_field(&mut fields, "frontImage", o.frontImage);
            push_field(&mut fields, "loadImage", o.loadImage);
            push_field(&mut fields, "loadAlpha", o.loadAlpha);
            push_field(&mut fields, "constantCount", o.constantCount);
            add_count_field(&mut fields, "constants", o.constants as *mut std::ffi::c_void, o.constantCount as usize);
        }
        "LANG" => {
            let l = &dw.lang;
            push_field(&mut fields, "unknown1", l.unknown1);
            push_field(&mut fields, "languageCount", l.languageCount);
            push_field(&mut fields, "entryCount", l.entryCount);
            add_count_field(&mut fields, "entryIds", l.entryIds as *mut std::ffi::c_void, l.entryCount as usize);
            add_count_field(&mut fields, "languages", l.languages as *mut std::ffi::c_void, l.languageCount as usize);
        }
        "EXTN" => {
            let e = &dw.extn;
            push_field(&mut fields, "count", e.count);
            add_count_field(&mut fields, "extensions", e.extensions as *mut std::ffi::c_void, e.count as usize);
        }
        "SOND" => {
            let s = &dw.sond;
            push_field(&mut fields, "count", s.count);
            add_count_field(&mut fields, "sounds", s.sounds as *mut std::ffi::c_void, s.count as usize);
        }
        "AGRP" => {
            let a = &dw.agrp;
            push_field(&mut fields, "count", a.count);
            add_count_field(&mut fields, "audioGroups", a.audioGroups as *mut std::ffi::c_void, a.count as usize);
        }
        "SPRT" => {
            let s = &dw.sprt;
            push_field(&mut fields, "count", s.count);
            push_field(&mut fields, "parsedCount", s.parsedCount);
            add_count_field(&mut fields, "sprites", s.sprites as *mut std::ffi::c_void, s.count as usize);
        }
        "BGND" => {
            let b = &dw.bgnd;
            push_field(&mut fields, "count", b.count);
            add_count_field(&mut fields, "backgrounds", b.backgrounds as *mut std::ffi::c_void, b.count as usize);
        }
        "PATH" => {
            let p = &dw.path;
            push_field(&mut fields, "count", p.count);
            add_count_field(&mut fields, "paths", p.paths as *mut std::ffi::c_void, p.count as usize);
        }
        "SCPT" => {
            let s = &dw.scpt;
            push_field(&mut fields, "count", s.count);
            add_count_field(&mut fields, "scripts", s.scripts as *mut std::ffi::c_void, s.count as usize);
        }
        "GLOB" => {
            let g = &dw.glob;
            push_field(&mut fields, "count", g.count);
            add_count_field(&mut fields, "codeIds", g.codeIds as *mut std::ffi::c_void, g.count as usize);
        }
        "SHDR" => {
            let s = &dw.shdr;
            push_field(&mut fields, "count", s.count);
            add_count_field(&mut fields, "shaders", s.shaders as *mut std::ffi::c_void, s.count as usize);
        }
        "FONT" => {
            let f = &dw.font;
            push_field(&mut fields, "count", f.count);
            add_count_field(&mut fields, "fonts", f.fonts as *mut std::ffi::c_void, f.count as usize);
        }
        "TMLN" => {
            let t = &dw.tmln;
            push_field(&mut fields, "count", t.count);
            add_count_field(&mut fields, "timelines", t.timelines as *mut std::ffi::c_void, t.count as usize);
        }
        "TPAG" => {
            let t = &dw.tpag;
            push_field(&mut fields, "count", t.count);
            add_count_field(&mut fields, "items", t.items as *mut std::ffi::c_void, t.count as usize);
        }
        "OBJT" => {
            let o = &dw.objt;
            push_field(&mut fields, "count", o.count);
            add_count_field(&mut fields, "objects", o.objects as *mut std::ffi::c_void, o.count as usize);
        }
        "ROOM" => {
            let r = &dw.room;
            push_field(&mut fields, "count", r.count);
            add_count_field(&mut fields, "rooms", r.rooms as *mut std::ffi::c_void, r.count as usize);
        }
        _ => {}
    }

    fields
}

struct App {
    version: String,
    file_path: String,
    chunks: Vec<ChunkInfo>,
    active_chunk: usize,
}

impl eframe::App for App {
    fn ui(&mut self, ui: &mut egui::Ui, _frame: &mut eframe::Frame) {
        ui.ctx().set_visuals(egui::Visuals::dark());
        ui.ctx().set_theme(egui::ThemePreference::Dark);

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
                            "SHDR" => "Shaders",
                            "FONT" => "Fonts",
                            "TMLN" => "Timelines",
                            "TPAG" => "Texture Pages",
                            "OBJT" => "Objects",
                            "ROOM" => "Rooms",
                            name => name,
                        };

                        let selected = self.active_chunk == idx;
                        if ui
                            .selectable_label(selected, chunk_name)
                            .clicked()
                        {
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
                            ui.label("No structured fields are available for this chunk.");
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
                    .unwrap_or_else(|| active_chunk_items.first().cloned().unwrap_or_else(|| ChunkItem {
                        name: "Unknown item".to_string(),
                        fields: Vec::new(),
                    }));

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
                                if ui
                                    .selectable_label(selected, item.name.clone())
                                    .clicked()
                                {
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
                        ui.group(|ui| {
                            ui.label(format!("Chunk: {}", active_chunk_name));
                            ui.label(format!("Offset: {}", active_chunk_offset));
                            ui.label(format!("Length: {}", active_chunk_length));
                            ui.label(format!("Item: {}", active_item.name));
                            ui.separator();

                            egui::ScrollArea::vertical().max_height(260.0).show(ui, |ui| {
                                egui::Grid::new(format!("chunk-item-fields-{}-{}", active_chunk_name, active_item.name))
                                    .num_columns(2)
                                    .spacing([12.0, 4.0])
                                    .show(ui, |ui| {
                                        for field in &active_item.fields {
                                            ui.label(&field.name);
                                            ui.label(&field.value);
                                            ui.end_row();
                                        }
                                    });
                            });
                        });
                    });
                });
            });
    }
}

fn main() {
    let file_path = std::env::args().nth(1).unwrap_or_else(|| {
        eprintln!(
            "Usage: {} <path-to-data.win>",
            std::env::args()
                .next()
                .unwrap_or_else(|| "gmdataparser".to_string())
        );
        std::process::exit(1);
    });

    let path = CString::new(file_path.as_str()).unwrap_or_else(|_| {
        panic!("File path contains a NUL byte");
    });

    let mut dw: DataWin = unsafe { std::mem::zeroed() };
    let version: String;
    let mut chunks = Vec::new();

    unsafe {
        println!("Loading file: {:?}", path);

        if DataWin_loadFile(&mut dw, path.as_ptr()) != 0 {
            panic!("Failed to load file");
        }

        if DataWin_parse(&mut dw) != 0 {
            panic!("Failed to parse file");
        }

        version = format!(
            "{}.{}.{}.{}",
            dw.detectedFormat.major,
            dw.detectedFormat.minor,
            dw.detectedFormat.release,
            dw.detectedFormat.build
        );

        println!("Detected version: {}", version);

        if dw.chunks.count > 0 {
            let items = std::slice::from_raw_parts(dw.chunks.items, dw.chunks.count);
            chunks = items
                .iter()
                .map(|chunk| {
                    let name = CStr::from_ptr(chunk.name.as_ptr())
                        .to_string_lossy()
                        .into_owned();
                    let fields = build_chunk_fields(&name, &dw);
                    let items = build_pointer_table_items(&name, &dw);
                    ChunkInfo {
                        name: name.clone(),
                        offset: chunk.offset,
                        length: chunk.length,
                        fields,
                        items,
                        active_item: 0,
                        item_filter: String::new(),
                        visible_item_count: 100,
                    }
                })
                .collect();
        }
    }

    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([600.0, 400.0]),
        ..Default::default()
    };

    let run_result = eframe::run_native(
        "gmdataparser",
        options,
        Box::new(move |_cc| {
            Ok(Box::new(App {
                version,
                file_path,
                chunks,
                active_chunk: 0,
            }))
        }),
    );

    unsafe {
        DataWin_free(&mut dw);
    }

    run_result.expect("Failed to start GUI");
}