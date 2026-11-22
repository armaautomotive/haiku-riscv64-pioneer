#!/usr/bin/env python3
"""Check the real RV32 writer image waits for L1 before its NoC doorbell."""

from pathlib import Path
import re
import sys


def instructions(path):
    result = []
    for line in Path(path).read_text().splitlines():
        match = re.match(r"\s*[0-9a-f]+:\s+[0-9a-f]+\s+(\S+)\s*(.*?)\s*(?:#.*)?$", line)
        if match:
            result.append((match.group(1), match.group(2).replace(" ", "")))
    return result


def has_ordered_noc1_write(code):
    for i in range(len(code) - 3):
        if code[i] != ("fence", "rw,rw") or code[i + 1][0] != "lw":
            continue
        source = code[i + 1][1].split(",", 1)[0]
        if code[i + 2] not in (("mv", f"zero,{source}"), ("addi", f"zero,{source},0")):
            continue
        # Follow the small address-setup sequence to the actual NoC1
        # doorbell; a stack spill in this interval is not a doorbell write.
        registers = {}
        for op, args in code[i + 3 : i + 15]:
            fields = args.split(",")
            if op == "lui" and len(fields) == 2:
                registers[fields[0]] = int(fields[1], 0) << 12
            elif op == "li" and len(fields) == 2:
                registers[fields[0]] = int(fields[1], 0)
            elif op == "mv" and len(fields) == 2:
                registers[fields[0]] = registers.get(fields[1])
            elif op in ("add", "addi") and len(fields) == 3 and fields[1] in registers:
                try:
                    registers[fields[0]] = registers[fields[1]] + int(fields[2], 0)
                except (ValueError, TypeError):
                    registers[fields[0]] = None
            elif op == "sw" and len(fields) == 2:
                target = re.fullmatch(r"(-?\d+)\((\w+)\)", fields[1])
                if target and registers.get(target.group(2)) is not None:
                    address = registers[target.group(2)] + int(target.group(1))
                    if address == 0xFFB30040 and registers.get(fields[0]) == 1:
                        return True
    return False


if len(sys.argv) != 2 or not has_ordered_noc1_write(instructions(sys.argv[1])):
    raise SystemExit("writer image lacks dependent L1 read before NoC1 write")
print("PASS writer image waits for L1 source read before NoC1 doorbell; OFFLINE ONLY")
