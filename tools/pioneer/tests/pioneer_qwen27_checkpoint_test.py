#!/usr/bin/env python3
"""Host-only path/pin refusals; not a compiler or hardware qualification."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "reproduce_qwen27_p100a_checkpoint.py"
SPEC = importlib.util.spec_from_file_location("checkpoint", SCRIPT)
CHECKPOINT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECKPOINT)


class CheckpointTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="pioneer-checkpoint-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.file = self.root / "artifact"
        self.file.write_bytes(b"frozen artifact\n")
        self.pin = (16, hashlib.sha256(self.file.read_bytes()).hexdigest())
        self.sdk = self.root / "sdk"
        compiler = self.sdk / "cross-tools-riscv64/bin/riscv64-unknown-haiku-g++"
        compiler.parent.mkdir(parents=True)
        compiler.touch()
        self.libraries = self.root / "libraries"
        self.libraries.mkdir()
        self.out = self.root / "new-output"

    def test_exact_artifact(self):
        result = CHECKPOINT.verify_artifact(self.file, self.pin)
        self.assertEqual(result, {"bytes": 16, "sha256": self.pin[1]})

    def test_changed_same_length_artifact(self):
        self.file.write_bytes(b"changedartifact\n")
        with self.assertRaises(RuntimeError):
            CHECKPOINT.verify_artifact(self.file, self.pin)

    def test_truncated_artifact(self):
        self.file.write_bytes(b"short")
        with self.assertRaises(RuntimeError):
            CHECKPOINT.verify_artifact(self.file, self.pin)

    def test_missing_artifact(self):
        with self.assertRaises(RuntimeError):
            CHECKPOINT.verify_artifact(self.root / "missing", self.pin)

    def test_symlink_artifact(self):
        link = self.root / "link"
        link.symlink_to(self.file)
        with self.assertRaises(RuntimeError):
            CHECKPOINT.verify_artifact(link, self.pin)

    def test_fresh_output(self):
        CHECKPOINT.validate_paths(self.sdk, self.libraries, self.out)
        self.assertFalse(self.out.exists())

    def test_existing_output_preserved(self):
        self.out.mkdir()
        sentinel = self.out / "sentinel"
        sentinel.write_bytes(b"keep")
        with self.assertRaises(RuntimeError):
            CHECKPOINT.validate_paths(self.sdk, self.libraries, self.out)
        self.assertEqual(sentinel.read_bytes(), b"keep")

    def test_dangling_output_symlink(self):
        self.out.symlink_to(self.root / "absent")
        with self.assertRaises(RuntimeError):
            CHECKPOINT.validate_paths(self.sdk, self.libraries, self.out)
        self.assertTrue(self.out.is_symlink())

    def test_missing_sdk(self):
        with self.assertRaises(RuntimeError):
            CHECKPOINT.validate_paths(self.root / "absent", self.libraries, self.out)
        self.assertFalse(self.out.exists())

    def test_relative_output(self):
        with self.assertRaises(RuntimeError):
            CHECKPOINT.validate_paths(self.sdk, self.libraries, Path("relative"))


if __name__ == "__main__":
    unittest.main()
