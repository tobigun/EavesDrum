#ifndef _WIIMOTE_BTSTACK_H_
#define _WIIMOTE_BTSTACK_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#include "wiimote.h"

typedef bool (*WiimoteReportCallback)(WiimoteReport* report);
typedef void (*WiimoteLedCallback)(bool led_on);
typedef void (*WiimoteConnectionCallback)(wii_addr_t wii_addr, wii_link_key_t link_key);

struct WiimoteConfig {
    wii_addr_t wii_addr;
    wii_link_key_t wii_link_key;

    bool has_speaker; // if speaker is enabled, audio data might be sent which takes some CPU time to process
    bool has_wmp; // wii motion plus

    WiimoteReportCallback report_callback;
    WiimoteLedCallback set_led_state_callback;
    WiimoteConnectionCallback connection_callback;
};

void wiimote_emulator_init(struct WiimoteConfig* config);
void wiimote_emulator_deinit();


typedef enum PairingType {
    PAIRING_SYNC, // sync button pressed
    PAIRING_GUEST // 1 + 2 buttons pressed
} PairingType_t;

void wiimote_emulator_pair_remote(PairingType_t type);

#if defined(__cplusplus)
}
#endif

#endif // _WIIMOTE_BTSTACK_H_
