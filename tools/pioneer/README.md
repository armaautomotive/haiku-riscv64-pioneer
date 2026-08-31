# Pioneer relay control

The DSD TECH SH-UR04A relay appears on this Mac as
`/dev/cu.usbserial-1`. The Pioneer serial console is the separate
`/dev/cu.usbserial-0001` device; the scripts deliberately exclude that port
from automatic relay detection.

Channel assignments:

- Relay 1: Pioneer power-button contacts, wired through `COM` and `NO`.
- Relay 2: SD attachment control. Closed means attached; open means detached.
- Relays 3 and 4: currently unused.

```sh
tools/pioneer/pioneer_power.sh test
tools/pioneer/pioneer_power.sh on
tools/pioneer/pioneer_power.sh off
tools/pioneer/pioneer_power.sh restart
tools/pioneer/pioneer_power.sh sd-attach
tools/pioneer/pioneer_power.sh sd-detach
tools/pioneer/pioneer_power.sh sd-status
tools/pioneer/pioneer_power.sh status
```

`on` holds the physical power button for 0.75 seconds. `off` and `restart` hold
it for seven seconds and release it, providing the long press used for forced
shutdown or recovery.
Override the hold time with `--duration`, or select another serial port with
`--device`:

```sh
tools/pioneer/pioneer_power.sh restart --duration 12
tools/pioneer/pioneer_power.sh test --device /dev/cu.usbserial-1
```

The relay uses 9600 baud, 8 data bits, no parity, and one stop bit. The script
uses only the Python standard library; no `pyserial` installation is needed.

Never detach the SD card while either OS has it mounted or while it may be
reading or writing. Detach it only while the Pioneer is fully powered off.
Likewise, attach it before powering on. A single relay contact is suitable only
if it drives a purpose-built SD isolation/multiplexer control input; it must not
be used to interrupt only the card's power line while its signal lines remain
connected.
