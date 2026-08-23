mod app;
mod bindings;
mod models;
mod texture;

use std::ffi::CString;

use app::create_app;
use bindings::*;

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
    }

    let app = create_app(file_path, version, dw);
    let options = eframe::NativeOptions {
        viewport: eframe::egui::ViewportBuilder::default().with_inner_size([600.0, 400.0]),
        ..Default::default()
    };

    let run_result = eframe::run_native(
        "gmdataparser",
        options,
        Box::new(move |_cc| Ok(Box::new(app))),
    );

    run_result.expect("Failed to start GUI");
}
