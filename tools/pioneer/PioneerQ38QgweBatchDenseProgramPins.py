"""Pinned QGWE offline image pair; not native or device authority."""
from PioneerQ38QgwdDenseProgramPins import ROOT_PINS as OLD_ROOT, WORKER_PINS as OLD_WORKER

root_changes = {
    'text.bin': (131028, '1ebf298c1b7749bf7cef151f64c8b5d9deb81806a199b98970467c6bc35c3fbc'),
    'conv.bin': (19664, '05781d446e5fcd2dd1ae191a0a2492a2557db0f147b9d79d9e5e1a1afd802daf'),
    'bf16.bin': (47944, 'f7306e10ccb7038944bac695e605b134ac32bc7887d972fdd31459cc6de2723b'),
    'program.elf': (229056, 'a10e4e5c0971d94b3a60e1a730a5d58f3aeccea67ceacd4c076903a9d554505e'),
}
worker_changes = {
    'globalworker.bin': (13180, '7e7282754f9d50027893166429a972325124d461fcb3c9ce527dcc3d44af562f'),
    'cachedfpu.bin': (13756, '2212c2bd6d304df6c4ba7e97ee8e0db2683557f851c366545a312c03f3bc9973'),
    'program.elf': (161884, 'd3614997705d473f6a2a0c0f23dabcbf1418f587bf7686c3862bcd55c81aaa42'),
}
ROOT_PINS = tuple((name, *root_changes[name]) if name in root_changes else (name, size, digest)
                  for name, size, digest in OLD_ROOT)
WORKER_PINS = tuple((name, *worker_changes[name]) if name in worker_changes else (name, size, digest)
                    for name, size, digest in OLD_WORKER)
