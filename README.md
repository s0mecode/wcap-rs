# wcap-rs

Rust bindings for `libwcap`, a C++ library that captures the screen, windows, or GStreamer-compatible sources on Linux using the freedesktop.org ScreenCast portal and GStreamer.

## Features

* Capture the whole screen, a single window, or any GStreamer source
* Retrieve frames as BGR data (3 bytes per pixel)
* Pure Rust FFI bindings with safe abstractions
* Example program demonstrating usage

## Requirements

* Rust toolchain (edition 2021)
* C++17 compiler (clang, gcc)
* GStreamer 1.0 development packages
* GStreamer App, Video, Gio, and GLib development packages
* pkg-config
* CMake (optional, for C++ build integration)

## Building

```bash
cargo build --release
```

The build compiles the underlying C++ library and creates Rust bindings.

## Usage

```rust
use wcap_rs::{CaptureType, ScreenCapture};

let mut cap = ScreenCapture::new();
if cap.select_source(CaptureType::Screen) {
    if let Some(frame) = cap.get_frame() {
        println!("Captured {}x{} frame", frame.width, frame.height);
    }
    cap.stop();
}
```

## Example

Run the included example:

```bash
cargo run --example test_capture
```

The example captures 100 frames and saves the last one as `snapshot.png`.

## API

```rust
pub struct ScreenCapture {
    // Opaque handle to the C++ capture object
}

pub struct Frame {
    pub width: u32,
    pub height: u32,
    pub data: Vec<u8>, // BGR format, 3 bytes per pixel
}

pub enum CaptureType {
    Screen,  // Capture entire screen
    Window,  // Capture a single window
    Any,     // Let user select any source
}

impl ScreenCapture {
    pub fn new() -> Self;
    pub fn select_source(&mut self, type_: CaptureType) -> bool;
    pub fn get_frame(&self) -> Option<Frame>;
    pub fn stop(&self);
}
```

## License

Apache-2.0 - see [License](LICENSE) for details.