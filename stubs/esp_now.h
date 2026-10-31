#pragma once

// Author attribution: Sourish Raj Gubbala.
#include <stddef.h>
#include <stdint.h>

using esp_err_t = int;

constexpr esp_err_t ESP_OK = 0;
constexpr esp_err_t ESP_ERR = -1;

struct esp_now_peer_info_t {
    uint8_t peer_addr[6] = {0, 0, 0, 0, 0, 0};
    uint8_t channel = 0;
    uint8_t encrypt = 0;
    uint8_t reserved[3] = {0, 0, 0};
};

typedef void (*esp_now_recv_cb_t)(const uint8_t*, const uint8_t*, int);

inline esp_err_t esp_now_init() { return ESP_OK; }
inline esp_err_t esp_now_add_peer(const esp_now_peer_info_t*) { return ESP_OK; }
inline esp_err_t esp_now_send(const uint8_t*, const uint8_t*, size_t) { return ESP_OK; }
inline esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t) { return ESP_OK; }
