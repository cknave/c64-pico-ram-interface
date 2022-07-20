#pragma once

#include <stdint.h>

#include "wifi.h"

// Pointer to 8k or 16k of RAM exposed to the C64 (call init_exposed_ram first)
extern uint8_t *exposed_ram;

// Initialize exposed RAM
void init_exposed_ram(unsigned int rom_size);


//
// NUFLI window: page through NUFLI image data
//

#define NUFLI_OFFSET (0x400)
#define NUFLI_WINDOW_SIZE (0x400)
#define exposed_nufli_page (exposed_ram + NUFLI_OFFSET)

// Wi-fi scan results, paginated items
#define WIFI_SCAN_RESULT_OFFSET (0x200)
#define WIFI_SCAN_RESULT_PAGE_SIZE (10)

extern unsigned int current_nufli_offset;

//
// Wi-fi scan result: scan state and paginated results
//

#pragma pack(push, 1)
struct ExposedWiFiScanPageEntry {
    wifi_auth_t auth;
    uint8_t bars;
    char ssid[33];  // null terminated
};
struct ExposedWiFiScanResult {
    wifi_scan_state_t scan_state;
    uint8_t current_page;
    uint8_t has_next_page;
    struct ExposedWiFiScanPageEntry items[WIFI_SCAN_RESULT_PAGE_SIZE];
};
#pragma pack(pop)
#define exposed_wifi_scan_result ((struct ExposedWiFiScanResult *)(exposed_ram + WIFI_SCAN_RESULT_OFFSET))

// Update the exposed wi-fi scan result data with this item
void set_wifi_scan_result_item(WiFiScanItem *item);