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

## Haiku build and SD deployment

The Pioneer firmware image used by this port contains an embedded Haiku EFI
loader. That embedded loader is the copy the board actually executes; updating
only `EFI/BOOT/BOOTRISCV64.EFI` is insufficient.

Build the image and BFS payload with `pioneer_build.sh`, then repack the newly
built loader into a known-working firmware image:

```sh
tools/pioneer/pioneer_firmware_embed.sh \
  --base /path/to/known-working/MilkV-Pioneer.fd \
  --loader /path/to/generated.riscv64/objects/haiku/riscv64/release/system/boot/efi/haiku_loader.efi \
  --output /path/to/MilkV-Pioneer.updated.fd
```

The repacker validates the Pioneer firmware GUIDs and layout, preserves the
vendor firmware outside the compressed DXE allocation, decompresses its own
output, and compares the embedded loader byte-for-byte with the input loader.

Deploying a loader now requires both the standalone loader and the verified
repacked firmware:

```sh
tools/pioneer/pioneer_sd_deploy.sh \
  --device /dev/mmcblk1 \
  --payload /path/to/haiku-pioneer-bfs.img \
  --sha256 BFS_SHA256 \
  --loader /path/to/haiku_loader.efi \
  --loader-sha256 LOADER_SHA256 \
  --firmware /path/to/MilkV-Pioneer.updated.fd \
  --firmware-sha256 FIRMWARE_SHA256 \
  --apply
```

Before writing, the deployment script independently decompresses the supplied
firmware and rejects it unless its embedded loader hash matches
`--loader-sha256`. It backs up and readback-verifies the BFS payload, standalone
loader, and booted firmware.
