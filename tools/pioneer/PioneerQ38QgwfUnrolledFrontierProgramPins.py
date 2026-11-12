"""Pinned QGWF frontier ROOT and unrolled worker; experimental candidate only."""
from PioneerQ38QgwfFrontierProgramPins import ROOT_PINS, WORKER_PINS as OLD_WORKER

changes = {
    'globalworker.bin': (13240, 'c8cbc5692cbeb0ac1ff7204a27b2ec3c8c872d6a12d68c37744f7999e49edff7'),
    'program.elf': (162072, '14f5f416836f5dee5a07c400952cb0efe8d64ce70edf5d2112ef855632f4cc0d'),
}
WORKER_PINS = tuple((name, *changes[name]) if name in changes else (name, size, digest)
                    for name, size, digest in OLD_WORKER)
