# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Open-source project metadata, community health files, and CI.
- Configurable USB identity and host-side protocol tests.
- Firmware version string exposed as the USB serial number.

### Changed

- Build products are published as GitHub Release assets instead of being
  tracked in the source tree.
- Documentation now reflects the tested RP2350 descriptor values.

### Fixed

- Removed a machine-specific Pico SDK path from the build configuration.
- Added input validation for build-time relay and USB settings.
- Guarded protocol APIs against null pointers.

## [0.1.0] - 2026-10-26

### Added

- Initial RP2350 USB HID relay firmware with one active-low relay channel.
- DCTTech-compatible feature report handling, relay state, and ID persistence.
- Windows and Linux `usbrelay` validation notes.
