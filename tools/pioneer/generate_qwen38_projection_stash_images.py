#!/usr/bin/env python3
"""Frozen QPS1 fixed-result ROOT + unchanged QFP3 peers,DATA only.

Reuse the complete six-section/boot validation and patch printer. All pins
must match before ANY output;never build/write/register/map/launch a device.
"""
import pathlib
import runpy
import sys

module = runpy.run_path(str(pathlib.Path(__file__).with_name('generate_qwen38_projection_graph_images.py')))
PINS = dict(module['PINS'])
PINS.update({
    'root-main': ('text', 7496, '7cc31b9383843f40e5521747120c1e43386f341cb6a19a6c82ce21100efcf82b', 'RootStashMain'),
    'root-prefix': ('prefix', 6564, '650e195c24403bc1d816d1c3a180db65a683911f5a52671c20f575f09e92f0ae', 'RootStashPrefix'),
    'root-graph': ('graph', 15640, '650c27791b198807f3e020cae6d25546c794fceda35912a1db42f5730c4f895a', 'RootStashGraph'),
})
main = module['main']
main.__globals__['PINS'] = PINS
if __name__ == '__main__':
    main(sys.argv[1:])
