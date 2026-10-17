#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
project=$directory/../../..
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-rtl-serialize.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/rtl_serialize_level.inc" "$temporary/rtl_serialize_helpers.inc" "$temporary/rtl_serialize_handler.inc" "$temporary/rtl_serialize_establish.inc"; rmdir "$temporary"' EXIT HUP INT TERM
source=$project/src/add-ons/kernel/drivers/network/ether/rtl8125/dev/pci/if_rge.c
for section in LEVEL HELPERS; do
 case "$section" in LEVEL) output=level;; HELPERS) output=helpers;; esac
 awk -v first="PIONEER_RTL_SERIALIZE_${section}_BEGIN" -v last="PIONEER_RTL_SERIALIZE_${section}_END" '
 index($0,first) {copying=1;next} index($0,last) {found=1;exit}
 copying {print} END {if(!found)exit 1}' "$source" > "$temporary/rtl_serialize_$output.inc"
done
awk '/^intr_handler\(void \*data\)/ {print "static int32";copying=1} copying {print} copying && /^}$/ {found=1;exit} END {if(!found)exit 1}' \
 "$project/src/libs/compat/freebsd_network/bus.cpp" > "$temporary/rtl_serialize_handler.inc"
awk '/^pci_intr_establish_openbsd\(/ {print "static void*";copying=1} copying {print} copying && /^}$/ {found=1;exit} END {if(!found)exit 1}' \
 "$project/src/libs/compat/openbsd_network/compat/dev/pci/pcivar.h" > "$temporary/rtl_serialize_establish.inc"
for profile in enabled disabled foreign_os foreign_arch; do
 case "$profile" in
  enabled) flags='-D__HAIKU__ -D__riscv -DHAIKU_PIONEER_RTL_SERIALIZE=1';;
  disabled) flags='-D__HAIKU__ -D__riscv';;
  foreign_os) flags='-D__riscv -DHAIKU_PIONEER_RTL_SERIALIZE=1';;
  foreign_arch) flags='-D__HAIKU__ -DHAIKU_PIONEER_RTL_SERIALIZE=1';;
 esac
 ${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter \
  -fsanitize=address,undefined $flags -I "$temporary" \
  "$directory/pioneer_rtl_serialize_test.cpp" -o "$temporary/test"
 "$temporary/test"
done
