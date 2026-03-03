#pragma once
#include <vector>
#include <cstdint>

namespace wcap {

    enum class CaptureType {
        Screen,
        Window,
        Any
    };

    struct Frame {
        int width;
        int height;
        std::vector<uint8_t> data;
    };

    class ScreenCapture {
    public:
        ScreenCapture();
        ~ScreenCapture();

        bool select_source(CaptureType type = CaptureType::Any);
        Frame get_frame();
        void stop();

    private:
        struct Impl;
        Impl* impl;
        CaptureType current_type = CaptureType::Any; 
    };

}