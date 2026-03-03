#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include <gio/gio.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include "wcap.hpp"

namespace wcap {

class PortalHelper {
    GDBusConnection *conn;
    std::string session_handle;
    std::string request_path;
    bool finished = false;
    uint32_t node_id = 0;

    static void on_signal(GDBusConnection*, const gchar*, const gchar* path, const gchar*, const gchar*, GVariant* params, gpointer user_data) {
        auto* self = (PortalHelper*)user_data;
        if (self->request_path != path) return;

        uint32_t response; GVariant* results;
        g_variant_get(params, "(u@a{sv})", &response, &results);

        if (response == 0) {
            GVariant* v_streams = g_variant_lookup_value(results, "streams", G_VARIANT_TYPE_ARRAY);
            if (v_streams) {
                GVariantIter iter; g_variant_iter_init(&iter, v_streams);
                uint32_t id; GVariant* opts;
                if (g_variant_iter_next(&iter, "(u@a{sv})", &id, &opts)) self->node_id = id;
                g_variant_unref(v_streams);
            }
            GVariant* v_handle = g_variant_lookup_value(results, "session_handle", G_VARIANT_TYPE_STRING);
            if (v_handle) {
                self->session_handle = g_variant_get_string(v_handle, NULL);
                g_variant_unref(v_handle);
            }
        }
        self->finished = true;
        g_variant_unref(results);
    }

    bool call(const char* method, GVariant* params) {
        finished = false;
        GError* error = nullptr;
        GVariant* ret = g_dbus_connection_call_sync(conn, 
            "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", 
            "org.freedesktop.portal.ScreenCast", method, params, 
            NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        
        if (error) { g_error_free(error); return false; }
        const char* path; g_variant_get(ret, "(o)", &path);
        request_path = path; g_variant_unref(ret);
        
        while (!finished) g_main_context_iteration(NULL, TRUE);
        return true;
    }

    GVariant* opts(const std::string& base, int step, CaptureType type) {
        GVariantBuilder b; g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
        std::string t = base + "_" + std::to_string(step);
        g_variant_builder_add(&b, "{sv}", "handle_token", g_variant_new_string(t.c_str()));
        if (step == 1) g_variant_builder_add(&b, "{sv}", "session_handle_token", g_variant_new_string((base + "_s").c_str()));
        else if (step == 2) { 
            g_variant_builder_add(&b, "{sv}", "multiple", g_variant_new_boolean(FALSE));
            
            uint32_t type_flag = 3; 
            if (type == CaptureType::Screen) type_flag = 1;
            else if (type == CaptureType::Window) type_flag = 2;

            g_variant_builder_add(&b, "{sv}", "types", g_variant_new_uint32(type_flag)); 
            g_variant_builder_add(&b, "{sv}", "cursor_mode", g_variant_new_uint32(2)); 
        }
        return g_variant_builder_end(&b);
    }

public:
    PortalHelper() {
        conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
        g_dbus_connection_signal_subscribe(conn, "org.freedesktop.portal.Desktop", "org.freedesktop.portal.Request", "Response", NULL, NULL, G_DBUS_SIGNAL_FLAGS_NONE, on_signal, this, NULL);
    }

    uint32_t get_id(CaptureType type) {
        std::string t = "u" + std::to_string(getpid());
        if (!call("CreateSession", g_variant_new("(@a{sv})", opts(t, 1, type)))) return 0;
        if (!call("SelectSources", g_variant_new("(o@a{sv})", session_handle.c_str(), opts(t, 2, type)))) return 0;
        if (!call("Start", g_variant_new("(os@a{sv})", session_handle.c_str(), "", opts(t, 3, type)))) return 0;
        return node_id;
    }
};

struct ScreenCapture::Impl {
    int fps;
    GstElement *pipeline = nullptr;
    GstElement *appsink = nullptr;
    std::vector<uint8_t> frame_buffer;

    Impl(int fps) : fps(fps) {
        gst_init(nullptr, nullptr);
    }

