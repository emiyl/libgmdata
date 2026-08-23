use crate::bindings::*;
use eframe::egui;

pub fn texture_bytes(texture: &Texture) -> Option<&[u8]> {
    if !texture.present || texture.blobData.is_null() || texture.blobSize == 0 {
        return None;
    }

    let len = texture.blobSize as usize;
    unsafe { Some(std::slice::from_raw_parts(texture.blobData, len)) }
}

pub fn decode_native_texture_rgba(data: &[u8], gm2022_5: bool) -> Option<Vec<u8>> {
    if data.is_empty() {
        return None;
    }

    let mut out_w = 0i32;
    let mut out_h = 0i32;

    let ptr = unsafe {
        crate::bindings::TextureDecode_decodeToRgba(data.as_ptr(), data.len(), gm2022_5, &mut out_w, &mut out_h)
    };
    if ptr.is_null() {
        return None;
    }

    let width = usize::try_from(out_w).ok()?;
    let height = usize::try_from(out_h).ok()?;
    let pixel_count = if out_w > 0 && out_h > 0 {
        width.checked_mul(height)?.checked_mul(4)?
    } else {
        data.len().checked_div(4)? * 4
    };

    let rgba = unsafe { std::slice::from_raw_parts(ptr, pixel_count).to_vec() };
    unsafe {
        unsafe extern "C" {
            fn free(ptr: *mut std::ffi::c_void);
        }
        free(ptr as *mut std::ffi::c_void);
    }
    Some(rgba)
}

pub fn fallback_texture_image(width: usize, height: usize) -> egui::ColorImage {
    let mut pixels = vec![0u8; width.saturating_mul(height).saturating_mul(4)];
    let dark = [24, 24, 28, 255];
    let light = [50, 50, 56, 255];
    let accent = [190, 64, 255, 255];

    for y in 0..height {
        for x in 0..width {
            let idx = (y * width + x) * 4;
            let checker = ((x / 8) + (y / 8)) % 2 == 0;
            let color = if checker { dark } else { light };
            pixels[idx..idx + 4].copy_from_slice(&color);

            if x < 2 || y < 2 || x + 2 >= width || y + 2 >= height {
                pixels[idx..idx + 4].copy_from_slice(&accent);
            }
        }
    }

    egui::ColorImage::from_rgba_unmultiplied([width, height], &pixels)
}

pub fn texture_preview_image(texture: &Texture, gm2022_5: bool) -> Option<egui::ColorImage> {
    if !texture.present || texture.blobData.is_null() {
        return None;
    }

    let bytes = texture_bytes(texture)?;
    if bytes.is_empty() {
        return None;
    }

    let width = if texture.textureWidth > 0 {
        texture.textureWidth as usize
    } else {
        1
    };
    let height = if texture.textureHeight > 0 {
        texture.textureHeight as usize
    } else {
        1
    };

    let decoded = match decode_native_texture_rgba(bytes, gm2022_5) {
        Some(pixels) => pixels,
        None => return Some(fallback_texture_image(width, height)),
    };

    let expected = width.checked_mul(height)?.checked_mul(4)?;
    if decoded.len() != expected {
        return Some(fallback_texture_image(width, height));
    }

    Some(egui::ColorImage::from_rgba_unmultiplied([width, height], &decoded))
}

pub fn texture_page_item_preview_image(
    page_item: &TexturePageItem,
    textures: &[Texture],
    gm2022_5: bool,
) -> Option<egui::ColorImage> {
    let page_index = usize::try_from(page_item.texturePageId).ok()?;
    let texture = textures.get(page_index)?;
    let full_page = texture_preview_image(texture, gm2022_5)?;
    let [page_width, page_height] = full_page.size;

    let src_x = page_item.sourceX as usize;
    let src_y = page_item.sourceY as usize;
    let src_w = page_item.sourceWidth as usize;
    let src_h = page_item.sourceHeight as usize;

    if src_w == 0 || src_h == 0 || src_x >= page_width || src_y >= page_height {
        return Some(full_page);
    }

    let x0 = src_x.min(page_width.saturating_sub(1));
    let y0 = src_y.min(page_height.saturating_sub(1));
    let x1 = (x0 + src_w).min(page_width);
    let y1 = (y0 + src_h).min(page_height);
    let crop_w = x1.saturating_sub(x0);
    let crop_h = y1.saturating_sub(y0);

    let mut pixels = vec![0u8; crop_w * crop_h * 4];
    for y in 0..crop_h {
        let src_row = y0 + y;
        let dst_row = y * crop_w;
        for x in 0..crop_w {
            let src_index = src_row * page_width + (x0 + x);
            let dst_index = dst_row + x;
            let rgba = full_page.pixels[src_index].to_array();
            let dst_offset = dst_index * 4;
            pixels[dst_offset..dst_offset + 4].copy_from_slice(&rgba);
        }
    }

    Some(egui::ColorImage::from_rgba_unmultiplied([crop_w, crop_h], &pixels))
}
