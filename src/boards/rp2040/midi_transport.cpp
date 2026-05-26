#include "midi_transport_tiny_usb_device.h"
#include "midi_transport_tiny_usb_host.h"
#ifdef HAS_BLUETOOTH
#include "ble/midi_transport_ble_client.h"
#include "ble/midi_transport_ble_server.h"
#endif

#include "SerialTxUART.h"
#include "midi_transport_serial.h"

#include "midi_transport_guitar_hero_spi.h"
#include <SPISlave.h> // required for MidiTransport_GuitarHero_SPI

MidiTransport_UsbDevice midiTransportUsbDevice;
MidiTransport_TinyUsbHost midiTransportTinyUsbHost;

#ifdef HAS_BLUETOOTH
MidiTransport_BleClient midiTransportBleClient;
MidiTransport_BleServer midiTransportBleServer;
#endif

MidiTransport_Serial midiTransportDin(SerialTx2);

MidiTransport_GuitarHero_SPI midiTransportGuitarHeroSPI;

MidiTransportInstances midiTransportInstances = {
  .usbDevice = &midiTransportUsbDevice,
  .usbHost = &midiTransportTinyUsbHost,
  .serialDin = &midiTransportDin,
#ifdef HAS_BLUETOOTH
  .bleClient = &midiTransportBleClient,
  .bleServer = &midiTransportBleServer,
#endif
  .guitarHeroDrumSPI = &midiTransportGuitarHeroSPI,
};
MidiTransportMultiplexer midiTransport(midiTransportInstances);
