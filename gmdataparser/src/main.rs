mod app;
mod audio;
mod bindings;
mod models;
mod texture;

use app::create_app;

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

    let app = create_app(file_path);
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
