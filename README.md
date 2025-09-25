<p align="center">
  <img alt="Yarn" src="doc/logo/Eavesdrum-banner.svg" width="100%"/>
</p>

<p align="center">
  No matter if you are E(a)ve or Ste(a)ve - drumming for e(a)veryone
</p>

<p align="center">
  <a href="https://github.com/tobigun/EavesDrum"><img alt="GitHub Actions status" src="https://github.com/tobigun/EavesDrum/actions/workflows/build-firmware.yml/badge.svg"/></a>
  <a href="https://github.com/tobigun/EavesDrum"><img alt="Latest Release" src="https://img.shields.io/github/release/tobigun/EavesDrum.svg?style=flat"/></a>
  <a href="https://opensource.org/licenses/"><img alt="GPLv3 License" src="https://img.shields.io/badge/License-GPL%20v3-yellow.svg"/></a>
</p>


# Welcome to EavesDrum!

EavesDrum is an open-source e-drum trigger module project, designed for drummers and makers.

It allows you to connect a multitude of drum pads or piezo triggers and converts their signals into MIDI events.

The configuration of the module is made simple and user-friendly through a built-in WebUI. You can adjust sensitivity, threshold, and other parameters directly from your browser—no need for complicated software or direct hardware programming.

The trigger module acts as a USB MIDI device - so simply connect it to your Notebook and use a drum software of your choice (EZdrummer, Addictive Drums, SSD5, ...) to convert your notebook into a complete drum module.

<a href="https://github.com/tobigun/EavesDrum"><img alt="GitHub Actions status" src="doc/latency.svg"/></a>

## Features

- **Drum Trigger Inputs:**
  - With the EavesDrum PCB, you will be able to connect your drum pads or piezo sensors to 32 analog inputs.
  - Each input can be mapped individually, e.g. to 16 TRS (3-pin Tip-Ring-Sleeve) connectors or 32 TS (2-pin Tip-Sleeve) connectors.
  - The default configuration uses the following drum setup:
    - Snare (1x TRS for head/rim + 1x TS for side-rim)
    - 5x Toms (each with 1 TRS for head/rim)
    - Kick drum (1x TS)
    - Ride (1x TRS for bow/edge + 1x TS for cup)
    - 4x Cymbals (each with 1x TRS for bow/edge + 1x TS for cup)
    - Hi-Hat (1x TRS for bow/edge + 1x TS for cup)
    - Hi-Hat Pedal resitive or optical (connected to a dedicated connector with 3.3V or 5V supply voltage)
- **Compatibility:**
  -  EavesDrum should be compatible with most Roland (non-USB), YAMAHA, ATV and EFNOTE pads
  -  The following pads were tested:
     - ATV XD-P10M/P13M 2-zone Drums
     - ATV aD-C14/C16/C18 3-zone Cymbals
     - EFNOTE EFD-S12P 3-zone Snare (head, rim, side-rim)
     - Roland PDX-8 2-zone Drum
     - Roland KD-10 Kick Pad
     - Zeitgeist ZG ECP 13" 3-zone Cymbal
     - Zeitgeist ZG ZKD7 Kick Pad
     - Drum-tec Hi-Hat controller (resistive)
     - DIY optical sensor (TCRT5000 based). See sections below for schematics and Gerber files
     - DIY drums and cymbals. See sections below for schematics and Gerber files
- **MIDI Mappings:**
  - Mappings are freely customizable via the UI.
  - It already contains Midi-Mappings for:
    - Addictive Drums 2
    - BFD Player 1.2
    - EZdrummer 3
    - MT Power 2
    - Steven Slate Drums 5 (SSD5)
- **Low Latency:**
  - Latency is mainly determined by your soundcard and the selected scan time.
    - With a decent sound card (e.g. Behringer UMC204HD) the output latency is about 5 ms.
    - A scan time of 3 ms is selected per default for each pad but can be manually lowered if your pad still triggers correctly.
    - This makes a total latency of ~8 ms
  - You can measure the latency of your setup with latency measurement feature built into the UI
- **Open Source:** Source code and PCB Gerber files are available for customization and extension.
- **WebUI Configuration:** Browser-based interface for setting up and tweaking trigger parameters such as sensitivity, threshold, retrigger prevention, and more.

## Before you start: is EavesDrum what you need?
**EavesDrum is for you, if you:**
- need more / additional trigger inputs to those that standard drum kits support
- want to mix drums from different vendors as most drum kits only work well with their own pads
- want to start drumming and do not want to pay a fortune for a drum kit
- have fun making stuff and just want to build your own drum kit or implement your own drum module features

**EavesDrum might not be for you, if you:**
- want to use the module for live performances as it does not give any guarantee error-free operation at all times or conditions
- want a drum module that works standalone without a Notebook
- need cross-talk cancelation, positional sensing or decay curves, as these features are not implemented (yet)

## Getting Started

