#pragma once

#ifdef LIB_PICO_CYW43_ARCH
#include "pico/cyw43_arch.h"
#endif

typedef enum __attribute__ ((__packed__)) {
    WIFI_SCAN_NOT_STARTED,
    WIFI_SCAN_IN_PROGRESS,
    WIFI_SCAN_COMPLETE,
    WIFI_SCAN_FAILED,
} wifi_scan_state_t;

typedef enum __attribute__ ((__packed__)) {
    WIFI_AUTH_UNKNOWN = 0,
    WIFI_AUTH_OPEN = 1,
    WIFI_AUTH_WPA_TKIP = 2,
    WIFI_AUTH_WPA2_AES = 3,
    WIFI_AUTH_WPA2_MIXED = 4,
} wifi_auth_t;

// Convert WIFI_AUTH enum to cyw43 auth constant
static inline uint32_t cyw43_auth_for(wifi_auth_t auth) {
    switch(auth) {
        case WIFI_AUTH_WPA_TKIP:
            return CYW43_AUTH_WPA_TKIP_PSK;
        case WIFI_AUTH_WPA2_AES:
            return CYW43_AUTH_WPA2_AES_PSK;
        case WIFI_AUTH_WPA2_MIXED:
            return CYW43_AUTH_WPA2_MIXED_PSK;
        default:
            return 0;
    }
}

// cyw43_ll.h doesn't define constants for these
#define CYW43_SCAN_AUTH_MODE_WEP_PSK    0b001
#define CYW43_SCAN_AUTH_MODE_WPA        0b010
#define CYW43_SCAN_AUTH_MODE_WPA2       0b100

// Convert cyw43 auth constant to WIFI_AUTH enum
static inline wifi_auth_t wifi_auth_for_auth_mode(uint8_t auth_mode) {
    if(auth_mode == 0) {
        return WIFI_AUTH_OPEN;
    }
    // Really just guessing at the mapping between these and the CYW43_AUTH_* constants
    switch(auth_mode & ~CYW43_SCAN_AUTH_MODE_WEP_PSK) {
        case CYW43_SCAN_AUTH_MODE_WPA:
            return WIFI_AUTH_WPA_TKIP;
        case CYW43_SCAN_AUTH_MODE_WPA2:
            return WIFI_AUTH_WPA2_AES;
        case CYW43_SCAN_AUTH_MODE_WPA | CYW43_SCAN_AUTH_MODE_WPA2:
            return WIFI_AUTH_WPA2_MIXED;
    }
    return WIFI_AUTH_UNKNOWN;

}

typedef struct WiFiScanItem WiFiScanItem;
struct WiFiScanItem {
    int index;
    char *ssid;
    int rssi;
    wifi_auth_t auth;
    WiFiScanItem *next;
};

typedef struct {
    wifi_scan_state_t state;
    int num_items;
    WiFiScanItem *first;
} WiFiScan;

// Current scan state
extern WiFiScan current_wifi_scan;

// Callback for wi-fi scan result
// updated_item will be NULL for WIFI_SCAN_COMPLETE and WIFI_SCAN_FAILED state
typedef void (*wifi_scan_cb)(WiFiScan *scan, WiFiScanItem *updated_item);

// Timeout to connect to wi-fi (milliseconds)
extern const uint32_t WIFI_CONNECT_TIMEOUT;

// Last wi-fi poll time, used to determine when next to poll
extern absolute_time_t last_wifi_poll_time;


// Initialize last wi-fi poll time
void wifi_init();

// Start a wi-fi scan
void wifi_scan(wifi_scan_cb cb);


static inline bool should_poll_wifi() {
#ifdef LIB_PICO_CYW43_ARCH
    absolute_time_t current = get_absolute_time();
    return absolute_time_diff_us(last_wifi_poll_time, current) > 1000;
#else
    return false;
#endif
}

static inline void poll_wifi() {
#ifdef LIB_PICO_CYW43_ARCH
    cyw43_arch_poll();
    last_wifi_poll_time = get_absolute_time();
#endif
}
