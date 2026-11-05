#!/usr/bin/env python3
"""Pinned current prepared ROOT clock helpers;no native/device access."""
import hashlib,os,pathlib,subprocess,sys,tempfile
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root=pathlib.Path(sys.argv[1]);here=pathlib.Path(__file__).resolve().parent
assert root.is_absolute() and root.is_dir() and not root.is_symlink()
PINS=(
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',128860,'b864a51b814558183aa3154759ca9820d4491c6651da9f17b93f684583eace38'),
 ('terminal.bin',8,'468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
 ('conv.bin',32692,'6ff42c5bf9dd25181639d495e042aff1203a95abc6a8a34cbdcac32052c5bd9d'),
 ('prepared.bin',5744,'30edf0fce5c4734d8b654a1dcc82b945f27ae314d9eeef2e5efe23a8453c1ebe'),
 ('program.elf',198204,'74cae45049174dd53c96203104a0b4b501f757b2210b2de86da5507334ebacc5'),
)
snapshots={}
for name,size,digest in PINS:
 path=root/name;assert path.is_file() and not path.is_symlink()
 data=path.read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
 snapshots[name]=data
assert hashlib.sha256((root/'entire-unprofiled-prepared-root.elf').read_bytes()).hexdigest()=='38a8f64f9bf75946953d87ec08d3bba07b44ea36303936ab912c89771f68d039'
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
with tempfile.TemporaryDirectory(prefix='pioneer-prepared-profile-clock-',dir='/private/tmp') as name:
 private=pathlib.Path(name);image=private/'image';image.mkdir(mode=0o700)
 for filename,data in snapshots.items():(image/filename).write_bytes(data);(image/filename).chmod(0o400)
 symbols=subprocess.check_output([tool+'nm','-n',str(image/'program.elf')],text=True)
 assert symbols==(root/'program.symbols').read_text();(image/'program.symbols').write_text(symbols)
 for section in ('boot','text','terminal','conv','prepared'):
  out=private/(section+'.bin')
  subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(image/'program.elf'),str(out)],check=True)
  assert out.read_bytes()==snapshots[section+'.bin']
 binary=private/'clock'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_token_phase_profile_image_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(image)],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
 # Existing prepared native/program selections must reject this distinct
 # entire ROOT before touching a worker path or creating any header.
 for counter in (False,True):
  output=private/('refused-'+str(counter)+'.h')
  result=subprocess.run([sys.executable,str(here.parent/'generate_qwen38_prepared_q4_full_offload_programs.py'),
   str(image),str(private/'MUST_NOT_OPEN_WORKER'),str(output),*(['--counter-unroll'] if counter else [])],capture_output=True,text=True)
  assert result.returncode and 'AssertionError' in result.stderr and 'FileNotFoundError' not in result.stderr and not output.exists()
 for filename,data in snapshots.items():assert (root/filename).read_bytes()==data
print('PASS_PREPARED_PROFILE_IMAGE six pinned sections/ELF and original entire counter ROOT authenticated;'
 'actual linked clock helpers pass;both existing prepared program families refuse before worker IO;'
 'NO native ownership,physical timing,clock frequency,model quality,installation or TPS grant',flush=True)
