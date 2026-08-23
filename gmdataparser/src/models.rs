use std::ffi::CStr;

use crate::bindings::*;

#[derive(Clone, Debug)]
pub struct ChunkField {
    pub name: String,
    pub value: String,
}

#[derive(Clone, Debug)]
pub struct ChunkItem {
    pub name: String,
    pub fields: Vec<ChunkField>,
}

#[derive(Clone, Debug)]
pub struct ChunkInfo {
    pub name: String,
    pub offset: u32,
    pub length: u32,
    pub fields: Vec<ChunkField>,
    pub items: Vec<ChunkItem>,
    pub active_item: usize,
    pub item_filter: String,
    pub visible_item_count: usize,
}

pub fn cstr_or_null(ptr: *const std::os::raw::c_char) -> String {
    if ptr.is_null() {
        "<null>".to_string()
    } else {
        unsafe { CStr::from_ptr(ptr).to_string_lossy().into_owned() }
    }
}

pub fn push_field(fields: &mut Vec<ChunkField>, name: &str, value: impl ToString) {
    fields.push(ChunkField {
        name: name.to_string(),
        value: value.to_string(),
    });
}

pub fn add_count_field(
    fields: &mut Vec<ChunkField>,
    name: &str,
    ptr: *mut std::ffi::c_void,
    count: usize,
) {
    push_field(
        fields,
        name,
        if ptr.is_null() {
            "<null>".to_string()
        } else {
            format!("{} items", count)
        },
    );
}

pub fn item_label(name: &str, index: usize) -> String {
    if name.is_empty() {
        format!("Item {}", index)
    } else {
        name.to_string()
    }
}

pub fn checked_ptr_slice<T: Copy>(ptr: *mut T, count: usize) -> Option<Vec<T>> {
    if ptr.is_null() || count == 0 {
        return None;
    }

    let addr = ptr as usize;
    if addr % std::mem::align_of::<T>() != 0 {
        return None;
    }

    let byte_count = count.checked_mul(std::mem::size_of::<T>())?;
    if byte_count > isize::MAX as usize {
        return None;
    }

    unsafe {
        let slice = std::slice::from_raw_parts(ptr, count);
        Some(slice.to_vec())
    }
}

