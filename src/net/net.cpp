// net.cpp — see net.h. Design: docs/architecture.md, "Networking".

#include "net.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>

constexpr uint32_t CONNECT_TIMEOUT_MS = 10000;

static NetMode  s_mode = NetMode::Connecting;
static uint32_t s_started_ms = 0;
static bool     s_mdns_started = false;

static char s_ssid[33];        // saved network, or the hotspot name
static char s_password[65];    // saved network password
static char s_ap_password[9];  // 8 characters + terminator
static char s_hostname[33];

// "Desk Pad!" → "desk-pad": lower case, spaces to '-', only [a-z0-9-].
static void make_hostname(const char* name, char* out, size_t size) {
    size_t n = 0;
    for (const char* c = name; *c && n + 1 < size; c++) {
        char ch = *c;
        if (ch >= 'A' && ch <= 'Z') ch = ch - 'A' + 'a';
        if (ch == ' ') ch = '-';
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-') {
            out[n++] = ch;
        }
    }
    out[n] = '\0';
    if (n == 0) {
        strncpy(out, "triggergrid", size);
    }
}

// The hotspot password is random, made on first boot and kept in NVS. No
// 0/O or 1/l/I, so it's easy to read off the screen.
static void load_ap_password(Preferences& prefs) {
    String saved = prefs.getString("ap_pass", "");
    if (saved.length() == 8) {
        strncpy(s_ap_password, saved.c_str(), sizeof(s_ap_password));
        return;
    }
    static const char kChars[] = "abcdefghjkmnpqrstuvwxyz23456789";
    for (int i = 0; i < 8; i++) {
        s_ap_password[i] = kChars[esp_random() % (sizeof(kChars) - 1)];
    }
    s_ap_password[8] = '\0';
    prefs.putString("ap_pass", s_ap_password);
}

static void start_mdns() {
    if (!s_mdns_started && MDNS.begin(s_hostname)) {
        MDNS.addService("http", "tcp", 80);
        s_mdns_started = true;
    }
}

static void start_hotspot() {
    if (s_mode == NetMode::Connecting && WiFi.getMode() != WIFI_OFF) {
        WiFi.disconnect(true);   // stop trying the saved network
    }
    WiFi.mode(WIFI_AP);

    // TriggerGrid-XXXX: the last two bytes of the MAC tell pads apart. Read
    // after WiFi.mode(), once the radio has its address.
    uint8_t mac[6];
    WiFi.softAPmacAddress(mac);
    snprintf(s_ssid, sizeof(s_ssid), "TriggerGrid-%02X%02X", mac[4], mac[5]);

    WiFi.softAP(s_ssid, s_ap_password);   // WPA2: the hotspot is never open
    s_mode = NetMode::AccessPoint;
    start_mdns();
    Serial.printf("net: setup hotspot \"%s\", password %s, http://%s\n",
                  s_ssid, s_ap_password, WiFi.softAPIP().toString().c_str());
}

void net_begin(const char* device_name) {
    make_hostname(device_name, s_hostname, sizeof(s_hostname));

    Preferences prefs;
    prefs.begin("net", false);
    load_ap_password(prefs);
    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("pass", "");
    prefs.end();

    WiFi.setHostname(s_hostname);
    if (ssid.isEmpty()) {
        Serial.println("net: no saved network");
        start_hotspot();
        return;
    }

    strncpy(s_ssid, ssid.c_str(), sizeof(s_ssid) - 1);
    strncpy(s_password, password.c_str(), sizeof(s_password) - 1);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(s_ssid, s_password);
    s_mode = NetMode::Connecting;
    s_started_ms = millis();
    Serial.printf("net: joining \"%s\"\n", s_ssid);
}

void net_update() {
    if (s_mode != NetMode::Connecting) {
        return;
    }
    if (WiFi.status() == WL_CONNECTED) {
        s_mode = NetMode::Station;
        start_mdns();
        Serial.printf("net: connected, http://%s.local (%s)\n",
                      s_hostname, WiFi.localIP().toString().c_str());
    } else if (millis() - s_started_ms > CONNECT_TIMEOUT_MS) {
        Serial.println("net: could not join the saved network");
        start_hotspot();
    }
}

NetMode net_mode() {
    return s_mode;
}

bool net_connected() {
    return s_mode == NetMode::Station && WiFi.status() == WL_CONNECTED;
}

const char* net_ssid() {
    return s_ssid;
}

const char* net_ap_password() {
    return s_ap_password;
}

const char* net_hostname() {
    return s_hostname;
}

void net_ip(char* out, size_t size) {
    IPAddress ip;
    if (s_mode == NetMode::AccessPoint) {
        ip = WiFi.softAPIP();
    } else if (net_connected()) {
        ip = WiFi.localIP();
    } else {
        out[0] = '\0';
        return;
    }
    snprintf(out, size, "%s", ip.toString().c_str());
}

int net_rssi() {
    return net_connected() ? WiFi.RSSI() : 0;
}

void net_save_credentials(const char* ssid, const char* password) {
    Preferences prefs;
    prefs.begin("net", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    prefs.end();
}

void net_force_setup_hotspot() {
    if (s_mode != NetMode::AccessPoint) {
        start_hotspot();
    }
}

void net_scan_start() {
    // In hotspot mode the radio must also be a station to scan.
    if (s_mode == NetMode::AccessPoint) {
        WiFi.mode(WIFI_AP_STA);
    }
    WiFi.scanNetworks(true /* async */);
}

int net_scan_count() {
    int n = WiFi.scanComplete();
    return n == WIFI_SCAN_RUNNING ? -1 : (n < 0 ? 0 : n);
}

bool net_scan_entry(int index, char* ssid, size_t ssid_size, int* rssi, bool* secure) {
    if (index < 0 || index >= net_scan_count()) {
        return false;
    }
    snprintf(ssid, ssid_size, "%s", WiFi.SSID(index).c_str());
    *rssi = WiFi.RSSI(index);
    *secure = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
    return true;
}
