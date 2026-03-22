#include "midi_transport.h"
#ifdef ENABLE_MIDI_PORTMIDI_TRANSPORT
#include "midi_transport_portmidi.h"
MidiTransport_PortMidi nativeMidiTransport;
#else
#include "midi_transport_dummy.h"
MidiTransport_Dummy nativeMidiTransport;
#endif

#ifdef HAS_BLUETOOTH
#include "midi_transport_ble_client_simulation.h"
MidiTransport_BleSimulation midiTransportBleClientSimulation;
#endif

MidiTransportInstances midiTransportInstances = {
  .usbDevice = &nativeMidiTransport,
  .usbHost = &nativeMidiTransport,
  .serialDin = &nativeMidiTransport,
#ifdef HAS_BLUETOOTH
  .bleClient = &midiTransportBleClientSimulation,
  .bleServer = &nativeMidiTransport,
#endif
  .guitarHeroDrum = &nativeMidiTransport
};
MidiTransportMultiplexer midiTransport(midiTransportInstances);
