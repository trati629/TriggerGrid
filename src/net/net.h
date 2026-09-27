// net.h — Wi-Fi: join the saved network, or run a setup hotspot.
//
// With saved credentials the pad joins that network (station mode) and is
// reachable as http://<name>.local. If there are none, or joining takes more
// than 10 s, it starts its own WPA2 hotspot "TriggerGrid-XXXX" instead.
// Credentials and the hotspot password live in NVS, never in config.json.
#pragma once

#include <stddef.h>
#include <stdint.h>

enum class NetMode {
    Connecting,    // trying the saved network
    Station,       // on the saved network (may be temporarily disconnected)
    AccessPoint,   // running the setup hotspot
};

// Start Wi-Fi. `device_name` becomes the host name ("Desk Pad" → desk-pad).
void net_begin(const char* device_name);

// Call from loop(): handles the 10 s timeout and starts mDNS once connected.
void net_update();

NetMode     net_mode();
bool        net_connected();       // station mode and associated
const char* net_ssid();            // saved network, or the hotspot name
const char* net_ap_password();     // hotspot password
const char* net_hostname();        // e.g. "triggergrid"
void        net_ip(char* out, size_t size);   // "192.168.1.42", or "" if none
int         net_rssi();            // dBm, station mode only

// Save a network (NVS). Takes effect after a reboot.
void net_save_credentials(const char* ssid, const char* password);

// Drop to the setup hotspot until the next reboot (device menu, or holding
// the status bar for 3 s).
void net_force_setup_hotspot();

// Asynchronous network scan for the web page: start it, then read the
// results when they're ready. net_scan_count() is -1 while scanning.
void net_scan_start();
int  net_scan_count();
bool net_scan_entry(int index, char* ssid, size_t ssid_size, int* rssi, bool* secure);
