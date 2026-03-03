#include "wcap_c.h"
#include "wcap.hpp"
#include <cstring>
#include <cstdlib>

struct wcap_handle_t {
    wcap::ScreenCapture* capture;
};

extern "C" {

    wcap_handle_t* wcap_create(void) {
        wcap_handle_t* handle = new wcap_handle_t;
        handle->capture = new wcap::ScreenCapture();
        return handle;
    }

    void wcap_destroy(wcap_handle_t* handle) {
        if (handle) {
            delete handle->capture;
            delete handle;
        }
    }

    int wcap_select_source(wcap_handle_t* handle, wcap_capture_type_t type) {
        if (!handle || !handle->capture) return 0;

        wcap::CaptureType t = wcap::CaptureType::Any;
        if (type == WCAP_CAPTURE_SCREEN) t = wcap::CaptureType::Screen;
        else if (type == WCAP_CAPTURE_WINDOW) t = wcap::CaptureType::Window;

        return handle->capture->select_source(t) ? 1 : 0;
    }

    wcap_frame_t wcap_get_frame(wcap_handle_t* handle) {
        wcap_frame_t frame = {0, 0, 0, nullptr};
        if (!handle || !handle->capture) return frame;

        wcap::Frame f = handle->capture->get_frame();
        if (f.data.empty()) return frame;

        frame.width = f.width;
        frame.height = f.height;
        frame.data_len = f.data.size();

        frame.data = (uint8_t*)malloc(f.data.size());
        if (frame.data) {
            memcpy(frame.data, f.data.data(), f.data.size());
        }

        return frame;
    }

    void wcap_frame_free(wcap_frame_t* frame) {
        if (frame && frame->data) {
            free(frame->data);
            frame->data = nullptr;
            frame->data_len = 0;
        }
    }

    void wcap_stop(wcap_handle_t* handle) {
        if (handle && handle->capture) {
            handle->capture->stop();
        }
    }

}