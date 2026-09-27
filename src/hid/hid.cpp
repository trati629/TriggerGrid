// hid.cpp — see hid.h. HID over GATT with NimBLE-Arduino 2.x.

#include "hid.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include "report_map.h"

static NimBLEServer*         s_server   = nullptr;
static NimBLEHIDDevice*      s_hid      = nullptr;
static NimBLECharacteristic* s_keyboard = nullptr;
static NimBLECharacteristic* s_consumer = nullptr;

// Written by NimBLE's task, read by the actions task and loop().
static volatile bool    s_connected = false;
static volatile bool    s_encrypted = false;
static volatile uint8_t s_leds = 0;

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* server, NimBLEConnInfo& info) override {
        (void)server;
        s_connected = true;
        s_encrypted = info.isEncrypted();
        Serial.printf("ble: connected to %s\n", info.getAddress().toString().c_str());
    }

    void onDisconnect(NimBLEServer* server, NimBLEConnInfo& info, int reason) override {
        (void)server;
        (void)info;
        s_connected = false;
        s_encrypted = false;
        Serial.printf("ble: disconnected (reason %d), advertising again\n", reason);
        // NimBLE restarts advertising by itself (advertiseOnDisconnect).
    }

    void onAuthenticationComplete(NimBLEConnInfo& info) override {
        s_encrypted = info.isEncrypted();
        Serial.printf("ble: pairing %s (bonded: %s)\n",
                      s_encrypted ? "done" : "FAILED", info.isBonded() ? "yes" : "no");
    }
};

// The computer writes its keyboard LED state (NumLock, CapsLock, …) here.
class LedCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo& info) override {
        (void)info;
        NimBLEAttValue value = chr->getValue();
        if (value.size() > 0) {
            s_leds = value[0];
        }
    }
};

bool hid_init(const char* device_name) {
    if (!NimBLEDevice::init(device_name)) {
        return false;
    }
    // Bond so the computer reconnects after a power cycle. There is no
    // keyboard or display for a passkey, so pairing is "Just Works".
    NimBLEDevice::setSecurityAuth(true /* bond */, false /* MITM */, true /* secure connections */);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    s_server = NimBLEDevice::createServer();
    s_server->setCallbacks(new ServerCallbacks());
    s_server->advertiseOnDisconnect(true);

    s_hid = new NimBLEHIDDevice(s_server);
    s_hid->setManufacturer("TriggerGrid");
    s_hid->setPnp(0x02, 0x303A, 0x8001, 0x0100);   // USB-IF vendor source, Espressif VID
    s_hid->setHidInfo(0x00, 0x01);                 // no country code, remote wake
    s_hid->setReportMap((uint8_t*)kReportMap, sizeof(kReportMap));
    s_hid->setBatteryLevel(100);                   // USB powered: always full

    s_keyboard = s_hid->getInputReport(REPORT_ID_KEYBOARD);
    s_consumer = s_hid->getInputReport(REPORT_ID_CONSUMER);
    s_hid->getOutputReport(REPORT_ID_KEYBOARD)->setCallbacks(new LedCallbacks());

    s_server->start();

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->setAppearance(HID_KEYBOARD);
    adv->addServiceUUID(s_hid->getHidService()->getUUID());
    adv->setName(device_name);
    adv->enableScanResponse(true);
    return adv->start();
}

bool hid_connected() {
    return s_connected && s_encrypted;
}

bool hid_numlock_on() {
    return s_leds & 0x01;
}

void hid_send_keys(uint8_t modifiers, const uint8_t* keys, uint8_t count) {
    uint8_t report[8] = {modifiers, 0};
    for (uint8_t i = 0; i < count && i < 6; i++) {
        report[2 + i] = keys[i];
    }
    s_keyboard->setValue(report, sizeof(report));
    s_keyboard->notify();
}

void hid_release_keys() {
    hid_send_keys(0, nullptr, 0);
}

void hid_send_consumer(uint16_t usage) {
    uint8_t report[2] = {(uint8_t)(usage & 0xFF), (uint8_t)(usage >> 8)};
    s_consumer->setValue(report, sizeof(report));
    s_consumer->notify();
}

void hid_forget_bonds() {
    NimBLEDevice::deleteAllBonds();
    for (uint16_t handle : s_server->getPeerDevices()) {
        s_server->disconnect(handle);
    }
}
