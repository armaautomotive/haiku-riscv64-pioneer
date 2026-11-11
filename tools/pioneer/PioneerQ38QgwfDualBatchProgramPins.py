"""Pinned QGWF controller/worker pair for offline inspection only."""
from PioneerQ38QgweBatchDenseProgramPins import ROOT_PINS as OLD_ROOT, WORKER_PINS as OLD_WORKER

root_changes = {
    'text.bin': (129828, '408d88647b91f97b9d7e97c0769b75064cffe1274f164e730eb031acc0aba732'),
    'conv.bin': (19664, '510f0c73b326a50b11423b491b057edbc33902cc2bdb99b3186e8ab02acdcd1b'),
    'bf16.bin': (49124, 'a693e1cdec1b29f73b032ee1718f119b83a7a4004b439be571d603caa460914f'),
    'program.elf': (228980, '5257f8da3b692ae964a0f29e7d895d0274985c113a8137653c53c655b34d8454'),
}
worker_changes = {
    'globalworker.bin': (13164, '597c8b27af19a41122cf58655149dde4b7a2d0408fe5cbf781768546ee3d45c5'),
    'cachedfpu.bin': (14664, '21da384ca221c0f48d907348a414585035bee89cc60356c76d566f8f924dbff8'),
    'program.elf': (162072, 'c6f884e7a594d84832ce6691836ea10e5cffd10abf64ddab08f2b2023e462c0e'),
}
ROOT_PINS = tuple((name, *root_changes[name]) if name in root_changes else (name, size, digest)
                  for name, size, digest in OLD_ROOT)
WORKER_PINS = tuple((name, *worker_changes[name]) if name in worker_changes else (name, size, digest)
                    for name, size, digest in OLD_WORKER)
