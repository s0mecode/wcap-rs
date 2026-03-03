use std::path::PathBuf;

fn main() {
    // Probe GStreamer dependencies
    let gstreamer = pkg_config::Config::new()
        .atleast_version("1.0")
        .probe("gstreamer-1.0")
        .expect("gstreamer-1.0 not found");

    let gstreamer_app = pkg_config::Config::new()
        .atleast_version("1.0")
        .probe("gstreamer-app-1.0")
        .expect("gstreamer-app-1.0 not found");

    let gstreamer_video = pkg_config::Config::new()
        .atleast_version("1.0")
        .probe("gstreamer-video-1.0")
        .expect("gstreamer-video-1.0 not found");

    let gio = pkg_config::Config::new()
        .atleast_version("2.0")
        .probe("gio-2.0")
        .expect("gio-2.0 not found");

    let glib = pkg_config::Config::new()
        .atleast_version("2.0")
        .probe("glib-2.0")
        .expect("glib-2.0 not found");

    // Collect include paths
    let mut include_paths: Vec<PathBuf> = Vec::new();
    include_paths.push(PathBuf::from("wcap/include"));
    include_paths.extend(gstreamer.include_paths.clone());
    include_paths.extend(gstreamer_app.include_paths.clone());
    include_paths.extend(gstreamer_video.include_paths.clone());
    include_paths.extend(gio.include_paths.clone());
    include_paths.extend(glib.include_paths.clone());

    // Compile C++ sources
    let mut build = cc::Build::new();
    build
        .cpp(true)
        .file("wcap/src/wcap.cpp")
        .file("wcap/src/wcap_c.cpp")
        .flag("-std=c++17")
        .flag("-O3");

    for path in &include_paths {
        build.include(path);
    }

    build.compile("wcap");

    // Link C++ standard library (CRITICAL)
    println!("cargo:rustc-link-lib=stdc++");

    // Tell cargo to rerun if sources change
    println!("cargo:rerun-if-changed=wcap/src/wcap.cpp");
    println!("cargo:rerun-if-changed=wcap/src/wcap_c.cpp");
    println!("cargo:rerun-if-changed=wcap/include/wcap.hpp");
    println!("cargo:rerun-if-changed=wcap/include/wcap_c.h");
}