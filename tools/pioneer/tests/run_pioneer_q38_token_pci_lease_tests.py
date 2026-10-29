#!/usr/bin/env python3
"""PCI-journal endpoint DATA qualification,not native cleanup authority."""
import os
import pathlib
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-token-pci-',dir='/private/tmp') as directory:
    binary = pathlib.Path(directory)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_token_pci_lease_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
