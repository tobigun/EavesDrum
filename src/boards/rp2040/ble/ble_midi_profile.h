
// clang-format off
// auto-generated from .gatt file 

// att db format version 1

// binary attribute representation:
// - size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#include <stdint.h>

// Reference: https://en.cppreference.com/w/cpp/feature_test
#if __cplusplus >= 200704L
constexpr
#endif
const uint8_t profile_data[] =
{
    // ATT DB Version
    1,

    // 0x0001 PRIMARY_SERVICE-GAP_SERVICE
    0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18, 
    // 0x0002 CHARACTERISTIC-GAP_DEVICE_NAME - READ
    0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x02, 0x03, 0x00, 0x00, 0x2a, 
    // 0x0003 VALUE CHARACTERISTIC-GAP_DEVICE_NAME - READ -'BLE-MIDI2USBHUB'
    // READ_ANYBODY
    0x17, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x2a, 0x42, 0x4c, 0x45, 0x2d, 0x4d, 0x49, 0x44, 0x49, 0x32, 0x55, 0x53, 0x42, 0x48, 0x55, 0x42, 


    // #import <midi_service.gatt> -- BEGIN
    // Implementation of the MIDI Over Bluetooth Low Energy version 1.0 specification
    // https://midi.org/specifications/midi-transports-specifications/midi-over-bluetooth-low-energy-ble-midi
    //
    // MIDI Service
    // 0x0004 PRIMARY_SERVICE-03B80E5A-EDE8-4B33-A751-6CE34EC4C700
    0x18, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x28, 0x00, 0xc7, 0xc4, 0x4e, 0xe3, 0x6c, 0x51, 0xa7, 0x33, 0x4b, 0xe8, 0xed, 0x5a, 0x0e, 0xb8, 0x03, 
    // MIDI read, write_without_response and notify
    // 0x0005 CHARACTERISTIC-7772E5DB-3868-4112-A1A9-F2669D106BF3 - DYNAMIC | READ | WRITE_WITHOUT_RESPONSE | NOTIFY | ENCRYPTION_KEY_SIZE_16
    0x1b, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x28, 0x16, 0x06, 0x00, 0xf3, 0x6b, 0x10, 0x9d, 0x66, 0xf2, 0xa9, 0xa1, 0x12, 0x41, 0x68, 0x38, 0xdb, 0xe5, 0x72, 0x77, 
    // 0x0006 VALUE CHARACTERISTIC-7772E5DB-3868-4112-A1A9-F2669D106BF3 - DYNAMIC | READ | WRITE_WITHOUT_RESPONSE | NOTIFY | ENCRYPTION_KEY_SIZE_16
    // READ_ENCRYPTED, WRITE_ENCRYPTED, ENCRYPTION_KEY_SIZE=16
    0x16, 0x00, 0x07, 0xf7, 0x06, 0x00, 0xf3, 0x6b, 0x10, 0x9d, 0x66, 0xf2, 0xa9, 0xa1, 0x12, 0x41, 0x68, 0x38, 0xdb, 0xe5, 0x72, 0x77, 
    // 0x0007 CLIENT_CHARACTERISTIC_CONFIGURATION
    // READ_ANYBODY, WRITE_ENCRYPTED, ENCRYPTION_KEY_SIZE=16
    0x0a, 0x00, 0x0f, 0xf1, 0x07, 0x00, 0x02, 0x29, 0x00, 0x00, 
    // #import <midi_service.gatt> -- END
    // END
    0x00, 0x00, 
}; // total size 76 bytes 


//
// list service handle ranges
//
#define ATT_SERVICE_GAP_SERVICE_START_HANDLE 0x0001
#define ATT_SERVICE_GAP_SERVICE_END_HANDLE 0x0003
#define ATT_SERVICE_GAP_SERVICE_01_START_HANDLE 0x0001
#define ATT_SERVICE_GAP_SERVICE_01_END_HANDLE 0x0003
#define ATT_SERVICE_03B80E5A_EDE8_4B33_A751_6CE34EC4C700_START_HANDLE 0x0004
#define ATT_SERVICE_03B80E5A_EDE8_4B33_A751_6CE34EC4C700_END_HANDLE 0x0007
#define ATT_SERVICE_03B80E5A_EDE8_4B33_A751_6CE34EC4C700_01_START_HANDLE 0x0004
#define ATT_SERVICE_03B80E5A_EDE8_4B33_A751_6CE34EC4C700_01_END_HANDLE 0x0007

//
// list mapping between characteristics and handles
//
#define ATT_CHARACTERISTIC_GAP_DEVICE_NAME_01_VALUE_HANDLE 0x0003
#define ATT_CHARACTERISTIC_7772E5DB_3868_4112_A1A9_F2669D106BF3_01_VALUE_HANDLE 0x0006
#define ATT_CHARACTERISTIC_7772E5DB_3868_4112_A1A9_F2669D106BF3_01_CLIENT_CONFIGURATION_HANDLE 0x0007
