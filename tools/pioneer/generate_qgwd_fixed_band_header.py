#!/usr/bin/env python3
"""Encode the independently pinned fixed BFP8 diagnostic image; never install."""
import hashlib
from pathlib import Path
import struct
import sys
if len(sys.argv)!=3:raise SystemExit('usage: absolute-image-directory absolute-new-header')
image,output=map(Path,sys.argv[1:])
if not image.is_absolute() or not image.is_dir() or not output.is_absolute() or output.exists() or output.is_symlink():
    raise SystemExit('image must exist and output must be new')
boot=(image/'boot.bin').read_bytes();text=(image/'text.bin').read_bytes()
if (len(boot)!=4 or len(text)!=2096 or hashlib.sha256(boot).hexdigest()!='d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
    or hashlib.sha256(text).hexdigest()!='f69c2803d854bbef87f37df7cb96536e1f5d940060ed98fec6cb3200d50a9ad5'):
    raise SystemExit('fixed physical-image identity changed')
words=struct.unpack('<'+'I'*(len(text)//4),text)
lines=['/* Independently pinned fixed-K BFP8 diagnostic image; not Work. */',
       '#ifndef PIONEER_NATIVE_BFP_BAND_GENERATED_IMAGE_H','#define PIONEER_NATIVE_BFP_BAND_GENERATED_IMAGE_H',
       'template<unsigned K> struct PioneerNativeBfpBandGeneratedImage;',
       'template<> struct PioneerNativeBfpBandGeneratedImage<5120> {',
       ' static constexpr unsigned Width=5120,Phases=4,Boot=0x%08x;'%struct.unpack('<I',boot)[0],
       ' static unsigned Word(unsigned index){static const unsigned code[]={']
for start in range(0,len(words),8):lines.append('  '+','.join('0x%08x'%w for w in words[start:start+8])+',')
lines+=[' };return index<sizeof(code)/4?code[index]:0;}','};','#endif']
with output.open('x') as f:f.write('\n'.join(lines)+'\n')
print('HEADER_SHA256',hashlib.sha256(output.read_bytes()).hexdigest())
print('PINNED IMAGE DATA ONLY; no driver, device or speed claim')
