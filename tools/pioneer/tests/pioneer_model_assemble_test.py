#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


class AssembleTests(unittest.TestCase):
    def test_exact_contents_and_fail_closed_inputs(self):
        with tempfile.TemporaryDirectory(prefix="pioneer-model-assemble-") as directory:
            root=Path(directory)
            binary=root/"test"
            source=Path(__file__).resolve().parents[1]/"pioneer_model_assemble.cpp"
            subprocess.run(["c++","-std=c++11","-O1","-g","-Wall","-Wextra","-Werror",
                "-fsanitize=address,undefined","-DPIONEER_ASSEMBLE_TEST_BYTES=1024",str(source),"-o",str(binary)],check=True)
            data=bytes(range(256))*4
            pieces=[root/f"part{i}" for i in range(3)]
            for path,payload in zip(pieces,(data[:255],data[255:555],data[555:])):
                path.write_bytes(payload)
            output=root/"output"
            def run(out,inputs):
                return subprocess.run([str(binary),str(out)]+[str(p) for p in inputs],capture_output=True,
                    env=dict(os.environ,UBSAN_OPTIONS="halt_on_error=1"))
            result=run(output,pieces)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual(output.read_bytes(),data)
            self.assertEqual(output.stat().st_mode&0o777,0o600)
            self.assertNotEqual(run(output,pieces).returncode,0)
            self.assertEqual(output.read_bytes(),data)
            link=root/"link"
            link.symlink_to(pieces[0])
            self.assertNotEqual(run(root/"bad-link",[link]+pieces[1:]).returncode,0)
            self.assertFalse((root/"bad-link").exists())
            target=root/"target"
            target.write_bytes(b"preserve")
            destination_link=root/"destination-link"
            destination_link.symlink_to(target)
            self.assertNotEqual(run(destination_link,pieces).returncode,0)
            self.assertEqual(target.read_bytes(),b"preserve")
            for payload in (b"",b"short",data+b"oversized"):
                pieces[0].write_bytes(payload)
                self.assertNotEqual(run(root/"invalid",pieces).returncode,0)
                self.assertFalse((root/"invalid").exists())


if __name__ == "__main__":
    unittest.main()
