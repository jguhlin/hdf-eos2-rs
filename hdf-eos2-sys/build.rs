extern crate bindgen;
extern crate cc;
extern crate hdf5_sys;

use std::env;
use std::path::PathBuf;

fn main() {
    // Get HDF5 include and lib directories from hdf5-sys crate
    let hdf5_include = env::var("DEP_HDF5_INCLUDE").expect("Set HDF5_INCLUDE_DIR environment variable to HDF5 include path");
    let hdf5_lib_dir = env::var("DEP_HDF5_ROOT").expect("Set HDF5_LIB_DIR environment variable to HDF5 library path");

    // Link to the correct HDF5 debug libraries
    let hdf5_lib = env::var("DEP_HDF5_LIBRARY").unwrap_or_else(|_| "hdf5".to_string());
    let hdf5_hl_lib = env::var("DEP_HDF5_HL_LIBRARY").unwrap_or_else(|_| "hdf5_hl".to_string());

    // Add search path for HDF5 library and link to the libraries
    println!("cargo:rustc-link-search=native={}", hdf5_lib_dir);
    println!("cargo:rustc-link-lib=static={}", hdf5_lib);  // hdf5_debug
    println!("cargo:rustc-link-lib=static={}", hdf5_hl_lib);  // hdf5_hl_debug
    println!("cargo:include={}", hdf5_include);

    cc::Build::new()
        // .file("hdf-eos5-2.0/gctp/src/gctp.c") // Example: Add more .c files here as needed
        .include("hdf-eos5-2.0/include")
        .include(hdf5_include.clone())  // Include HDF5 headers
        .static_flag(true)

        .file("hdf-eos5-2.0/src/EHapi.c")
        .file("hdf-eos5-2.0/src/GDapi.c")
        .file("hdf-eos5-2.0/src/PTapi.c")
        .file("hdf-eos5-2.0/src/SWapi.c")
        .file("hdf-eos5-2.0/src/TSapi.c")
        .file("hdf-eos5-2.0/src/ZAapi.c")
        .file("hdf-eos5-2.0/src/HE5common.c")
        .compile("hdf_eos5");

    // Step 2: Generate Rust bindings with bindgen
    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .clang_arg("-Ihdf-eos5-2.0/include") // Tell bindgen where to find the headers
        .clang_arg(format!("-I{}", hdf5_include)) // Tell bindgen where to find the HDF5 headers
        .blocklist_item("FP_NAN")
        .blocklist_item("FP_INFINITE")
        .blocklist_item("FP_ZERO")
        .blocklist_item("FP_SUBNORMAL")
        .blocklist_item("FP_NORMAL")
        .generate()
        .expect("Unable to generate bindings");

    // Step 3: Write the bindings to the $OUT_DIR
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");

    // Step 4: Link the compiled library to the Rust code
    println!("cargo:rustc-link-lib=static=hdf_eos5");
    
}
