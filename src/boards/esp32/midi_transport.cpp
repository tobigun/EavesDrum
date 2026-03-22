#include "midi_transport.h"
#include "midi_transport_esp32_ble_midi.h"
#include <BLEMidi.h>

MidiTransport_Esp32BleMidi midiTransportEsp32BleMidi;

MidiTransportInstances midiTransportInstances = {
  .usbDevice = &midiTransportEsp32BleMidi, // dummy
  .bleServer = &midiTransportEsp32BleMidi
};
MidiTransportMultiplexer midiTransport(midiTransportInstances);
