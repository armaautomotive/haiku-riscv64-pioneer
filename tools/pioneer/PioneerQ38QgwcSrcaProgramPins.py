"""Pinned corrected SrcA ROOT and worker; data inspection, not launch authority."""
from PioneerQ38Qgw8ProgramPins import WORKER_PINS as QGW8_WORKER_PINS

ROOT_PINS = (
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',129480,'6f715be833854c950067b45431c7c4a0e8679b57801facb524440d8858526cf9'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',19664,'ddb5d683a42d12168d4fa48f3580ef018d8f6c96cee616aafa3c216466522ad4'),
 ('bf16.bin',39716,'148a1d0897d07b69f0cd03d553a4f424e0da96f5cba4038ae8e0d991ecaf5f50'),
 ('program.elf',220156,'d44e18940bcd37cf35bc2f124b314843cfd99314e8aaebd5d4983eb074918eb0'),
)
_changed={
 'globalworker.bin':(11948,'f65873029aaaa2493c6eb1ec33297080a4c4292b0a7bf76d0f8252cbf5a42214'),
 'dense.bin':(14468,'8965216d2e186982204a4f304b62cc7254911e5c463c61b6bb03cea15fc6c810'),
 'program.elf':(157308,'3450e5b2d46b27e9ac3f81cb62de79d53b0613d85af0acc03a73a149e812e09e'),
}
WORKER_PINS=tuple((name,*_changed[name]) if name in _changed else (name,size,digest)
                  for name,size,digest in QGW8_WORKER_PINS)
