#include <stdbool.h>
#include <stdint.h>

#include <pico/time.h>

#include "wifi.h"

// Timeout to connect to wi-fi (milliseconds)
const uint32_t WIFI_CONNECT_TIMEOUT = 30000;

absolute_time_t last_wifi_poll_time;
static repeating_timer_t scan_state_poll_timer;

WiFiScan current_wifi_scan;

static void wifi_scan_free_items(WiFiScan *wifi_scan);
static WiFiScanItem *wifi_scan_upsert(WiFiScan *wifi_scan, const cyw43_ev_scan_result_t *result);

static int on_scan_result(void *env, const cyw43_ev_scan_result_t *result);
static bool on_poll_scan_state(repeating_timer_t *timer);


void wifi_init() {
    last_wifi_poll_time = nil_time;

    scan_state_poll_timer.alarm_id = 0;
    current_wifi_scan.state = WIFI_SCAN_NOT_STARTED;
    current_wifi_scan.num_items = 0;
    current_wifi_scan.first = NULL;
}

void wifi_scan(wifi_scan_cb cb) {
    // Cancel any existing polling we were doing on a previous scan
    if(scan_state_poll_timer.alarm_id > 0) {
        cancel_repeating_timer(&scan_state_poll_timer);
    }

    // Clear out results from any previous scan
    wifi_scan_free_items(&current_wifi_scan);

    // Begin scan
    cyw43_arch_enable_sta_mode();
    cyw43_wifi_scan_options_t scan_options = {0};
    int rc = cyw43_wifi_scan(&cyw43_state, &scan_options, cb, on_scan_result);
    if(rc < 0) {
        printf("Failed to start scan: %d\n", rc);
        current_wifi_scan.state = WIFI_SCAN_FAILED;
        cb(&current_wifi_scan, NULL);
        return;
    }

    printf("\nPerforming wifi scan\n");
    current_wifi_scan.state = WIFI_SCAN_IN_PROGRESS;
    cb(&current_wifi_scan, NULL);

    // Check in on the scan state periodically
    add_repeating_timer_ms(500, on_poll_scan_state, cb, &scan_state_poll_timer);
}


static int on_scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    wifi_scan_cb cb = (wifi_scan_cb)env;
    // Skip blank ssids
    if(result && result->ssid_len > 0) {
        WiFiScanItem *item = wifi_scan_upsert(&current_wifi_scan, result);
        cb(&current_wifi_scan, item);
    }
    return 0;
}

static bool on_poll_scan_state(repeating_timer_t *timer) {
    wifi_scan_cb cb = (wifi_scan_cb)timer->user_data;
    if(!cyw43_wifi_scan_active(&cyw43_state)) {
        current_wifi_scan.state = WIFI_SCAN_COMPLETE;
        cb(&current_wifi_scan, NULL);
        printf("Scan complete\n");
        return false;
    }
    return true;
}


static void wifi_scan_free_items(WiFiScan *wifi_scan) {
    WiFiScanItem *item = wifi_scan->first;
    while(item) {
        WiFiScanItem *next = item->next;
        free(item->ssid);
        free(item);
        item = next;
    }
    wifi_scan->num_items = 0;
    wifi_scan->first = NULL;
}

static WiFiScanItem *wifi_scan_upsert(WiFiScan *wifi_scan, const cyw43_ev_scan_result_t *result) {
    // Check for an existing item
    WiFiScanItem **next_ptr = &wifi_scan->first;
    WiFiScanItem *result_item = NULL;
    WiFiScanItem *curr;
    while((curr = *next_ptr)) {
        next_ptr = &curr->next;
        if(!strncmp(curr->ssid, (char *)result->ssid, result->ssid_len)) {
            printf("\nUpdating %s\n", curr->ssid);
            result_item = curr;
            break;
        }
    }

    // If none was found, create one
    if(!result_item) {
        result_item = malloc(sizeof(WiFiScanItem));
        result_item->ssid = strndup((char *)result->ssid, result->ssid_len);
        printf("\nInserting %s\n", result_item->ssid);
        result_item->index = wifi_scan->num_items;
        result_item->next = NULL;
        // Add it to the scan
        *next_ptr = result_item;
        wifi_scan->num_items++;
    }

    // Update the rssi and auth values
    result_item->rssi = result->rssi;
    result_item->auth = wifi_auth_for_auth_mode(result->auth_mode);
    return result_item;
}