pub fn matches_item_query(item: &ChunkItem, query: &str) -> bool {
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

pub fn build_pointer_table_items(name: &str, dw: &DataWin) -> Vec<ChunkItem> {
    let mut items = Vec::new();

    match name {
        "OPTN" => {
            let count = dw.optn.constantCount as usize;
            if count == 0 || dw.optn.constants.is_null() {
                return items;
            }
            let Some(constants) = checked_ptr_slice(dw.optn.constants, count) else {
                return items;
            };
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
            let Some(languages) = checked_ptr_slice(dw.lang.languages, count) else {
                return items;
            };
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
            let Some(extensions) = checked_ptr_slice(dw.extn.extensions, count) else {
                return items;
            };
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
            let Some(sounds) = checked_ptr_slice(dw.sond.sounds, count) else {
                return items;
            };
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
            let Some(audio_groups) = checked_ptr_slice(dw.agrp.audioGroups, count) else {
                return items;
            };
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
            let Some(sprites) = checked_ptr_slice(dw.sprt.sprites, count) else {
                return items;
            };
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
            let Some(backgrounds) = checked_ptr_slice(dw.bgnd.backgrounds, count) else {
                return items;
            };
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
            let Some(paths) = checked_ptr_slice(dw.path.paths, count) else {
                return items;
            };
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
            let Some(scripts) = checked_ptr_slice(dw.scpt.scripts, count) else {
                return items;
            };
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
            let Some(code_ids) = checked_ptr_slice(dw.glob.codeIds, count) else {
                return items;
            };
            for (idx, code_id) in code_ids.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "codeId", *code_id);
                items.push(ChunkItem {
                    name: format!("Code ID {}", idx),
                    fields,
                });
            }
        }
        "CODE" => {
            let count = dw.code.count as usize;
            if count == 0 || dw.code.entries.is_null() {
                return items;
            }
            let Some(entries) = checked_ptr_slice(dw.code.entries, count) else {
                return items;
            };
            for (idx, entry) in entries.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", entry.present);
                push_field(&mut fields, "name", cstr_or_null(entry.name));
                push_field(&mut fields, "length", entry.length);
                push_field(&mut fields, "localsCount", entry.localsCount);
                push_field(&mut fields, "argumentsCount", entry.argumentsCount);
                push_field(&mut fields, "offset", entry.offset);
                push_field(&mut fields, "bytecodeAbsoluteOffset", entry.bytecodeAbsoluteOffset);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(entry.name), idx),
                    fields,
                });
            }
        }
        "VARI" => {
            let count = dw.vari.variableCount as usize;
            if count == 0 || dw.vari.variables.is_null() {
                return items;
            }
            let Some(variables) = checked_ptr_slice(dw.vari.variables, count) else {
                return items;
            };
            for (idx, variable) in variables.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "name", cstr_or_null(variable.name));
                push_field(&mut fields, "instanceType", variable.instanceType);
                push_field(&mut fields, "varID", variable.varID);
                push_field(&mut fields, "occurrences", variable.occurrences);
                push_field(&mut fields, "firstAddress", variable.firstAddress);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(variable.name), idx),
                    fields,
                });
            }
        }
        "FUNC" => {
            let count = dw.func.functionCount as usize;
            if count == 0 || dw.func.functions.is_null() {
                return items;
            }
            let Some(functions) = checked_ptr_slice(dw.func.functions, count) else {
                return items;
            };
            for (idx, function) in functions.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "name", cstr_or_null(function.name));
                push_field(&mut fields, "occurrences", function.occurrences);
                push_field(&mut fields, "firstAddress", function.firstAddress);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(function.name), idx),
                    fields,
                });
            }
        }
        "SHDR" => {
            let count = dw.shdr.count as usize;
            if count == 0 || dw.shdr.shaders.is_null() {
                return items;
            }
            let Some(shaders) = checked_ptr_slice(dw.shdr.shaders, count) else {
                return items;
            };
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
            let Some(fonts) = checked_ptr_slice(dw.font.fonts, count) else {
                return items;
            };
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
            let Some(timelines) = checked_ptr_slice(dw.tmln.timelines, count) else {
                return items;
            };
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
            let Some(items_ptr) = checked_ptr_slice(dw.tpag.items, count) else {
                return items;
            };
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
            let Some(objects) = checked_ptr_slice(dw.objt.objects, count) else {
                return items;
            };
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
            let Some(rooms) = checked_ptr_slice(dw.room.rooms, count) else {
                return items;
            };
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
        "ACRV" => {
            let count = dw.acrv.count as usize;
            if count == 0 || dw.acrv.curves.is_null() {
                return items;
            }
            let Some(curves) = checked_ptr_slice(dw.acrv.curves, count) else {
                return items;
            };
            for (idx, curve) in curves.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", curve.present);
                push_field(&mut fields, "name", cstr_or_null(curve.name));
                push_field(&mut fields, "graphType", curve.graphType);
                push_field(&mut fields, "channelCount", curve.channelCount);
                push_field(&mut fields, "globalId", curve.globalId);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(curve.name), idx),
                    fields,
                });
            }
        }
        "FEDS" => {
            let count = dw.feds.count as usize;
            if count == 0 || dw.feds.effects.is_null() {
                return items;
            }
            let Some(effects) = checked_ptr_slice(dw.feds.effects, count) else {
                return items;
            };
            for (idx, effect) in effects.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", effect.present);
                push_field(&mut fields, "name", cstr_or_null(effect.name));
                push_field(&mut fields, "value", cstr_or_null(effect.value));
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(effect.name), idx),
                    fields,
                });
            }
        }
        "FEAT" => {
            let count = dw.feat.count as usize;
            if count == 0 || dw.feat.strings.is_null() {
                return items;
            }
            let Some(strings) = checked_ptr_slice(dw.feat.strings, count) else {
                return items;
            };
            for (idx, string) in strings.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "value", cstr_or_null(*string));
                items.push(ChunkItem {
                    name: format!("Feature {}", idx),
                    fields,
                });
            }
        }
        "SEQN" => {
            let count = dw.seqn.count as usize;
            if count == 0 || dw.seqn.items.is_null() {
                return items;
            }
            let Some(sequences) = checked_ptr_slice(dw.seqn.items, count) else {
                return items;
            };
            for (idx, sequence) in sequences.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "name", cstr_or_null(sequence.name));
                push_field(&mut fields, "playback", sequence.playback);
                push_field(&mut fields, "playbackSpeed", sequence.playback_speed);
                push_field(&mut fields, "playbackSpeedType", sequence.playback_speed_type);
                push_field(&mut fields, "length", sequence.length);
                push_field(&mut fields, "originX", sequence.origin_x);
                push_field(&mut fields, "originY", sequence.origin_y);
                push_field(&mut fields, "volume", sequence.volume);
                push_field(&mut fields, "width", sequence.width);
                push_field(&mut fields, "height", sequence.height);
                push_field(&mut fields, "broadcastMessageCount", sequence.broadcast_message_count);
                push_field(&mut fields, "trackCount", sequence.track_count);
                push_field(&mut fields, "functionIdCount", sequence.function_id_count);
                push_field(&mut fields, "momentCount", sequence.moment_count);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(sequence.name), idx),
                    fields,
                });
            }
        }
        "TAGS" => {
            let count = dw.tags.count as usize;
            if count > 0 && !dw.tags.strings.is_null() {
                let Some(strings) = checked_ptr_slice(dw.tags.strings, count) else {
                    return items;
                };
                for (idx, string) in strings.iter().enumerate() {
                    let mut fields = Vec::new();
                    push_field(&mut fields, "value", cstr_or_null(*string));
                    items.push(ChunkItem {
                        name: format!("String {}", idx),
                        fields,
                    });
                }
            }

            let asset_count = dw.tags.asset_tag_count as usize;
            if asset_count == 0 || dw.tags.asset_tags.is_null() {
                return items;
            }
            let Some(asset_tags) = checked_ptr_slice(dw.tags.asset_tags, asset_count) else {
                return items;
            };
            for (idx, asset_tag) in asset_tags.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "id", asset_tag.id);
                push_field(&mut fields, "tagCount", asset_tag.tag_count);
                let tag_count = asset_tag.tag_count as usize;
                if tag_count > 0 && !asset_tag.tags.is_null() {
                    let Some(tags) = checked_ptr_slice(asset_tag.tags, tag_count) else {
                        return items;
                    };
                    let tag_names: Vec<String> = tags.iter().map(|tag| cstr_or_null(*tag)).collect();
                    push_field(&mut fields, "tags", tag_names.join(", "));
                }
                items.push(ChunkItem {
                    name: format!("Asset tag {}", idx),
                    fields,
                });
            }
        }
        "EMBI" => {
            let count = dw.embi.count as usize;
            if count == 0 || dw.embi.items.is_null() {
                return items;
            }
            let Some(entries) = checked_ptr_slice(dw.embi.items, count) else {
                return items;
            };
            for (idx, entry) in entries.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "name", cstr_or_null(entry.name));
                push_field(&mut fields, "texturePageEntryId", entry.texture_page_entry_id);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(entry.name), idx),
                    fields,
                });
            }
        }
        "PSEM" => {
            let count = dw.psem.count as usize;
            if count == 0 || dw.psem.items.is_null() {
                return items;
            }
            let Some(emitter_items) = checked_ptr_slice(dw.psem.items, count) else {
                return items;
            };
            for (idx, emitter) in emitter_items.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "name", cstr_or_null(emitter.name));
                push_field(&mut fields, "enabled", emitter.enabled);
                push_field(&mut fields, "mode", emitter.mode);
                push_field(&mut fields, "emitCount", emitter.emit_count);
                push_field(&mut fields, "emitRelative", emitter.emit_relative);
                push_field(&mut fields, "delayMin", emitter.delay_min);
                push_field(&mut fields, "delayMax", emitter.delay_max);
                push_field(&mut fields, "distribution", emitter.distribution);
                push_field(&mut fields, "shape", emitter.shape);
                push_field(&mut fields, "spriteId", emitter.sprite_id);
                push_field(&mut fields, "textureEnum", emitter.texture_enum);
                push_field(&mut fields, "frameIndex", emitter.frame_index);
                push_field(&mut fields, "lifetimeMin", emitter.lifetime_min);
                push_field(&mut fields, "lifetimeMax", emitter.lifetime_max);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(emitter.name), idx),
                    fields,
                });
            }
        }
        "PSYS" => {
            let count = dw.psys.count as usize;
            if count == 0 || dw.psys.items.is_null() {
                return items;
            }
            let Some(systems) = checked_ptr_slice(dw.psys.items, count) else {
                return items;
            };
            for (idx, system) in systems.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "name", cstr_or_null(system.name));
                push_field(&mut fields, "originX", system.origin_x);
                push_field(&mut fields, "originY", system.origin_y);
                push_field(&mut fields, "drawOrder", system.draw_order);
                push_field(&mut fields, "globalSpaceParticles", system.global_space_particles);
                push_field(&mut fields, "emitterCount", system.emitter_count);
                if system.emitter_count > 0 && !system.emitters.is_null() {
                    let emitters = system.emitters;
                    let Some(emitter_slice) = checked_ptr_slice(emitters, system.emitter_count as usize) else {
                        return items;
                    };
                    let emitter_names: Vec<String> = emitter_slice
                        .iter()
                        .map(|emitter| cstr_or_null(emitter.name))
                        .collect();
                    push_field(&mut fields, "emitters", emitter_names.join(", "));
                }
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(system.name), idx),
                    fields,
                });
            }
        }
        "GMEN" => {
            let count = dw.gmen.count as usize;
            if count == 0 || dw.gmen.code_ids.is_null() {
                return items;
            }
            let Some(code_ids) = checked_ptr_slice(dw.gmen.code_ids, count) else {
                return items;
            };
            for (idx, code_id) in code_ids.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "codeId", *code_id);
                items.push(ChunkItem {
                    name: format!("Code ID {}", idx),
                    fields,
                });
            }
        }
        "STAT" => {
            let count = dw.stat.eventCount as usize;
            if count == 0 || dw.stat.events.is_null() {
                return items;
            }
            let Some(events) = checked_ptr_slice(dw.stat.events, count) else {
                return items;
            };
            for (idx, event) in events.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "name", cstr_or_null(event.name));
                push_field(&mut fields, "version", cstr_or_null(event.version));
                push_field(&mut fields, "latency", event.latency);
                push_field(&mut fields, "priority", event.priority);
                push_field(&mut fields, "enabled", event.enabled);
                push_field(&mut fields, "populationSampleRate", event.populationSampleRate);
                push_field(&mut fields, "id", event.id);
                push_field(&mut fields, "partCVersion", event.partCVersion);
                push_field(&mut fields, "fieldCount", event.fieldCount);
                if event.fieldCount > 0 && !event.fields.is_null() {
                    let field_count = event.fieldCount as usize;
                    let Some(fields_ptr) = checked_ptr_slice(event.fields, field_count) else {
                        return items;
                    };
                    let field_names: Vec<String> = fields_ptr
                        .iter()
                        .map(|field| format!("{}:{}", cstr_or_null(field.name), field.type_))
                        .collect();
                    push_field(&mut fields, "fields", field_names.join(", "));
                }
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(event.name), idx),
                    fields,
                });
            }
        }
        "DAFL" => {
            let count = dw.dafl.count as usize;
            if count == 0 {
                return items;
            }
            items.push(ChunkItem {
                name: "Data file list".to_string(),
                fields: vec![ChunkField {
                    name: "count".to_string(),
                    value: count.to_string(),
                }],
            });
        }
        "UILR" => {
            let count = dw.uilr.count as usize;
            if count == 0 {
                return items;
            }
            items.push(ChunkItem {
                name: "UI layout".to_string(),
                fields: vec![ChunkField {
                    name: "count".to_string(),
                    value: count.to_string(),
                }],
            });
        }
        "STRG" => {
            let count = dw.strg.count as usize;
            if count == 0 || dw.strg.strings.is_null() {
                return items;
            }
            let Some(strings) = checked_ptr_slice(dw.strg.strings, count) else {
                return items;
            };
            for (idx, string) in strings.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "value", cstr_or_null(*string));
                items.push(ChunkItem {
                    name: format!("String {}", idx),
                    fields,
                });
            }
        }
        "TGIN" => {
            let count = dw.tgin.count as usize;
            if count == 0 || dw.tgin.groups.is_null() {
                return items;
            }
            let Some(groups) = checked_ptr_slice(dw.tgin.groups, count) else {
                return items;
            };
            for (idx, group) in groups.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", group.present);
                push_field(&mut fields, "name", cstr_or_null(group.name));
                push_field(&mut fields, "directory", cstr_or_null(group.directory));
                push_field(&mut fields, "extension", cstr_or_null(group.extension));
                push_field(&mut fields, "loadType", group.loadType);
                push_field(&mut fields, "texturePageCount", group.texturePageCount);
                push_field(&mut fields, "spriteCount", group.spriteCount);
                push_field(&mut fields, "spineSpriteCount", group.spineSpriteCount);
                push_field(&mut fields, "fontCount", group.fontCount);
                push_field(&mut fields, "tileSetCount", group.tileSetCount);
                add_count_field(&mut fields, "texturePages", group.texturePages as *mut std::ffi::c_void, group.texturePageCount as usize);
                add_count_field(&mut fields, "sprites", group.sprites as *mut std::ffi::c_void, group.spriteCount as usize);
                add_count_field(&mut fields, "spineSprites", group.spineSprites as *mut std::ffi::c_void, group.spineSpriteCount as usize);
                add_count_field(&mut fields, "fonts", group.fonts as *mut std::ffi::c_void, group.fontCount as usize);
                add_count_field(&mut fields, "tilesets", group.tilesets as *mut std::ffi::c_void, group.tileSetCount as usize);
                items.push(ChunkItem {
                    name: item_label(&cstr_or_null(group.name), idx),
                    fields,
                });
            }
        }
        "TXTR" => {
            let count = dw.txtr.count as usize;
            if count == 0 || dw.txtr.textures.is_null() {
                return items;
            }
            let Some(textures) = checked_ptr_slice(dw.txtr.textures, count) else {
                return items;
            };
            for (idx, texture) in textures.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", texture.present);
                push_field(&mut fields, "scaled", texture.scaled);
                push_field(&mut fields, "generatedMips", texture.generatedMips);
                push_field(&mut fields, "textureBlockSize", texture.textureBlockSize);
                push_field(&mut fields, "textureWidth", texture.textureWidth);
                push_field(&mut fields, "textureHeight", texture.textureHeight);
                push_field(&mut fields, "indexInGroup", texture.indexInGroup);
                push_field(&mut fields, "blobOffset", texture.blobOffset);
                push_field(&mut fields, "blobSize", texture.blobSize);
                push_field(&mut fields, "mapped", texture.mapped);
                match crate::texture::texture_bytes(texture) {
                    Some(bytes) => push_field(&mut fields, "bytes", format!("{} bytes", bytes.len())),
                    None => push_field(&mut fields, "bytes", "unloaded"),
                }
                items.push(ChunkItem {
                    name: format!("Texture {}", idx),
                    fields,
                });
            }
        }
        "AUDO" => {
            let count = dw.audo.count as usize;
            if count == 0 || dw.audo.entries.is_null() {
                return items;
            }
            let Some(entries) = checked_ptr_slice(dw.audo.entries, count) else {
                return items;
            };
            for (idx, entry) in entries.iter().enumerate() {
                let mut fields = Vec::new();
                push_field(&mut fields, "present", entry.present);
                push_field(&mut fields, "dataSize", entry.dataSize);
                push_field(&mut fields, "dataOffset", entry.dataOffset);
                items.push(ChunkItem {
                    name: format!("Audio {}", idx),
                    fields,
                });
            }
        }
        _ => {}
    }

    items
}

