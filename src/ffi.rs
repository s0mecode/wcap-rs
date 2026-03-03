use std::os::raw::{c_int, c_uchar};

#[repr(C)]
pub struct wcap_handle_t {
    _private: [u8; 0],
}

#[repr(C)]
pub struct wcap_frame_t {
    pub width: i32,
    pub height: i32,
    pub data_len: usize,
    pub data: *mut c_uchar,
}

#[repr(C)]
#[allow(non_camel_case_types)]
pub enum wcap_capture_type_t {
    WCAP_CAPTURE_SCREEN = 0,
    WCAP_CAPTURE_WINDOW = 1,
    WCAP_CAPTURE_ANY = 2,
}

extern "C" {
    pub fn wcap_create() -> *mut wcap_handle_t;
    pub fn wcap_destroy(handle: *mut wcap_handle_t);
    pub fn wcap_select_source(handle: *mut wcap_handle_t, type_: wcap_capture_type_t) -> c_int;
    pub fn wcap_get_frame(handle: *mut wcap_handle_t) -> wcap_frame_t;
    pub fn wcap_frame_free(frame: *mut wcap_frame_t);
    pub fn wcap_stop(handle: *mut wcap_handle_t);
}
