# Licensing and trademark notes

The original source code in this repository is licensed under the MIT License.
See [LICENSE](LICENSE).

The build downloads the Raspberry Pi Pico SDK and TinyUSB as external
dependencies. They are not vendored here and remain subject to their own
licenses and notices. Picotool is also used to create the UF2 image and is not
part of this repository.

This is an independent compatibility implementation. It is not affiliated
with, endorsed by, or supported by DCTTech or Raspberry Pi Ltd. "Raspberry Pi",
"Pico", "RP2350", and "TinyUSB" may be trademarks of their respective owners.
The MIT License does not grant rights to third-party names or logos.

The default USB VID/PID and manufacturer/product strings are compatibility
defaults. Anyone distributing a modified device should verify that its USB
identity is appropriate for the intended use and distribution. See the
[USB device identity](README.md#usb-device-identity) section before publishing
a build.
