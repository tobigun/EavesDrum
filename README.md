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

EavesDrum is an e-drum trigger module project, designed for musicians and makers.

It allows you to connect drum pads or piezo triggers and converts their signals into MIDI events. The configuration of the module is made simple and user-friendly through a built-in WebUI.

You can adjust sensitivity, threshold, and other parameters directly from your browser—no need for complicated software or direct hardware programming.

## Features

- **Drum Trigger Inputs:** Connect multiple drum pads or piezo sensors.
- **WebUI Configuration:** Intuitive browser-based interface for setting up and tweaking trigger parameters such as sensitivity, threshold, retrigger prevention, and more.
- **Flexible Output:** Converts trigger signals to MIDI.
- **Open Source:** Source code is available for customization and extension.

## Getting Started

1. **Buy Hardware:**<br/>
   Buy one of the supported Microcontroller boards.
   - Raspberry Pi Pico 2 (recommended)
   - Raspberry Pi Pico
2. **Build the EavesDrum module:**<br/>
   - Either built it yourself or order it via a PCB manufacturer of your choice (e.g. JLCPCB, PCBWay, Aisler).
4. **Flash firmware:**<br/>
   - Press the BOOTSEL on your Pico Board to enter the bootloader.
   - The board will be detected as an USB flash drive.
   - Drag&Drop the firmware .uf2 file onto the drive.
5. **WebUI Access:**<br/>
   - After flashing is done, the device should be detected as an USB network device.
   - Access the WebUI via the static IP <a href="http://192.168.7.1">http://192.168.7.1</a> or the DNS name <a href="http://eaves.drum/">http://eaves.drum</a>.
6. **Calibrate:**<br/>
   - Calibrate the voltage offset with the UI Wizard
7. **Connect Drums:**<br/>
   - Connect your drum pads or triggers to the EavesDrum hardware.
7. **Configuration:**<br/>
   - Adjust settings and MIDI mappings.
8. **Play:**<br/>
   - Once configured, start playing and enjoy responsive drum triggering!

## Technologies Used

- **C Programming Language:** Core firmware is written in C for performance on embedded hardware.
- **Web Technologies:** WebUI for configuration (HTML/CSS/JS, served from the device).
- **MIDI Protocol:** For communication with music software and hardware.

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE.txt).

## Author

- [Tobias Gunkel (tobigun)](https://github.com/tobigun)

---
