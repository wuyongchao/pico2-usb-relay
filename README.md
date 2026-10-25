# Pico 2 DCTTech-compatible USB relay

RP2350 (Pico 2) TinyUSB firmware that exposes one relay through the
DCTTech HID protocol used by `usbrelay`.

The firmware is compiled successfully but has not yet been tested on the
physical Pico 2 or a relay board. Test the USB path with an LED or meter
first, then connect the relay driver.

## Defaults

- MCU: Pico 2 / RP2350, ARM Secure image
- USB: `16c0:05df`, `USBRelay1`, manufacturer `www.dcttech.com`
- Relay ID: `PICO1` (five uppercase letters/digits)
- Relay channel 1: GPIO6
- Logic ON drives GPIO6 low; startup drives GPIO6 high (relay released)
- ID changes are persisted in the last flash sector
- No USB CDC interface
- USB disconnect does not intentionally change the relay output

Do not connect a relay coil directly to a GPIO. Use a suitable transistor,
MOSFET or opto-isolated driver and a flyback diode. Verify the relay board's
active level with the actual hardware before connecting a load.

## Build

```sh
cmake -S . -B build -DPICO_BOARD=pico2 -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Outputs:

```text
build/pico2_usbrelay.uf2
build/pico2_usbrelay.elf
```

Change the GPIO or polarity at configure time:

```sh
cmake -S . -B build -DPICO_BOARD=pico2 \
  -DUSBRELAY_CH1_GPIO=6 \
  -DUSBRELAY_ACTIVE_LOW=ON \
  -DUSBRELAY_RELAY_ID=PICO1
cmake --build build --parallel
```

`USBRELAY_ACTIVE_LOW=ON` means logical relay ON outputs 0 V on the GPIO.
Set it to `OFF` if the driver is active high.

ID persistence can be disabled with `-DUSBRELAY_PERSIST_ID=OFF`.

## Host protocol tests

These tests compile with the host compiler and do not touch hardware:

```sh
cmake -S tests -B build-host -DCMAKE_BUILD_TYPE=Release
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure
```

## Hardware validation

With the Pico 2 isolated and no load attached.

The provided Windows utility is a different frontend from the Linux
`usbrelay` package. Use its options:

```cmd
usbrelay.exe -list
usbrelay.exe -status
usbrelay.exe -serial PICO1 -on 1
usbrelay.exe -serial PICO1 -off 1
```

For a Linux/Unix build of `usbrelay`, use:

```sh
lsusb -d 16c0:05df -v
usbrelay -d
usbrelay PICO1_1=1
usbrelay
usbrelay PICO1_1=0
```

Recheck the descriptors after flashing. These values must match the reference:

```text
bMaxPacketSize0          8
HID bcdHID               0x0101
bEndpointAddress         0x81 (Interrupt IN)
wMaxPacketSize           0x0008
bInterval                0x14 (20 ms)
```

Expected state transitions:

```text
PICO1_1=0
PICO1_1=1
```

The physical GPIO6 level should be inverse to the logical state with the
default active-low configuration. If enumeration succeeds but `usbrelay`
does not see the board or reports the wrong ID/state, capture a raw USB trace
and the `usbrelay -d` output before changing protocol bytes.

For the first run, keep the relay driver disconnected and measure GPIO6 with a
meter or LED. This firmware does not automatically release the relay when USB
disconnects; use an external hardware fail-safe if that behavior is required.

## Protocol

The HID Feature Report data is eight bytes without a Report ID:

```text
<ID0> <ID1> <ID2> <ID3> <ID4> 00 00 <state-bitmap>
```

Control uses a nine-byte HIDAPI write buffer whose first byte is the report
ID:

```text
00 FF <channel> 00 00 00 00 00 00   # on
00 FD <channel> 00 00 00 00 00 00   # off
00 FA 00 <ID0> <ID1> <ID2> <ID3> <ID4>  # change ID
```

Channel 9 means all channels; channel 0 in the ID command selects ID change.
The report descriptor is the original 22-byte DCTTech descriptor. A local HID
class driver preserves that descriptor while adapting the historical
`usbrelay` GET_REPORT(1) response.

## Multi-channel support

The current firmware intentionally implements one relay channel. Multi-channel
GPIO mapping is a later extension; do not advertise `USBRelay2`, `USBRelay4`,
or `USBRelay8` until those outputs are implemented.
