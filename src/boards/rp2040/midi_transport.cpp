#include "midi_transport_tiny_usb_device.h"
#include "midi_transport_tiny_usb_host.h"
#ifdef HAS_BLUETOOTH
#include "ble/midi_transport_ble_client.h"
#include "ble/midi_transport_ble_server.h"
#endif

#include "SerialTxUART.h"
#include "midi_transport_serial.h"

#include "midi_transport_guitar_hero.h"
#include <SPISlave.h> // required for MidiTransport_GuitarHero

#define PIN_MIDI_TX 8
#define PIN_MIDI_TX_BOARD_V1_1 20

MidiTransport_UsbDevice midiTransportUsbDevice;
MidiTransport_TinyUsbHost midiTransportTinyUsbHost;

#ifdef HAS_BLUETOOTH
MidiTransport_BleClient midiTransportBleClient;
MidiTransport_BleServer midiTransportBleServer;
#endif

MidiTransport_Serial midiTransportDin(SerialTx2, PIN_MIDI_TX);
//MidiTransport_Serial midiTransportDinBoardV1_1(SerialTx2, PIN_MIDI_TX_BOARD_V1_1);

MidiTransport_GuitarHero midiTransportGuitarHero;

MidiTransportInstances midiTransportInstances = {
  .usbDevice = &midiTransportUsbDevice,
  .usbHost = &midiTransportTinyUsbHost,
  .serialDin = &midiTransportDin,
#ifdef HAS_BLUETOOTH
  .bleClient = &midiTransportBleClient,
  .bleServer = &midiTransportBleServer,
#endif
  .guitarHeroDrum = &midiTransportGuitarHero,
};
MidiTransportMultiplexer midiTransport(midiTransportInstances);
