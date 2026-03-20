#include "midi_transport.h"
#include "midi_transport_arduino_midi.h"
#include "midi_serial_usb_device.h"
#include "midi_serial_usb_host.h"
#ifdef HAS_BLUETOOTH
#include "ble/midi_serial_ble_client.h"
#include "ble/midi_serial_ble_server.h"
#endif
#include "midi_transport_guitar_hero.h"

MidiSerialUsbDevice midiSerialUsbDevice;
MidiTransport_ArduinoMidi<MidiSerialUsbDevice> midiTransportUsbDevice(midiSerialUsbDevice);

MidiSerialUsbHost midiSerialUsbHost;
MidiTransport_ArduinoMidi<MidiSerialUsbHost> midiTransportUsbHost(midiSerialUsbHost);
  
#ifdef HAS_BLUETOOTH
MidiSerialBleClient midiSerialBleClient;
MidiTransport_ArduinoMidi<MidiSerialBleClient> midiTransportBleClient(midiSerialBleClient);

MidiSerialBleServer midiSerialBleServer;
MidiTransport_ArduinoMidi<MidiSerialBleServer> midiTransportBleServer(midiSerialBleServer);
#endif

MidiTransport_ArduinoMidi<SerialUART> midiTransportDin(Serial1);

MidiTransport_GuitarHero midiTransportGuitarHero;

MidiTransportInstances midiTransportInstances = {
  .usbDevice = &midiTransportUsbDevice,
  .usbHost = &midiTransportUsbHost,
  .serialDin = &midiTransportDin,
  .guitarHeroDrum = &midiTransportGuitarHero,
#ifdef HAS_BLUETOOTH
  .bleClient = &midiTransportBleClient,
  .bleServer = &midiTransportBleServer,
#endif
};
MidiTransportMultiplexer midiTransport(midiTransportInstances);
