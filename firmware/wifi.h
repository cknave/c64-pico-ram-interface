#pragma once

#ifdef LIB_PICO_CYW43_ARCH
#include "pico/cyw43_arch.h"
#endif

typedef enum {
    WIFI_SCAN_NOT_STARTED,
    WIFI_SCAN_IN_PROGRESS,
    WIFI_SCAN_COMPLETE,
    WIFI_SCAN_FAILED,
} wifi_scan_state_t;

typedef struct WiFiScanItem WiFiScanItem;
struct WiFiScanItem {
    int index;
    char *ssid;
    int rssi;
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
