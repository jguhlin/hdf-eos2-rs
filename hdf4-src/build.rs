use std::env;

fn feature_enabled(feature: &str) -> bool {
    env::var(format!("CARGO_FEATURE_{}", feature)).is_ok()
}

fn main() {
    println!("cargo:rerun-if-changed=build.rs");
    let mut cfg = cmake::Config::new("ext/");

    // only build the static c library, disable everything else
    cfg.define("HDF4_NO_PACKAGES", "ON");
    for option in &[
        "BUILD_SHARED_LIBS",
        "BUILD_TESTING",
        "HDF4_BUILD_TOOLS",
        "HDF4_BUILD_EXAMPLES",
        "HDF4_BUILD_JAVA",
        "HDF4_BUILD_FORTRAN",
        "HDF4_BUILD_CPP_LIB",
        "HDF4_BUILD_UTILS",
        "HDF4_ENABLE_PARALLEL",
    ] {
        cfg.define(option, "OFF");
    }

    // disable these by default, can be enabled via features
    for option in &[
        "HDF4_ENABLE_DEPRECATED_SYMBOLS",
        "HDF4_ENABLE_THREADSAFE",
        "ALLOW_UNSUPPORTED",
        "HDF4_BUILD_HL_LIB",
    ] {
        cfg.define(option, "OFF");
    }

    if feature_enabled("ZLIB") {
        let zlib_include_dir = env::var_os("DEP_Z_INCLUDE").unwrap();
        let mut zlib_header = env::split_paths(&zlib_include_dir).next().unwrap();
        zlib_header.push("zlib.h");
        let zlib_lib = "z";
        cfg.define("HDF4_ENABLE_Z_LIB_SUPPORT", "ON")
            .define("H5_ZLIB_HEADER", &zlib_header)
            .define("ZLIB_STATIC_LIBRARY", zlib_lib);
        println!("cargo:zlib_header={}", zlib_header.to_str().unwrap());
        println!("cargo:zlib={}", zlib_lib);
    }

    if feature_enabled("DEPRECATED") {
        cfg.define("HDF4_ENABLE_DEPRECATED_SYMBOLS", "ON");
    }

    if feature_enabled("THREADSAFE") {
        cfg.define("HDF4_ENABLE_THREADSAFE", "ON");
        if feature_enabled("HL") {
            println!("cargo:warning=Unsupported HDF4 options: hl with threadsafe.");
            cfg.define("ALLOW_UNSUPPORTED", "ON");
        }
    }

    let targeting_windows = env::var("CARGO_CFG_TARGET_OS").unwrap() == "windows";
    let debug_postfix = if targeting_windows { "_D" } else { "_debug" };

    if feature_enabled("HL") {
        cfg.define("HDF4_BUILD_HL_LIB", "ON");
        let mut HDF4_hl_lib =
            if cfg!(target_env = "msvc") { "libHDF4_hl" } else { "HDF4_hl" }.to_owned();
        if let Ok(opt_level) = env::var("OPT_LEVEL") {
            if opt_level == "0" {
                HDF4_hl_lib.push_str(debug_postfix);
            }
        }
        println!("cargo:hl_library={}", HDF4_hl_lib);
    }

    if cfg!(unix) && targeting_windows {
        let wine_exec =
            if env::var("CARGO_CFG_TARGET_ARCH").unwrap() == "x86_64" { "wine64" } else { "wine" };
        // when cross-compiling to windows, use Wine to run code generation programs
        cfg.define("CMAKE_CROSSCOMPILING_EMULATOR", wine_exec);
    }

    let dst = cfg.build();
    println!("cargo:root={}", dst.display());

    let HDF4_incdir = format!("{}/include", dst.display());
    println!("cargo:include={}", HDF4_incdir);

    let mut HDF4_lib = if cfg!(target_env = "msvc") { "libHDF4" } else { "HDF4" }.to_owned();
    if let Ok(opt_level) = env::var("OPT_LEVEL") {
        if opt_level == "0" {
            HDF4_lib.push_str(debug_postfix);
        }
    }
    println!("cargo:library={}", HDF4_lib);
}