1. **Build the EavesDrum module:**<br/>
   - Either built it yourself or order it via a PCB manufacturer of your choice (e.g. JLCPCB, PCBWay, Aisler).
   - Check the sections below for schematics and Gerber files
   - EavesDrum supports the following Microcontroller boards:
     - Raspberry Pi Pico 2 (recommended)
     - Raspberry Pi Pico
2. **Download & Flash the firmware:**<br/>
   - Download the firmware for your Pico board from the Release download section
   - Press the BOOTSEL while connecting your Pico Board via USB to enter the bootloader mode
   - The board will now be detected as an USB flash drive
   - Drag & Drop the firmware .uf2 file onto the drive
3. **WebUI Access:**<br/>
   - After flashing is done, the device should be detected as an USB network device
   - Open your browser and navigate to the WebUI by entering either
     - the static IP <a href="http://192.168.7.1">http://192.168.7.1</a> or
     - the DNS name <a href="http://eaves.drum/">http://eaves.drum</a>
4. **Calibrate:**<br/>
   - Calibrate the voltage offset with the UI Wizard
5. **Connect Drums:**<br/>
   - Connect your drum pads or triggers to the EavesDrum hardware
6. **Configuration:**<br/>
   - Adjust settings and MIDI mappings
7. **Play:**<br/>
   - Once configured, start playing and enjoy responsive drum triggering!

## <a name="how-to-build-ui"> How to build the UI
- Install [NodeJS](https://nodejs.org/en)
- Open the project directory in a terminal (cmd or git bash)
- Change directory to the `webui` directory
  - `cd webui`
- Install the dependencies with NPM
  - `npm install`
  - (This will require NodeJS and NPM, which should be installed with NodeJS. If the command does not work, make sure that the binary path of NodeJs is in your environment PATH)
- Now build the UI with `npm run build`. The result should be written to the `data` directory in the root directory of the project
- You can also start `npm run serve` to start a server on `http://localhost:8080`. This will proxy requests either to the [PC simulation](#simulation) or to the hardware device, depending on the selected target in `vite.config.ts`.

## <a name="how-to-build-fw"> How to build the firmware
- Download and install [Visual Studio Code](https://code.visualstudio.com/)
- Open Visual Studio Code and install the [PlatformIO](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) extension
- Download the EavesDrum code from this repository via Git or zip
- Open the root directory of this project (the one with the `platform.ini` file) in Visual Studio Code
- Select the target ("pico2" is recommended)
- Press `build`
- Press `upload`
  - Per default the Raspberry Pi Debug Probe is selected to upload the firmware
  - Change `upload_protocol` from `cmsis-dap` to `mbed` in the `platform.ini` file (section `pico-base`) if you do not have a Debug Probe and want to upload the firmware per USB instead. This will create an UF2 file and flash it via the flash-drive in bootloader mode.
    - If the upload fails, try to enter the bootloder manually by pressing the `BOOTSEL` button of the Pico when connecting it via USB (or alternatively pressing the `RESET` button if you use the EavesDrum PCB).
- Click on the `Platform` / `Upload Filesystem image` task in the PlatformIO tab (click the side-bar icon with the PlatformIO ant logo to open the menu first)
  - Important: this requires that the WebUI is setup first. Follow the steps of [How to build the UI](#how-to-build-ui) until `npm install`. A manual build is not required, as the `Upload Filesystem image` task will trigger a build for you.

## <a name="simulation"></a> How to simulate a drum-kit on a PC
There is also a simulation of a drum kit for PC in case you do not want to test on real hardware.

**Build the simulation:**
- Install the GCC/G++ Toolchain for Windows via MSYS2
  - https://code.visualstudio.com/docs/cpp/config-mingw#_installing-the-mingww64-toolchain
- Install portmidi in the MSYS2 shell:
  - `pacman -S mingw-w64-x86_64-portmidi`
- Follow the steps of [How to build the firmware](#how-to-build-fw) until the build step but select the `native-win` environment instead

**Run the simulation:**
- The `upload` button will start the application
- The WebUI will be reachable on port 80 via http://localhost
- Some actions can be triggered via the terminal:
  - A pad hit can be triggered with the number keys (1: first pad, 2: second pad, ..., 0: 10th pad). The zones are triggered randomly
  - Press `Shift` + \<number> to choke the corresponding pad
  - Press `+` or `-` to increase or increase the offset of the monitored pad. This is useful to simulate pedals
  - Press `#` to select the amount of Jitter in the signal of the monitored pad. This is useful to simulate noise in a pad's signal or fluctuations in pedal presses
  - Press any other key to trigger a hit of the monitored pad

## Other related projects
- [Hello Drum](https://github.com/RyoKosaka/HelloDrum-arduino-Library) by  Ryo Kosaka
  - a nice Open Source trigger module with DIY drum pads (https://open-e-drums.com/)
  - unfortunateley the last commit is 5 years old
- [Edrumulus](https://github.com/corrados/edrumulus) by Volker Fischer (corrados)
  - another Open Source trigger module with Positional sensing

## License
This project is licensed under the [GNU General Public License v3.0](LICENSE.txt).

## Author
- [Tobias Gunkel (tobigun)](https://github.com/tobigun)

