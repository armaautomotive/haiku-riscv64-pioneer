#!/usr/bin/env python3
"""TEST byte pins only; NOT reservation, native, predecessor or speed authority."""
import hashlib
import pathlib
import sys
from pioneer_q38_resident_advance_image_common import PINS as ADVANCE_PINS,verify as verify_advance
PINS=tuple(p for p in ADVANCE_PINS if p[0]!='text')+(
 ('text',128,'ba57fa1835372b8a7769ec605bdf6ab68b6cbaafda2bab3e3052886be464c766'),
 ('continue',31992,'2f8a6dda08c67daeb543cab817713d940ec15f5cdc63a06477950e9dc9f0a5fe'),
)
def verify(image):
 image=pathlib.Path(image);peer=verify_advance(image/'unchanged-advance')
 for section,size,digest in PINS:
  data=(image/(section+'.bin')).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,section
 return peer
if __name__=='__main__':
 assert len(sys.argv)==2
 verify(sys.argv[1])
