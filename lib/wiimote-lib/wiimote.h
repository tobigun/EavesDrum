#ifndef WIIMOTE_H
#define WIIMOTE_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "wm_crypto.h"

enum wiimote_connected_extension_type
{
  Nunchuk = 0x0,
  Classic = 0x1,
  BalanceBoard = 0x2,
  GuitarHeroDrum = 0x3,
  NoExtension = 0xff
};

struct wiimote_ir_object
{
  uint16_t x;
  uint16_t y;
  uint8_t size;
  uint8_t xmin;
  uint8_t ymin;
  uint8_t xmax;
  uint8_t ymax;
  uint8_t intensity;
};

struct wiimote_nunchuk
{
  uint16_t accel_x;
  uint16_t accel_y;
  uint16_t accel_z;
  uint8_t x;
  uint8_t y;
  bool c;
  bool z;
};

struct wiimote_classic
{
  bool a; // GH drum: green
  bool b; // GH drum: red
  bool x; // GH drum: blue
  bool y; // GH drum: yellow
  bool minus;
  bool plus;
  bool rtrigger;
  bool ltrigger;
  bool home;
  bool rz; // GH drum: kick
  bool lz; // GH drum: orange
  bool up;
  bool down;
  bool left;
  bool right;
  uint8_t ls_x; // GH drum: stick x
  uint8_t ls_y; // GH drum: stick y
  uint8_t rs_x;
  uint8_t rs_y;
  uint8_t lt;
  uint8_t rt;

};

struct wiimote_motionplus
{
  uint16_t yaw_down;
  uint16_t roll_left;
  uint16_t pitch_left;
  bool yaw_slow;
  bool roll_slow;
  bool pitch_slow;
};

struct wiimote_midi
{
  uint8_t note;
  uint8_t velocity;
  uint8_t channel; // 0: midi ch 1 (also used for no hit), 9: midi ch 10
};

#define WIIMOTE_BUTTONS \
  bool a; \
  bool b; \
  bool minus; \
  bool plus; \
  bool home; \
  bool one; \
  bool two; \
  bool up; \
  bool down; \
  bool left; \
  bool right; \
  bool sync; \
  bool power;

struct wiimote_buttons_only{
  WIIMOTE_BUTTONS
};

struct wiimote_state_usr
{
  WIIMOTE_BUTTONS

  //accelerometer (10 bit range)
  //0 acceleration is approximately 0x200
  uint16_t accel_x;
  uint16_t accel_y;
  uint16_t accel_z;

  struct wiimote_ir_object ir_object[4];

  enum wiimote_connected_extension_type connected_extension_type;

  struct wiimote_nunchuk nunchuk;
  struct wiimote_classic classic;
  struct wiimote_motionplus motionplus;
  struct wiimote_midi midi;
};

void reset_ir_object(struct wiimote_ir_object * object);
void reset_input_ir(struct wiimote_ir_object ir_object[4]);
void reset_input_nunchuk(struct wiimote_nunchuk * nunchuk);
void reset_input_classic(struct wiimote_classic * classic);
void reset_input_motionplus(struct wiimote_motionplus * motionplus);

struct wiimote_state_sys
{
  bool led_1;
  bool led_2;
  bool led_3;
  bool led_4;

  bool rumble;

  bool ircam_enabled;
  bool speaker_enabled;
  bool has_speaker;

  uint8_t battery_level;
  bool low_battery;

  int extension_hotplug_timer;
  bool extension_connected;
  enum wiimote_connected_extension_type connected_extension_type;
  bool wmp_connected; // wii motion plus

  struct ext_crypto_state extension_crypto_state;
  bool extension_report;
  bool extension_encrypted;
  uint8_t extension_report_type;
  uint8_t extension_type; // unused
  uint8_t wmp_state; //0 inactive, 1 active, 2 deactivated

  uint8_t reporting_mode;
  bool reporting_continuous;

  struct queued_report * queue;
  struct queued_report * queue_end;

  uint8_t register_a2[10]; //speaker
  uint8_t register_a4[256]; //extension
  uint8_t register_a6[256]; //wii motion plus
  uint8_t register_b0[52]; //ir camera
};

struct wiimote_state
{
  struct wiimote_state_sys sys;
  struct wiimote_state_usr usr;
};

//new adding -> only buttons struture for wiimote
struct wiimote_buttons{
  WIIMOTE_BUTTONS

  //accelerometer (10 bit range)
  //0 acceleration is approximately 0x200
  uint16_t accel_x;
  uint16_t accel_y;
  uint16_t accel_z;

  //ir
  int8_t ir_x;
  int8_t ir_y;
};

typedef uint8_t wii_addr_t[6];
typedef uint8_t wii_link_key_t[16];

//new adding -> wiimote + classic
typedef struct{
  struct wiimote_buttons wiimote;
  struct wiimote_nunchuk nunchuk;
  struct wiimote_classic classic;
  struct wiimote_midi midi;

  uint8_t switch_mode; // unused
  enum wiimote_connected_extension_type extension;
  uint8_t reset_ir;
  uint8_t fake_motion;
  uint8_t center_accel;
  uint8_t sideway; // unused
} WiimoteReport;

void wiimote_init(struct wiimote_state *state);
void wiimote_destroy(struct wiimote_state *state);

void wiimote_reset(struct wiimote_state *state);

int process_report(struct wiimote_state *state, uint16_t report_id, const uint8_t *report, int len);
int generate_report(struct wiimote_state * state, uint8_t * buf, bool input_report_changed);

void read_eeprom(struct wiimote_state * state, uint32_t offset, uint16_t size);
void write_eeprom(struct wiimote_state * state, uint32_t offset, uint8_t size, const uint8_t * buf);
void read_register(struct wiimote_state *state, uint32_t offset, uint16_t size);
void write_register(struct wiimote_state *state, uint32_t offset, uint8_t size, const uint8_t * buf);

void init_extension(struct wiimote_state *state);

#if defined(__cplusplus)
}
#endif

#endif //WIIMOTE_H
