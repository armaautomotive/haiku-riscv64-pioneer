#!/usr/bin/env python3
"""Check the isolated kernel builder rejects unverified source edits."""
import importlib.util
from pathlib import Path
import shutil
import sys
import tempfile

root = Path(__file__).resolve().parents[3]
if len(sys.argv) != 2:
    raise SystemExit('usage: pioneer_qwen35_pin_kernel_source_test.py SAVED_SOURCE_MIRROR')
mirror = Path(sys.argv[1]).resolve(strict=True)
script = root / 'tools/pioneer/build_qwen35_pin_kernel.py'
spec = importlib.util.spec_from_file_location('pioneer_pin_builder', script)
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)

with tempfile.TemporaryDirectory(prefix='pioneer-pin-source-') as tmp:
    workspace = Path(tmp)
    for name in (builder.DM, builder.MODULE, builder.CPU):
        destination = workspace / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / name, destination)
    builder.REPO = workspace
    builder.checked_sources(mirror)
    dm = workspace / builder.DM
    module = workspace / builder.MODULE
    cpu = workspace / builder.CPU
    original_dm = dm.read_text()
    original_module = module.read_text()
    original_cpu = cpu.read_text()
    lock = ('\tdevice_node** _node)\n{\n\tRecursiveLocker _(sLock);\n\n'
            '\tif ((parent == NULL && sRootNode != NULL) || moduleName == NULL)')
    assert original_dm.count(lock) == 1
    find_lock = ('find_child_node(device_node* parent, const device_attr* attributes,\n'
                 '\tdevice_node** _node)\n{\n\tRecursiveLocker _(sLock);\n\n')
    assert original_dm.count(find_lock) == 1
    retained_entry = ('#ifdef PIONEER_DEVICE_MANAGER_RETAINED_PIN_SERVICE\n'
                      '\t&gPioneerDeviceManagerRetainedPinModule,\n#endif\n')
    assert original_module.count(retained_entry) == 1
    cases = (
        (dm, original_dm.replace(lock, lock.replace('\tRecursiveLocker _(sLock);\n\n', ''), 1)),
        (dm, original_dm.replace(find_lock, find_lock.replace('\tRecursiveLocker _(sLock);\n\n', ''), 1)),
        (dm, original_dm.replace('// PIONEER_DM_REGISTER_NODE_BEGIN\n', '', 1)),
        (dm, original_dm.replace('PIONEER_DM_DRIVER_PIN_BEGIN', 'PIONEER_DM_DRIVER_PIN_CHANGED', 1)),
        (dm, original_dm.replace('static const char* sGenericContextPath;', 'static const char* sDifferentContextPath;', 1)),
        (module, original_module.replace(retained_entry, '', 1)),
        (cpu, original_cpu + '\n// unexpected change\n'),
    )
    for path, changed in cases:
        assert changed != path.read_text()
        path.write_text(changed)
        try:
            builder.checked_sources(mirror)
        except (RuntimeError, ValueError):
            pass
        else:
            raise AssertionError('builder accepted altered source: ' + str(path))
        dm.write_text(original_dm)
        module.write_text(original_module)
        cpu.write_text(original_cpu)
    builder.checked_sources(mirror)
print('PASS isolated pin-kernel source checks reject unverified changes')
