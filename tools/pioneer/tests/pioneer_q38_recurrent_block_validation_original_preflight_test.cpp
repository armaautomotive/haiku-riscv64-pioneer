/* SPDX-License-Identifier: MIT */
// Actual complete authenticated c600 source/reference and actual client
// preflight. Open is deliberately unavailable;NO device FD/context/IO.
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38RecurrentBlockValidationSession.h"
#include <cassert>
#include <cstdio>
struct RecurrentNoDeviceIO {
 PioneerQ38RecurrentBlockOriginalSource& source;unsigned opens=0;
 explicit RecurrentNoDeviceIO(PioneerQ38RecurrentBlockOriginalSource& s):source(s){}
 int Open(){assert(source.Ready() && source.ReferenceReads()==3853 && source.Spans()==57 && !opens++);return -1;}
 bool Call(int,unsigned,void*,size_t){assert(!"NO device call is allowed in preflight qualification");return false;}
 int Close(int){assert(!"NO device FD was opened");return -1;}
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 auto source=OpenPioneerQ38RecurrentBlockOriginalSource(argv[1]);assert(source && !source->Ready());RecurrentNoDeviceIO io(*source);
 auto session=std::make_unique<PioneerQ38RecurrentBlockValidationSession<RecurrentNoDeviceIO>>(io,*source);
 assert(!session->Validate(1) && !source->Ready() && !io.opens);
 assert(!session->Validate() && session->failure && !strcmp(session->failure,"open") && io.opens==1
  && source->Ready() && source->ReferenceReads()==3853 && source->Spans()==57 && source->Pages()==68603 && source->Bytes()==280971136
  && !session->uploaded_pages && !session->upload_spans && !session->reply_captured && !session->comparison.checked);
 session->Abort();assert(!session->Validate() && !session->Finish() && io.opens==1);
 printf("PASS actual DEBUG72 client ORIGINAL fullc600 authentication/private3853-read reference+seal/ALL57 raw extent routes+addresses+no-overlap+68603pages280971136bytes BEFORE unavailable Open/private fingerprint%016llx;NO device FD,context,upload,compute,native/physical/full-model/TPS qualification\n",
  (unsigned long long)source->ReferenceFingerprint());
}
