//
// Created by kvance on 7/19/22.
//

#include <malloc.h>
#include <string.h>

#include "exposed_ram.h"
#include "loader_rom.h"
#include "raspi.h"

uint8_t *exposed_ram;
unsigned int current_nufli_offset;

void init_exposed_ram(unsigned int rom_size) {
    // Data exposed by the ROM window must be aligned by 16 kbytes so we can use the least
    // significant bits of its address for A0-A13
    exposed_ram = memalign(16384, rom_size);

    // Initialize the ROM with our loader code
    memcpy(exposed_ram, loader_rom, sizeof(loader_rom));

    // Initialize the NUFLI area of our ROM with the first 1KB
    memcpy(exposed_nufli_page, raspi, NUFLI_WINDOW_SIZE);
    current_nufli_offset = 0;

    // Clear the wi-fi scan result area
    memset(exposed_wifi_scan_result, 0, sizeof(*exposed_wifi_scan_result));
}

void set_wifi_scan_result_item(WiFiScanItem *item) {
    uint page_start = exposed_wifi_scan_result->current_page * WIFI_SCAN_RESULT_PAGE_SIZE;
    uint page_end = page_start + WIFI_SCAN_RESULT_PAGE_SIZE - 1;
    if(item->index < page_start || item->index > page_end) {
        return;
    }

    uint page_index = item->index % WIFI_SCAN_RESULT_PAGE_SIZE;
    struct ExposedWiFiScanPageEntry *entry = &exposed_wifi_scan_result->items[page_index];
    strcpy(entry->ssid, item->ssid);
    entry->auth = item->auth;

    // From this table: https://www.metageek.com/training/resources/understanding-rssi/
    if(item->rssi < -90) {
        entry->bars = 0;
    } else if(item->rssi < -80) {
        entry->bars = 1;
    } else if(item->rssi < -70) {
        entry->bars = 2;
    } else if(item->rssi < -67) {
        entry->bars = 3;
    } else {
        entry->bars = 4;
    }
}