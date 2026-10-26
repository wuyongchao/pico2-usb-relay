# Pico 2 USB Relay

An open-source RP2350 firmware that presents one relay through the DCTTech
HID protocol used by `usbrelay`. It is built with the Raspberry Pi Pico SDK
and TinyUSB, and is intended as a compact USB-controlled relay interface for
bench automation and low-voltage switching.

> **Hardware status:** USB enumeration and report handling have been validated
> against the Windows utility described below. The GPIO output and relay load
> path still require validation on the target relay board. Use an LED or meter
> before connecting a load.

## Features

- One DCTTech-compatible relay channel with on, off, and status commands.
- Five-character relay ID with optional persistence in the last flash sector.
- Configurable GPIO, active polarity, relay ID, and USB identity at build time.
- Portable host protocol test suite that does not require hardware.
- Reproducible RP2350 firmware output for Pico 2.

## Requirements

- Linux, macOS, or Windows with CMake 3.20 or newer.
- GNU Arm Embedded Toolchain with `arm-none-eabi-gcc`.
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) 2.3.0 or
  newer. SDK submodules are required.
- [picotool](https://github.com/raspberrypi/picotool) 2.3.0 or newer.
- A Raspberry Pi Pico 2 (RP2350) board.

The following example installs the prerequisites on Ubuntu. Use current
upstream release notes if your distribution packages differ.

```sh
sudo apt install build-essential cmake gcc-arm-none-eabi \
  libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib git

git clone --branch 2.3.0 --recurse-submodules \
  https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
export PICO_SDK_PATH="$HOME/pico-sdk"
```

Install `picotool` by following its upstream build instructions, or let the
Pico SDK build it on demand. A local `picotool` installation makes the build
faster.

## Build

The build uses `PICO_SDK_PATH` from the environment, or accepts an explicit
`-DPICO_SDK_PATH=...` option:

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build -DPICO_BOARD=pico2 -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

`USBRELAY_VERSION` defaults to `0.1.0`. Set it to the semantic version of a
release build, without the leading `v`.

Build products include:

```text
build/pico2_usbrelay.uf2
build/pico2_usbrelay.elf
```

The UF2 file is intentionally generated rather than committed. GitHub Actions
attaches it to tagged releases.

### Flash

1. Hold **BOOTSEL** while connecting the Pico 2.
2. Release **BOOTSEL** after the `RP2350` mass-storage device appears.
3. Copy `build/pico2_usbrelay.uf2` to that device.

The board reboots automatically. For a manual flash, follow the Pico SDK
`picotool` instructions.

## Defaults and configuration

| Setting | Default |
| --- | --- |
| MCU | Pico 2 / RP2350, Arm Secure image |
| Relay channel | 1 |
| Control GPIO | GPIO6 |
| Logical ON | GPIO low |
| Startup state | OFF (GPIO high) |
| Relay ID | `PICO1` |
| USB identity | `16c0:05df`, `USBRelay1` |
| USB serial string | Firmware version (`0.1.0` by default) |
| ID persistence | Enabled in the last flash sector |
| USB CDC | Disabled |
| USB disconnect state | Unchanged (no automatic release) |

Configure the GPIO, polarity, ID, and USB identity at configure time:

```sh
cmake -S . -B build -DPICO_BOARD=pico2 -DCMAKE_BUILD_TYPE=Release \
  -DUSBRELAY_CH1_GPIO=6 \
  -DUSBRELAY_ACTIVE_LOW=ON \
  -DUSBRELAY_RELAY_ID=PICO1 \
  -DUSBRELAY_VID=16C0 \
  -DUSBRELAY_PID=05DF \
  -DUSBRELAY_MANUFACTURER=www.dcttech.com \
  -DUSBRELAY_PRODUCT=USBRelay1
cmake --build build --parallel
```

`USBRELAY_ACTIVE_LOW=ON` means logical ON drives the GPIO to 0 V. Set it to
`OFF` when the relay driver is active high. Relay IDs must be exactly five
uppercase ASCII letters or digits. Flash persistence can be disabled with
`-DUSBRELAY_PERSIST_ID=OFF`.

### USB device identity

The defaults reproduce the vendor ID, product ID, and product string expected
by the compatibility tool. They are build-time options specifically to make
this behavior explicit.

- Do not imply that a modified device is an official DCTTech product.
- Do not use the default VID/PID for unrelated products or commercial clones.
- For a new product, use a VID you control and obtain a PID allocation, or use
  an appropriate open hardware VID/PID program.
- Keep the compatibility defaults only on privately built devices whose USB
  identity is appropriate for that use.

See [LICENSE-NOTES.md](LICENSE-NOTES.md) for licensing and trademark details.

## Host tests

Run the protocol tests without a Pico or a target toolchain:

```sh
cmake -S tests -B build-host -DCMAKE_BUILD_TYPE=Release
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure
```

The tests cover feature-report layout, channel and all-channel commands,
invalid input, ID changes, and null-pointer handling.

## Hardware validation

Start with the Pico isolated and no relay load attached. Confirm the GPIO
changes state before connecting a relay driver.

The reported Windows compatibility utility uses a different command line from
the Linux `usbrelay` package:

```powershell
.\usbrelay.exe -list
.\usbrelay.exe -status
.\usbrelay.exe -serial PICO1 -on 1
.\usbrelay.exe -serial PICO1 -off 1
```

For a Linux/Unix build of `usbrelay`, use:

```sh
lsusb -d 16c0:05df -v
usbrelay -d
usbrelay PICO1_1=1
usbrelay
usbrelay PICO1_1=0
```

Expected state transitions are `PICO1_1=0` and `PICO1_1=1`. With the default
active-low setting, the physical GPIO6 level is inverted from the logical
state.

The RP2350/TinyUSB descriptors used by this firmware are listed below. They
differ from the original low-speed relay in the EP0 packet size, HID version,
and endpoint packet size.

```text
USB speed                   Full-Speed
bMaxPacketSize0             64
HID bcdHID                  0x0111
bEndpointAddress            0x81 (Interrupt IN)
wMaxPacketSize              16
bInterval                   20 ms (0x14)
Report descriptor           22 bytes
Feature report data         8 bytes
```

The previous documentation claimed values from the original device that are
not produced by this firmware. If enumeration works but the host utility does
not see the board, capture `usbrelay -d` output and a raw USB trace before
changing protocol bytes. See [docs/DEBUGGING.md](docs/DEBUGGING.md) for the
investigation history.

## Protocol

The HID Feature Report data is eight bytes without a Report ID:

```text
<ID0> <ID1> <ID2> <ID3> <ID4> 00 00 <state-bitmap>
```

Control uses a nine-byte HIDAPI write buffer whose first byte is the Report ID:

```text
00 FF <channel> 00 00 00 00 00 00       # on
00 FD <channel> 00 00 00 00 00 00       # off
00 FE 00 00 00 00 00 00 00              # all on
00 FC 00 00 00 00 00 00 00              # all off
00 FA 00 <ID0> <ID1> <ID2> <ID3> <ID4>  # change ID
```

Channel 9 means all channels. Channel 0 in the ID command selects an ID
change. The 22-byte report descriptor matches the DCTTech relay report layout.

The firmware currently implements one relay channel. Do not advertise
`USBRelay2`, `USBRelay4`, or `USBRelay8` until the corresponding GPIO outputs
are implemented.

## Safety

This project controls physical hardware and is provided without warranty.

- Never connect a relay coil directly to a GPIO.
- Use a transistor, MOSFET, or opto-isolated driver with a flyback diode.
- Verify the relay board's active level and supply requirements.
- Do not switch mains voltage without appropriate isolation, creepage,
  enclosure, fusing, and compliance testing.
- USB disconnection does not release the relay. Add an external fail-safe if
  the application requires one.
- Test a modified build on an isolated load before putting it into service.

## Project layout

```text
src/                 Firmware and reusable relay protocol implementation
tests/               Host-side protocol tests
docs/                Hardware and USB debugging notes
.github/workflows/   Continuous integration and release workflows
```

## Contributing

Bug reports and pull requests are welcome. Read
[CONTRIBUTING.md](CONTRIBUTING.md) before changing protocol behavior,
descriptors, or GPIO defaults. Security issues should follow
[SECURITY.md](SECURITY.md), not a public issue.

## License

Original project code is available under the [MIT License](LICENSE). External
dependencies retain their own licenses. This project is an independent
compatibility implementation and is not affiliated with DCTTech or Raspberry
Pi Ltd.
