#include "midi_transport.h"
#include "midi_transport_arduino_midi.h"
#include "midi_serial_usb_device.h"
#include "midi_serial_usb_host.h"
#ifdef HAS_BLUETOOTH
#include "ble/midi_transport_ble_client.h"
#include "ble/midi_transport_ble_server.h"
#endif
#include "midi_transport_guitar_hero.h"
#include <SPISlave.h> // required for MidiTransport_GuitarHero

MidiSerialUsbDevice midiSerialUsbDevice;
MidiTransport_ArduinoMidi<MidiSerialUsbDevice> midiTransportUsbDevice(midiSerialUsbDevice);

MidiSerialUsbHost midiSerialUsbHost;
MidiTransport_ArduinoMidi<MidiSerialUsbHost> midiTransportUsbHost(midiSerialUsbHost);
  
#ifdef HAS_BLUETOOTH
MidiSerialBleClient midiSerialBleClient;
MidiTransport_BleClient midiTransportBleClient(midiSerialBleClient);

MidiTransport_BleServer midiTransportBleServer;
#endif

MidiTransport_ArduinoMidi<SerialUART> midiTransportDin(Serial1);

MidiTransport_GuitarHero midiTransportGuitarHero;

MidiTransportInstances midiTransportInstances = {
  .usbDevice = &midiTransportUsbDevice,
  .usbHost = &midiTransportUsbHost,
  .serialDin = &midiTransportDin,
#ifdef HAS_BLUETOOTH
  .bleClient = &midiTransportBleClient,
  .bleServer = &midiTransportBleServer,
#endif
  .guitarHeroDrum = &midiTransportGuitarHero,
};
MidiTransportMultiplexer midiTransport(midiTransportInstances);
