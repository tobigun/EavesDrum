# pico-w-ble-midi-lib

This library contains a GATT server and a GATT client for the BLE-MIDI service,
MIDI 1.0 packet encoder and decoder. The client and server can work by themselves,
or a higher level application can switch between client and server modes. It
implements the [Specification for MIDI over Bluetooth Low Energy
(BLE-MIDI) 1.0](https://midi.org/specifications/midi-transports-specifications/midi-over-bluetooth-low-energy-ble-midi).
Downloading this standard requires a free login from the
[midi.org](https://midi.org/) website.

This library expects to be built using `pico-sdk` version
2.0 or later and it expects to execute on a Raspberry Pi Pico W board
with a RP2040 processor and a WiFi/Bluetooth module
that uses the Infineon CYW43439 WiFi/Bluetooth chip.

Before using this library in a commercial application, please read the
`LICENSE` file and the comments in each source code header file. I
have tried to structure this library to make as much code as possible
MIT License.

For a simple example that uses the BLE-MIDI server to send and receive MIDI data,
see the [pico-w-ble-midi-server-demo](https://github.com/rppicomidi/pico-w-ble-midi-server-demo)
project. For a more practical demo, see the [ble-midi2usbhost](https://github.com/rppicomidi/ble-midi2usbhost)
project.

## Before using any library
You must initialize the Bluetooth hardware by calling `cyw43_arch_init()` 
once before calling the init() function for either library. 

## BLE-MIDI server library
This library enables the user application to send and receive MIDI 1.0 data
via standard MIDI 1.0 byte streams. The `ble_midi_server_lib` library
provides all Bluetooth functions an application needs to send and receive MIDI 1.0 byte streams.
It supports a simple profile that contains only the MIDI service and the
device's GAP_DEVICE_NAME characteristic.
It uses the `ble_midi_service_lib` library for the MIDI data transfers,
and it handles all other control messages between Bluetooth stack and the application.
If you want to make a more
complicated profile than the one `ble_midi_server_lib` supports,
you can use `ble_midi_service_lib` to implement it. For example,
you might want to add a battery service to your profile if your
project is battery powered. However, the code that replaces
`ble_midi_server_lib` library will be more complicated.

The `ble_midi_service_lib` INTERFACE
library uses the `ble_midi_pkt_codec` INTERFACE library
to encode the MIDI byte stream to BLE-MIDI 1.0 packets
time-stamped with the system time. It then sends the
BLE-MIDI encoded data to the connected BLE-MIDI client.

The [Accessory Guidelines for Apple Products](https://developer.apple.com/accessories/Accessory-Design-Guidelines.pdf)
specifies a minumum connection interval of 15ms and latency of 30ms (i.e., 2 connection
intervals). The `midi_service_stream_handler.c` code specifies a minimum interval
of 7.5ms and a maximum interval of 15ms with a maximum latency of 0. My iPad consistently
connects at 15ms with 0 latency. It is probably worth experimenting with this because
ideally the connection interval would be 7.5 ms with 0 latency to minimize timing jitter and latency.

When the BLE-MIDI client sends BLE-MIDI 1.0 data to this server, the
`ble_midi_service_lib` works with the the `ble_midi_pkt_codec` to
decode the packet to an array of time-stamped byte stream
structures and buffers them for the application to process.
It is up to the application to handle the timestamps
for presentation order and jitter reduction. Currently,
`ble_midi_server_lib` does not manage timestamps.

The `ble_midi_server_init()` function requires a `profile_data`
argument. The definition of `profile_data` is normally
automatically generated from a `.gatt` file that has this
form
```
PRIMARY_SERVICE, GAP_SERVICE
CHARACTERISTIC, GAP_DEVICE_NAME, READ, "Your device name goes here"

#import <midi_service.gatt>
```
When your application
builds the code, it should generate the GATT database `.h` using
the Pico SDK cmake function `pico_btstack_make_gatt_header`. Be
sure to add the path to this library to your call to `pico_btstack_make_gatt_header` or else the script won't be able
to find the file referenced in the line `#import <midi_service.gatt>`.
When the application includes this `.h`, it will have
the definition of the `profile_data` variable. The 

The file `ble_midi_server.h` contains the server API.
The file `midi_server_stream_handler.h` contains the service API.
The user application must call `midi_service_stream_init()` before
it uses the other two functions. The user application must call
this function with a non-NULL pointer to a `btstack_packet_handler_t` packet
handler function. This packet handler function must, at a minimum,
handle the `BTSTACK_EVENT_STATE` event to start advertising when
the Bluetooth stack starts functioning. The callback must also
process the `HCI_EVENT_GATTSERVICE_META` event and use the
`GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED` event to capture
the connection handle that is a required argument to
`midi_service_stream_read()` and to `midi_service_stream_write()`.
The callback should also handle security manager messages if
encryption is required.

After your application calls `midi_service_stream_init()`, it
must turn on the Bluetooth chip by calling the function
`hci_power_control(HCI_POWER_ON)`.

Once the main application's `btstack_packet_handler_t` packet
handler function records a valid connection handle, the program's
main loop may process MIDI data received by polling
`midi_service_stream_read()`. It may also call `midi_service_stream_write()`
to generate BLE-MIDI messages in response to button presses,
control movement, incoming MIDI 1.0 stream from non-BLE sources, etc.

The files in this directory require the following libraries:
```
    pico_stdlib
    ble_midi_service_lib
    pico_btstack_cyw43
    pico_cyw43_arch_none
    ring_buffer_lib
```

The `ring_buffer_lib` library is custom code that can be found
[here](https://github.com/rppicomidi/ring_buffer_lib). The CMakeLists.txt
for this project expects the `ring_buffer_lib` source directory to be
located at the same directory level as the source files for this project.
For example, if this project is a subdirectory of `proj/lib`, then
the `ring_buffer_lib` source files should be stored in `proj/lib/ring_buffer_lib`.

The `ring_buffer_lib` needs to have a configuration file called `ring_buffer_lib_config.h`
stored somewhere in the project's include path. Because of the required ring
buffer sizes to support this library, please define the `RING_BUFFER_SIZE_TYPE`
macro to be a 16-bit data type or larger. For example
```
#define RING_BUFFER_SIZE_TYPE uint16_t
```
Note that the default definition is `uint8_t`, which is too small for
Bluetooth MIDI ring buffers.

## BLE-MIDI client library
This `ble_midi_client_lib` library implements the basic functions
you need to implement a BLE-MIDI client. A client needs some
sort of UI to manage scanning, connecting, and disconnecting.
The library provides `printf` type console library output and
is best suited for a command line interpreter-based application.
The client also advertises the GAP_DEVICE_NAME charateristic.

