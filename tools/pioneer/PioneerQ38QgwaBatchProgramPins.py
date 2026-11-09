"""Pinned QGWA batched ROOT and worker code; offline DATA only."""
from PioneerQ38Qgw8ProgramPins import WORKER_PINS as QGW8_WORKER_PINS

ROOT_PINS = (
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',129480,'9645df311faf71b9a34f257b36740785a2c36934752b273dc56c6fd830db203f'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',19664,'ff367c1ab96337b40216b961f67231f0fab60d95ae01acc924564547b955574f'),
 ('bf16.bin',39324,'e42c522785f379490f85c028894b12b75064a25eff8a7ab3b1277c888d987fe2'),
 ('program.elf',220156,'99a0dc86b01efa012323b8602668e74b757547927a60a1a4b8808df76a9c07de'),
)
_changed={
 'globalworker.bin':(11988,'894747f7bf2a282438932a7dc3b56003d9ec64abcda30914b0fd891b70034207'),
 'cachedfpu.bin':(13756,'46b4b2e86bc7b09383a3945ba5241aa8745d6939c0868d73636fe71de0913753'),
 'program.elf':(157516,'e0d7bb5a1434181a8560867f1bfee701a7787a878dff780bce215adf79f2fc47'),
}
WORKER_PINS=tuple((name,*_changed[name]) if name in _changed else (name,size,digest)
                  for name,size,digest in QGW8_WORKER_PINS)