pub fn build_chunk_fields(name: &str, dw: &DataWin) -> Vec<ChunkField> {
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
        "CODE" => {
            let c = &dw.code;
            push_field(&mut fields, "count", c.count);
            add_count_field(&mut fields, "entries", c.entries as *mut std::ffi::c_void, c.count as usize);
            push_field(&mut fields, "bytecodeBase", c.bytecodeBase);
            push_field(&mut fields, "bytecodeSize", c.bytecodeSize);
            add_count_field(&mut fields, "bytecodeData", c.bytecodeData as *mut std::ffi::c_void, c.bytecodeSize as usize);
        }
        "VARI" => {
            let v = &dw.vari;
            push_field(&mut fields, "varCount1", v.varCount1);
            push_field(&mut fields, "varCount2", v.varCount2);
            push_field(&mut fields, "maxLocalVarCount", v.maxLocalVarCount);
            push_field(&mut fields, "variableCount", v.variableCount);
            add_count_field(&mut fields, "variables", v.variables as *mut std::ffi::c_void, v.variableCount as usize);
        }
        "FUNC" => {
            let f = &dw.func;
            push_field(&mut fields, "functionCount", f.functionCount);
            add_count_field(&mut fields, "functions", f.functions as *mut std::ffi::c_void, f.functionCount as usize);
            push_field(&mut fields, "codeLocalsCount", f.codeLocalsCount);
            add_count_field(&mut fields, "codeLocals", f.codeLocals as *mut std::ffi::c_void, f.codeLocalsCount as usize);
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
        "ACRV" => {
            let a = &dw.acrv;
            push_field(&mut fields, "count", a.count);
            add_count_field(&mut fields, "curves", a.curves as *mut std::ffi::c_void, a.count as usize);
            push_field(&mut fields, "allChannelsCount", a.allChannelsCount);
            add_count_field(&mut fields, "allChannels", a.allChannels as *mut std::ffi::c_void, a.allChannelsCount as usize);
        }
        "FEDS" => {
            let f = &dw.feds;
            push_field(&mut fields, "count", f.count);
            add_count_field(&mut fields, "effects", f.effects as *mut std::ffi::c_void, f.count as usize);
        }
        "FEAT" => {
            let f = &dw.feat;
            push_field(&mut fields, "count", f.count);
            add_count_field(&mut fields, "strings", f.strings as *mut std::ffi::c_void, f.count as usize);
        }
        "SEQN" => {
            let s = &dw.seqn;
            push_field(&mut fields, "count", s.count);
            add_count_field(&mut fields, "sequences", s.items as *mut std::ffi::c_void, s.count as usize);
        }
        "TAGS" => {
            let t = &dw.tags;
            push_field(&mut fields, "count", t.count);
            add_count_field(&mut fields, "strings", t.strings as *mut std::ffi::c_void, t.count as usize);
            push_field(&mut fields, "assetTagCount", t.asset_tag_count);
            add_count_field(&mut fields, "assetTags", t.asset_tags as *mut std::ffi::c_void, t.asset_tag_count as usize);
        }
        "EMBI" => {
            let e = &dw.embi;
            push_field(&mut fields, "count", e.count);
            add_count_field(&mut fields, "items", e.items as *mut std::ffi::c_void, e.count as usize);
        }
        "PSEM" => {
            let p = &dw.psem;
            push_field(&mut fields, "count", p.count);
            add_count_field(&mut fields, "emitters", p.items as *mut std::ffi::c_void, p.count as usize);
        }
        "PSYS" => {
            let p = &dw.psys;
            push_field(&mut fields, "count", p.count);
            add_count_field(&mut fields, "systems", p.items as *mut std::ffi::c_void, p.count as usize);
        }
        "GMEN" => {
            let g = &dw.gmen;
            push_field(&mut fields, "count", g.count);
            add_count_field(&mut fields, "codeIds", g.code_ids as *mut std::ffi::c_void, g.count as usize);
        }
        "STAT" => {
            let s = &dw.stat;
            push_field(&mut fields, "providerName", cstr_or_null(s.providerName));
            let guid_hex: String = s.providerGuid.iter().map(|b| format!("{:02x}", b)).collect();
            push_field(&mut fields, "providerGuid", guid_hex);
            push_field(&mut fields, "providerLatency", s.providerLatency);
            push_field(&mut fields, "providerPriority", s.providerPriority);
            push_field(&mut fields, "providerEnabled", s.providerEnabled);
            push_field(&mut fields, "populationSampleRateCount", s.populationSampleRateCount);
            if s.populationSampleRateCount > 0 && !s.populationSampleRates.is_null() {
                let count = s.populationSampleRateCount as usize;
                let Some(samples) = checked_ptr_slice(s.populationSampleRates, count) else {
                    return fields;
                };
                push_field(&mut fields, "populationSampleRates", samples.iter().map(|v| v.to_string()).collect::<Vec<_>>().join(", "));
            }
            push_field(&mut fields, "eventCount", s.eventCount);
            add_count_field(&mut fields, "events", s.events as *mut std::ffi::c_void, s.eventCount as usize);
        }
        "DAFL" => {
            let d = &dw.dafl;
            push_field(&mut fields, "count", d.count);
        }
        "UILR" => {
            let u = &dw.uilr;
            push_field(&mut fields, "count", u.count);
        }
        "STRG" => {
            let s = &dw.strg;
            push_field(&mut fields, "count", s.count);
            add_count_field(&mut fields, "strings", s.strings as *mut std::ffi::c_void, s.count as usize);
        }
        "TGIN" => {
            let t = &dw.tgin;
            push_field(&mut fields, "count", t.count);
            add_count_field(&mut fields, "groups", t.groups as *mut std::ffi::c_void, t.count as usize);
        }
        "TXTR" => {
            let t = &dw.txtr;
            push_field(&mut fields, "count", t.count);
            add_count_field(&mut fields, "textures", t.textures as *mut std::ffi::c_void, t.count as usize);
        }
        "AUDO" => {
            let a = &dw.audo;
            push_field(&mut fields, "count", a.count);
            add_count_field(&mut fields, "entries", a.entries as *mut std::ffi::c_void, a.count as usize);
        }
        _ => {}
    }

    fields
}

pub fn build_chunk_info_list(dw: &DataWin) -> Vec<ChunkInfo> {
    let mut chunks = Vec::new();

    if dw.chunks.count > 0 {
        let chunk_items = match checked_ptr_slice(dw.chunks.items, dw.chunks.count as usize) {
            Some(items) => items,
            None => {
                println!(
                    "Skipping chunk table: invalid pointer or size for {} entries",
                    dw.chunks.count
                );
                Vec::new()
            }
        };

        if !chunk_items.is_empty() {
            chunks = chunk_items
                .iter()
                .map(|chunk| {
                    let name = unsafe { CStr::from_ptr(chunk.name.as_ptr()) }
                        .to_string_lossy()
                        .into_owned();
                    let fields = build_chunk_fields(&name, dw);
                    let items = build_pointer_table_items(&name, dw);
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

    chunks
}