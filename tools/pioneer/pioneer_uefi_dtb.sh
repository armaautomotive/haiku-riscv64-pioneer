#!/bin/bash
# Adapt the pinned v1.4.3 firmware DT to the deployed SG2042 OpenSBI timer
# bindings. Keep vendor CPU phandles/interrupt lists intact.
# This is an enumeration-test DT, not a validated Haiku kernel handoff.
set -euo pipefail

[[ $# -ge 3 ]] || { echo "Usage: $0 vendor.dtb working.dtb output.dtb [--sd-interrupt] [--firmware-pci-ranges]" >&2; exit 2; }
vendor=$1
reference=$2
output=$3
shift 3
sd_interrupt=
firmware_pci_ranges=0
for option in "$@"; do
	case "$option" in
		--sd-interrupt) sd_interrupt=$option ;;
		--firmware-pci-ranges) firmware_pci_ranges=1 ;;
		*) echo "Unknown option: $option" >&2; exit 2 ;;
	esac
done
[[ -f "$vendor" && -f "$reference" && ! -e "$output" ]] || {
	echo 'Inputs must exist and output must not exist' >&2; exit 1;
}
for tool in fdtget fdtput dtc shasum; do
	command -v "$tool" >/dev/null || exit 1
done
[[ $(shasum -a 256 "$vendor" | awk '{print $1}') == d57ec939b5d71513380eb4e4c8d5cb6efc15417c53f027bea7ae2b18500a138e ]] || {
	echo 'Unexpected vendor DTB revision' >&2; exit 1;
}
[[ $(fdtget -t x "$reference" /cpus timebase-frequency) == 2faf080 ]] || exit 1
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cp "$vendor" "$work/candidate.dtb"
# This selects OpenSBI's SG2042 platform override, which pre-registers one
# combined timer region. Without it, individual MTIMER ranges exhaust the
# root-domain region table before all 16 timers have been initialized.
[[ $(fdtget -t s "$reference" / compatible) == 'sophgo,mango sophgo,sg2042' ]] || exit 1
fdtput -t s "$work/candidate.dtb" / compatible sophgo,mango sophgo,sg2042
node=/soc/clint-mswi@7094000000
[[ $(fdtget -t s "$reference" "$node" compatible) == thead,c900-aclint-mswi ]] || exit 1
[[ $(fdtget -t x "$reference" "$node" reg) == "70 94000000 0 4000" ]] || exit 1
fdtput -t s "$work/candidate.dtb" "$node" compatible thead,c900-aclint-mswi

for ((index=0; index<16; index++)); do
	printf -v base '%x' "$((0xac000000 + index * 0x10000))"
	printf -v compare '%x' "$((0xac004000 + index * 0x10000))"
	node=/soc/clint-mtimer@70${base}
	[[ $(fdtget -t s "$reference" "$node" compatible) == 'sophgo,sg2042-aclint-mtimer thead,c900-aclint-mtimer' ]] || exit 1
	[[ $(fdtget -t s "$reference" "$node" reg-names) == mtimecmp ]] || exit 1
	[[ $(fdtget -t x "$reference" "$node" reg) == "70 $compare 0 c000" ]] || exit 1
	fdtput -t s "$work/candidate.dtb" "$node" compatible sophgo,sg2042-aclint-mtimer thead,c900-aclint-mtimer
	fdtput -t s "$work/candidate.dtb" "$node" reg-names mtimecmp
	fdtput -t x "$work/candidate.dtb" "$node" reg 70 "$compare" 0 c000
done

if [[ "$sd_interrupt" == --sd-interrupt ]]; then
	# The vendor firmware polls SD and omits this binding, but an OS needs
	# the complete hardware description. Use the known-working controller
	# interrupt, rebasing its parent to the vendor tree's own PLIC phandle.
	sd_node=/soc/bm-sd@704002b000
	old_sd_node=/soc/bm-sd@704002B000
	plic_node=/soc/interrupt-controller@7090000000
	[[ $(fdtget -t x "$reference" "$old_sd_node" reg) == '70 4002b000 0 1000' ]] || exit 1
	[[ $(fdtget -t x "$work/candidate.dtb" "$sd_node" reg) == '70 4002b000 0 1000' ]] || exit 1
	[[ $(fdtget -t x "$reference" "$old_sd_node" interrupts) == '88 4' ]] || exit 1
	[[ $(fdtget -t x "$reference" "$old_sd_node" interrupt-parent) == "$(fdtget -t x "$reference" "$plic_node" phandle)" ]] || exit 1
	[[ $(fdtget -t x "$reference" "$plic_node" '#interrupt-cells') == 2 ]] || exit 1
	[[ $(fdtget -t x "$work/candidate.dtb" "$plic_node" '#interrupt-cells') == 2 ]] || exit 1
	plic_phandle=$(fdtget -t x "$work/candidate.dtb" "$plic_node" phandle)
	[[ "$plic_phandle" =~ ^[0-9a-fA-F]+$ && "$plic_phandle" != 0 ]] || exit 1
	fdtput -t x "$work/candidate.dtb" "$sd_node" interrupt-parent "$plic_phandle"
	fdtput -t x "$work/candidate.dtb" "$sd_node" interrupts 88 4
	[[ $(fdtget -t x "$work/candidate.dtb" "$sd_node" interrupt-parent) == "$plic_phandle" ]] || exit 1
	[[ $(fdtget -t x "$work/candidate.dtb" "$sd_node" interrupts) == '88 4' ]] || exit 1
fi
if [[ "$firmware_pci_ranges" == 1 ]]; then
	# Pinned v1.4.3 PciHostBridgeLib uses PCD resource tables, not DT ranges.
	# Its serial ShowPciResource output confirms these three windows per root.
	# Describe those actual mappings; otherwise the OS reprograms outbound
	# windows that cannot reach the BARs allocated by firmware.
	for spec in '7060000000 0 40 42' '7062000000 2 48 4a' '7062800000 3 4c 4e'; do
		read -r controller domain host high <<< "$spec"
		node=/soc/pcie@$controller
		[[ $(fdtget -t x "$work/candidate.dtb" "$node" linux,pci-domain) == "$domain" ]] || exit 1
		[[ $(fdtget -t x "$work/candidate.dtb" "$node" bus-range) == '0 ff' ]] || exit 1
		fdtput -t x "$work/candidate.dtb" "$node" ranges \
			1000000 0 c0000000 "$host" c0000000 0 400000 \
			2000000 0 e0000000 "$host" e0000000 0 20000000 \
			43000000 "$high" 0 "$high" 0 2 0
	done
fi
# Leave space for ZSBL/OpenSBI fixups, including discovered memory nodes.
dtc -q -I dtb -O dtb -p 8192 -o "$output" "$work/candidate.dtb"
shasum -a 256 "$output"
