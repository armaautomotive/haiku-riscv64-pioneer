"""Pinned QGWF frontier ROOT with its existing dual-batch worker; offline only."""
from PioneerQ38QgwfDualBatchProgramPins import ROOT_PINS as OLD_ROOT, WORKER_PINS

changes = {
    'text.bin': (129864, '499a4540977ff4860788be21534d8aa56816f40cded2e8a3bab62cb6e1758a65'),
    'conv.bin': (19632, '15ebb8a550bc96651bfbe254b21606dae474980309136360667c28e1473938c7'),
    'bf16.bin': (49124, '90ddecadd664bb85dd56d5540eff627615a8fa7488b7b29603c6c5556ef839d9'),
    'program.elf': (229016, 'e50ed27fba25611700a3a93b569a37145ad1ea400718ec7707326198ed560665'),
}
ROOT_PINS = tuple((name, *changes[name]) if name in changes else (name, size, digest)
                  for name, size, digest in OLD_ROOT)
