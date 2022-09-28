# Raffstore Control System (RCS)
> Web enabled relay board with hardware button support for easy smart home integration

## Features
Hardware:
- 9 - 30 VDC power supply
- ESP32-S3 Microcontroller
- 7 channels with two inputs and two outputs each
- inputs and outputs rated for 230 VAC / 325DC
- inputs and outputs galvanically isolated
- outputs fused
- outputs mutually exclusive (both off, only one on)
- 10-100 Mbps Ethernet connection
- 2.4 GHz 802.11 b/g/n Wi-Fi connection
- UART connection
- UART and JTAG over USB-B

Software:
- simple HTTP API with per and all channel controls
- Wi-Fi fallback if no Ethernet connection
- web interface based on simple HTTP API (See [Web Interface and HTTP API](#web-interface-and-http-api))
- time based automatic output disabling (See [Stop Timeout](#stop-timeout))
- [hardware button pattern recognition](#hardware-buttons)
- simple profile based [configuration](#project-configuration)
- [OTA update support](#firmware-upgrade)


## Repository Layout
|          Path          |                        Description                        |
| :--------------------: | :-------------------------------------------------------: |
|      `hardware/`       |             KiCad 6 schematics and pcb layout             |
|      `software/`       |                ESP-IDF component directory                |
|   `software/config/`   |     [firmware config system](#project-configuration)      |
| `software/controller/` |     relay and switch controller handling hardware I/O     |
|    `software/http/`    |     web server serving the web interface and HTTP API     |
|    `software/main/`    |                    firmware entrypoint                    |
|  `software/network/`   |                background network service                 |
|   `software/update/`   |          [OTA update](#firmware-upgrade) service          |
|    `partitions.csv`    |       partition table for ESP32-S3 with 8 MB flash        |
|  `sdkconfig.defaults`  |   ESP-IDF project config for generating the full config   |
|     `thunder.json`     | Thunder Client config for [OTA update](#firmware-upgrade) |


## Usage

### Web Interface and HTTP API
The web interface and HTTP API is served on port 80. Every channel can be opened (1st output on), closed (2nd output on) and stopped (both outputs off).
This can be done though the API by sending a `POST` request to `/actions/<action>/<channel_num>` where `<action>` is either `open`, `close` or `stop`, and `channel_num` is between including 0 and excluding the configured number of channels. Additionally channels can be controlled all at once by omiting the `/<channel_num>`. This is also possible in the web interface.

### Stop Timeout
Each channel has a configurable stop timeout, which is the longest time a channel has one of its output on. The timeout starts / resets with each open or close request.
After reaching the timeout the channel is stopped. This ensures minimal idle power usage and stress on the motor.

### Hardware Buttons
Two buttons are supported per channel and are used one for opening and the other for closing.
Each button supports basic pattern matching to accommodate three different control modes:
1. Press and hold to move the channel until the button is released.
2. Single click the button move the channel until the button is pressed again or the stop timeout is reached.
3. Double click the button to move all channel at the same time until the button is pressed again or the stop timeout is reached.

### Project Configuration
The configuration system utilizes `#define` statements from the currently active profile. A profile consists of a single header file in `software/config/include/config/profiles/` and an entry in `config/Kconfig`. It is recommended to create a copy of the default profile and start tweaking from there. The active config profile can be selected with ESP-IDF menuconfig under `Component Config > Raffstore Control System`.

### Firmware Upgrade
The firmware can be upgraded either over UART, USB or over-the-air (OTA) with HTTP over Ethernet or Wi-Fi. For the wired approaches use the ESP-IDF flashing tool and flash `build/RaffstoreControlSystem.elf`. The OTA update requires the use of a HTTP client (e.g. Thunder Client). To start the update, send a POST request to `/flash` with the firmware image as the body. Do not use multipart form data file upload, instead just put the raw binary data in the body. Use `build/RaffstoreControlSystem.bin` for OTA updates. The server will not send a response, it will apply the update and reboot. If the image gets corrupted during upload or flashing, the update is invalidated and the previous firmware will be used.


## License
RaffstoreControlSystem - Web enabled relay board with hardware button support for easy smart home integration<br>
Copyright (C) 2022 The Rhynaz

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
