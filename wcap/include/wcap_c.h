#ifndef WCAP_C_H
#define WCAP_C_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct wcap_handle_t wcap_handle_t;
    typedef struct wcap_frame_t wcap_frame_t;

    typedef enum {
        WCAP_CAPTURE_SCREEN = 0,
        WCAP_CAPTURE_WINDOW = 1,
        WCAP_CAPTURE_ANY = 2
    } wcap_capture_type_t;

    struct wcap_frame_t {
        int32_t width;
        int32_t height;
        size_t data_len;
        uint8_t* data;
    };

    wcap_handle_t* wcap_create(void);
    void wcap_destroy(wcap_handle_t* handle);

    int wcap_select_source(wcap_handle_t* handle, wcap_capture_type_t type);
    wcap_frame_t wcap_get_frame(wcap_handle_t* handle);
    void wcap_frame_free(wcap_frame_t* frame);
    void wcap_stop(wcap_handle_t* handle);

#ifdef __cplusplus
}
#endif

#endif