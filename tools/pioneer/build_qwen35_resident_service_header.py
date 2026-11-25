#!/usr/bin/env python3
"""Turn the offline resident service witness into a guarded driver header.

This only packages an already linked witness; it does not validate the graph,
install a driver, or authorize any access to the card.
"""

import argparse
from pathlib import Path
from struct import iter_unpack, unpack


def generate(directory: Path) -> str:
    sharing = (directory / 'activation-sharing.marker').is_file()
    boot = (directory / 'boot.bin').read_bytes()
    text = (directory / 'text.bin').read_bytes()
    dispatch = (directory / 'dispatch.bin').read_bytes()
    if boot != bytes.fromhex('6fc00600'):
        raise ValueError('unexpected resident boot jump')
    if not 0 < len(text) <= 0xe000 or len(text) % 4:
        raise ValueError('resident text does not fit its aligned L1 range')
    if not 0 < len(dispatch) <= 0x6400 or len(dispatch) % 4:
        raise ValueError('resident dispatch does not fit its aligned L1 range')

    symbols = {}
    for row in (directory / 'symbols.txt').read_text(encoding='ascii').splitlines():
        parts = row.split()
        if len(parts) == 3 and parts[1] == 'T':
            value, _, name = parts
            if name in symbols:
                raise ValueError('duplicate text symbol: ' + name)
            symbols[name] = int(value, 16)
    if symbols.get('qwen35_dataflow_entry') != 0x72000:
        raise ValueError('unexpected resident entry address')
    park = symbols.get('qwen35_resident_graph_stopped_park', 0)
    offset = park - 0x72000
    if offset < 0 or offset % 4 or offset + 4 > len(text) or text[offset:offset + 4] != bytes.fromhex('6f000000'):
        raise ValueError('unexpected resident stopped-park instruction')
    for name in ('qwen35_resident_graph_service_main', 'qwen35_graph_run_descriptor',
                 'qwen35_shared_compute_coordinate', 'qwen35_graph_execute'):
        address = symbols.get(name, 0)
        if address % 4 or not (0x72000 <= address < 0x72000 + len(text)
                                or 0xa0400 <= address < 0xa0400 + len(dispatch)):
            raise ValueError('missing or out-of-range resident text symbol: ' + name)

    def words(data: bytes) -> str:
        return ''.join(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I', data))

    return (
        '#pragma once\n'
        '#if !defined(PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS) || '
        '!defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || '
        '!defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) || '
        '!defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || '
        + ('!defined(PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS) || ' if sharing
           else 'defined(PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS) || ')
        + 'defined(PIONEER_QWEN35_RECOVERY_QUALIFY) || '
        'defined(PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL)\n'
        '#error "resident service image is restricted to the non-loadable snapshot witness"\n'
        '#endif\nnamespace PioneerQwen35 {\n'
        f'inline constexpr unsigned DataflowServiceImageBootWord=0x{unpack("<I", boot)[0]:08x}U;\n'
        f'inline constexpr unsigned DataflowServiceStoppedParkAddress=0x{park:08x}U;\n'
        'inline constexpr unsigned DataflowServiceTextWords[]={\n' + words(text) + '};\n'
        'inline constexpr unsigned DataflowServiceDispatchWords[]={\n' + words(dispatch) + '};\n}\n'
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('header', type=Path)
    args = parser.parse_args()
    header = generate(args.image)
    with args.header.open('x', encoding='ascii', newline='\n') as output:
        output.write(header)


if __name__ == '__main__':
    main()