    ~Impl() {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
        }
    }
};

ScreenCapture::ScreenCapture() : impl(new Impl(0)) {}
ScreenCapture::~ScreenCapture() { delete impl; }

bool ScreenCapture::select_source(CaptureType type) {
    current_type = type;

    PortalHelper portal;
    uint32_t id = portal.get_id(type);
    if (id == 0) return false;


    std::string desc =
        "pipewiresrc path=" + std::to_string(id) + " do-timestamp=true ! "
        "videoconvert ! "
        "video/x-raw,format=BGRx ! "
        "appsink name=sink drop=true sync=false max-buffers=1";

    GError* err = nullptr;
    impl->pipeline = gst_parse_launch(desc.c_str(), &err);
    if (err || !impl->pipeline) {
        if (err) g_error_free(err);
        return false;
    }

    impl->appsink = gst_bin_get_by_name(GST_BIN(impl->pipeline), "sink");
    gst_element_set_state(impl->pipeline, GST_STATE_PLAYING);
    return true;
}

static void detect_bounds(const uint8_t* data, int width, int height, int stride, 
                          int& x, int& y, int& w, int& h) {
    int min_y = 0, max_y = height - 1, min_x = 0, max_x = width - 1;
    bool found = false;
    for (; min_y < height; ++min_y) {
        const uint8_t* p = data + (min_y * stride);
        for (int i = 0; i < width; ++i, p += 4) if (p[0] | p[1] | p[2]) { found = true; break; }
        if (found) break;
    }
    if (!found) { x = 0; y = 0; w = width; h = height; return; }
    for (; max_y >= min_y; --max_y) {
        const uint8_t* p = data + (max_y * stride);
        int i = 0; for (; i < width; ++i, p += 4) if (p[0] | p[1] | p[2]) break;
        if (i < width) break;
    }
    found = false;
    for (; min_x < width; ++min_x) {
        for (int k = min_y; k <= max_y; ++k) {
            const uint8_t* px = data + (k * stride) + (min_x * 4);
            if (px[0] | px[1] | px[2]) { found = true; break; }
        }
        if (found) break;
    }
    for (; max_x >= min_x; --max_x) {
        bool row_has_pixel = false;
        for (int k = min_y; k <= max_y; ++k) {
            const uint8_t* px = data + (k * stride) + (max_x * 4);
            if (px[0] | px[1] | px[2]) { row_has_pixel = true; break; }
        }
        if (row_has_pixel) break;
    }
    x = min_x; y = min_y; w = (max_x - min_x) + 1; h = (max_y - min_y) + 1;
    if (w <= 0 || h <= 0) { x = 0; y = 0; w = width; h = height; }
} 

Frame ScreenCapture::get_frame() {
    Frame f = {0, 0, {}};
    if (!impl->appsink) return f;

    
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(impl->appsink));
    
    
    if (!sample) return f;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstCaps* caps = gst_sample_get_caps(sample);
    GstVideoInfo info;
    
    if (!gst_video_info_from_caps(&info, caps)) {
        gst_sample_unref(sample);
        return f;
    }

    int raw_width = GST_VIDEO_INFO_WIDTH(&info);
    int raw_height = GST_VIDEO_INFO_HEIGHT(&info);
    int stride = GST_VIDEO_INFO_PLANE_STRIDE(&info, 0); 

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        int cx = 0, cy = 0, cw = raw_width, ch = raw_height;
        GstVideoCropMeta* meta = gst_buffer_get_video_crop_meta(buffer);
        if (meta) {
            cx = meta->x; cy = meta->y; cw = meta->width; ch = meta->height;
        } else if (current_type != CaptureType::Screen) {
            detect_bounds(map.data, raw_width, raw_height, stride, cx, cy, cw, ch);
        }
        f.width = cw;
        f.height = ch;
        size_t needed = static_cast<size_t>(cw) * ch * 3;
        if (impl->frame_buffer.size() < needed) {
            impl->frame_buffer.resize(needed);
        }
        uint8_t* dst_ptr = impl->frame_buffer.data();
        const uint8_t* src_ptr = map.data;
        for (int i = 0; i < ch; ++i) {
            const uint8_t* row_src = src_ptr + ((cy + i) * stride) + (cx * 4);
            uint8_t* row_dst = dst_ptr + (i * cw * 3);
            const uint8_t* sp = row_src;
            uint8_t* dp = row_dst;
            for (int j = 0; j < cw; ++j, sp += 4, dp += 3) {
                dp[0] = sp[0]; dp[1] = sp[1]; dp[2] = sp[2];
            }
        }
        if (impl->frame_buffer.size() == needed) {
            f.data = std::move(impl->frame_buffer);
        } else {
            f.data.assign(impl->frame_buffer.begin(), impl->frame_buffer.begin() + needed);
        }
        gst_buffer_unmap(buffer, &map);
    }
    
    gst_sample_unref(sample);
    return f;
}

void ScreenCapture::stop() {
    if (impl->pipeline) gst_element_set_state(impl->pipeline, GST_STATE_NULL);
}

}