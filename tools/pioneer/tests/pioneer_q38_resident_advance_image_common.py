#!/usr/bin/env python3
"""TEST byte pins ONLY. Not an L1 reservation/native/silicon/Work grant."""
import hashlib
import pathlib
import sys
from pioneer_q38_resident_layer_link_image_common import PINS as LINK_PINS, verify as verify_link

PINS=tuple(p for p in LINK_PINS if p[0]!='text')+(
 ('text',128,'dc8cee871492f76e05537e61d69adaf457efb85ded96ca8990f61e51b7a4d00c'),
 ('advance_matrix',13776,'bb56967029a2a4ca9eae46d4297ae9a51ba65ba2ca765c63b2ed966a107c69f4'),
 ('advance',15624,'0b93f02b1aee70e60aa86e611ef963f61cce55ca304f96c6e57a586e6e8c857c'),
 ('advance_gather',4256,'9ecabb2c6bb436bbc7380a695e136ba879b13cbd83ce7e4161c09f07a6901f7b'),
)
def verify(image):
 image=pathlib.Path(image);peer=verify_link(image/'unchanged-link')
 for section,size,digest in PINS:
  data=(image/(section+'.bin')).read_bytes()
  assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,section
 return peer
if __name__=='__main__':
 assert len(sys.argv)==2
 verify(sys.argv[1])
