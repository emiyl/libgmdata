fn main() {
    println!("cargo:rustc-link-search=native=../build/lib");
    println!("cargo:rustc-link-lib=static=gmdata");

    let bindings = bindgen::Builder::default()
        .header("../include/gmdata.h")
        .allowlist_function(".*")
        .allowlist_type(".*")
        .allowlist_var(".*")
        .raw_line("#![allow(non_snake_case)]")
        .raw_line("#![allow(non_camel_case_types)]")
        .raw_line("#![allow(non_upper_case_globals)]")
        .raw_line("#![allow(unused)]")
        .generate()
        .expect("Unable to generate bindings");

    bindings
        .write_to_file("src/bindings.rs")
        .expect("Couldn't write bindings");
}