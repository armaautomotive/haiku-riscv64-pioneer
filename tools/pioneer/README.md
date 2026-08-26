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

`on` holds the physical power button for 0.75 seconds. `off` and `restart` hold
it for seven seconds and release it, providing the long press used for forced
shutdown or recovery.
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

## SG2042 timer DTB patch

`pioneer_timer_dtb_patch.sh` adds the OpenSBI-compatible SG2042 timer fallback
to the boot DTB. It validates the exact SD card and original DTB hash, backs up
the DTB to the Linux SSD, preserves the vendor compatible, and verifies the
installed file. Run it first without `--apply` for a dry run.
