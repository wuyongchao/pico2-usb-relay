# Contributing

Thanks for helping improve this project. Keep changes focused and preserve the
existing protocol behavior unless the change is explicitly a compatibility
break.

## Reporting a bug

Please include:

- Firmware version or commit SHA.
- Board and MCU (for example, Pico 2 / RP2350).
- Host OS and `usbrelay` implementation/version.
- The configure command and any non-default options.
- Expected and actual behavior.
- Relevant `usbrelay -d`, `lsusb`, or USB trace output with sensitive data
  removed.

Do not report security vulnerabilities in a public issue. See
[SECURITY.md](SECURITY.md).

## Development setup

Follow the requirements and build steps in [README.md](README.md). Before
submitting a pull request, run:

```sh
cmake -S tests -B build-host -DCMAKE_BUILD_TYPE=Release
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure

cmake -S . -B build -DPICO_BOARD=pico2 -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

If the change touches hardware behavior, also record the exact board, wiring,
GPIO measurements, and host command used for validation. A successful USB
enumeration alone is not enough to prove that a relay output works.

## Pull requests

- Keep each pull request focused on one change.
- Follow the existing C11 style and keep warnings clean.
- Add or update host tests for protocol or parsing changes.
- Update the README, protocol examples, or changelog when behavior changes.
- Use clear commit messages, preferably `type: summary` such as
  `fix: validate relay ID length`.
- Confirm that new files have an appropriate license header or are covered by
  the project license.

By contributing, you agree that your contribution is licensed under the MIT
License used by this repository. Do not submit code, images, or documentation
that you cannot license under those terms.

## Protocol compatibility

Changes to USB descriptors, report layout, command bytes, or default relay
identity can break existing automation. Include evidence for host
compatibility changes and update the tests and documentation in the same pull
request.

## Electrical safety

Do not test mains-voltage or high-current loads as part of ordinary
development. Use a low-voltage load or an isolated indicator until the GPIO
and driver behavior has been verified. Do not submit a change that relies on a
GPIO driving a relay coil directly.
