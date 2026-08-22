# Pioneer power control

The DSD TECH SH-UR01A relay appears on this Mac as
`/dev/cu.usbserial-110`. The Pioneer serial console is the separate
`/dev/cu.usbserial-0001` device; the scripts deliberately exclude that port
from automatic relay detection.

Wire the Pioneer's power-button contacts through the relay's `COM` and `NO`
terminals. Closing the relay then acts like pressing the physical button;
opening it releases the button.

```sh
tools/pioneer/pioneer_power.sh test
tools/pioneer/pioneer_power.sh on
tools/pioneer/pioneer_power.sh off
tools/pioneer/pioneer_power.sh restart
```

`on` and `off` each hold the same physical power button for one second and
release it; the Pioneer's current power state determines the result. `restart`
holds it for 10 seconds and releases it, providing the long press used for forced recovery.
Override the hold time with `--duration`, or select another serial port with
`--device`:

```sh
tools/pioneer/pioneer_power.sh restart --duration 12
tools/pioneer/pioneer_power.sh test --device /dev/cu.usbserial-110
```

The relay uses 9600 baud, 8 data bits, no parity, and one stop bit. The script
uses only the Python standard library; no `pyserial` installation is needed.

Do not switch a load beyond the SH-UR01A relay rating. Avoid interrupting power
while the Pioneer is writing to its SD card; use `restart` as a hard recovery
mechanism when a clean shutdown is unavailable.
