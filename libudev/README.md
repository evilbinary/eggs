# libudev-zero

Mostly[^0] ABI/API compatible libudev alternative that works with any device
manager

## Features

- Static linking. Unlike systemd's libudev, it is fully supported.
- No lock-in to a device manager. Any or no device manager can be used.
- Actually readable C code. Unlike systemd, there is no insane amount of
abstractions or clever code.
- No runtime dependencies beyond libc. You will never be impacted[^1] by the
reckless scope of libsystemd.

## Dependencies

- C99 compiler (build time)
- meson (build time)
- POSIX & XSI libc
- Linux >= 2.6.39

## Installation

```sh
meson setup build --prefix /usr
meson compile -C build
meson install -C build
```

The traditional POSIX Makefile is also provided for historical reasons.

## Application usage

### Hotplugging

If desired, hotplugging support can be activated by rebroadcasting netlink
uevents to a configured netlink group[^2]. Because reading uevents directly
introduces a potential race condition with event handling, assistance from your
device manager is required. A good example of a device manager that supports
this by design is mdevd[^3].

If your device manager lacks native support for it, then you have the option to
use the external helper[^4], which can be used to substitute the missing
functionality. Installing the helper alongside the library is recommended, but
not required, unless a better mechanism is available.

## Rationale

This library is intended to provide a compatibility layer for applications that
require libudev on systems that do not offer it, such as when systemd, which
provides it, is too invasive or does not fit the design, and where porting to
the sysfs/uevent interfaces directly is not a practical alternative due to a
deep dependency on libudev or mere reluctance to maintain downstream patches.

[^0]: Some legacy functions marked as such by upstream are unlikely to be
implemented. Notable example is the `udev_queue` API.
[^1]: https://lwn.net/Articles/967212
[^2]: The default number is 0x4, but that can be changed at compile time. If your
distro comes with libudev-zero, make sure you use a valid number before proceeding.
[^3]: `mdevd -O 0x4`. Consult mdevd documentation for more information.
[^4]: See [helper.c](contrib/helper.c)

## License

ISC
