"""Paired returning-service BUILD pins;not runtime or ownership authority."""
from PioneerQ38Bf16ProfileProgramPins import ROOT_PINS
from PioneerQ38Bf16ProgramPins import WORKER_PINS as OLD_WORKER_PINS

CHANGED={
 'globalworker.bin':(8892,'80c64e6e090a630b8505a6b89d0b1fc62ee629a560c0ed918056a92dcab0a6da'),
 'cachedfpu.bin':(8172,'3ec81a4ff2046387c8a3c2c3212a55a510d6d8c2e5bd02a5be137f340534f898'),
 'program.elf':(146988,'536d2b09214572116a05ada8927193506ab9056949d2e242e10a44854260f675'),
}
WORKER_PINS=tuple((name,*CHANGED.get(name,(size,digest))) for name,size,digest in OLD_WORKER_PINS)
