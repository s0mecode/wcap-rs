mod ffi;

use std::slice;

pub struct ScreenCapture {
    handle: *mut ffi::wcap_handle_t,
}

pub struct Frame {
    pub width: u32,
    pub height: u32,
    pub data: Vec<u8>,
}

impl Default for ScreenCapture {
    fn default() -> Self {
        Self::new()
    }
}

impl ScreenCapture {
    pub fn new() -> Self {
        // Initialize GStreamer once globally if needed
        // unsafe { gst::init().unwrap(); }
        // Since C++ code calls gst_init in constructor, we rely on that.

        let handle = unsafe { ffi::wcap_create() };
        ScreenCapture { handle }
    }

    pub fn select_source(&mut self, type_: CaptureType) -> bool {
        let c_type = match type_ {
            CaptureType::Screen => ffi::wcap_capture_type_t::WCAP_CAPTURE_SCREEN,
            CaptureType::Window => ffi::wcap_capture_type_t::WCAP_CAPTURE_WINDOW,
            CaptureType::Any => ffi::wcap_capture_type_t::WCAP_CAPTURE_ANY,
        };

        unsafe { ffi::wcap_select_source(self.handle, c_type) == 1 }
    }

    pub fn get_frame(&self) -> Option<Frame> {
        unsafe {
            let c_frame = ffi::wcap_get_frame(self.handle);
            if c_frame.data.is_null() || c_frame.data_len == 0 {
                return None;
            }

            let slice = slice::from_raw_parts(c_frame.data, c_frame.data_len);
            let mut vec = Vec::with_capacity(c_frame.data_len);
            vec.extend_from_slice(slice);

            // Save values before freeing
            let width = c_frame.width;
            let height = c_frame.height;

            // Free C memory
            let mut frame_to_free = c_frame;
            ffi::wcap_frame_free(&mut frame_to_free);

            Some(Frame {
                width: width as u32,
                height: height as u32,
                data: vec,
            })
        }
    }

    pub fn stop(&self) {
        unsafe { ffi::wcap_stop(self.handle) }
    }
}

impl Drop for ScreenCapture {
    fn drop(&mut self) {
        unsafe { ffi::wcap_destroy(self.handle) }
    }
}

pub enum CaptureType {
    Screen,
    Window,
    Any,
}
