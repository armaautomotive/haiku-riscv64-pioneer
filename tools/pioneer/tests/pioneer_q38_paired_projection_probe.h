/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PAIRED_PROJECTION_PROBE_H
#define PIONEER_Q38_PAIRED_PROJECTION_PROBE_H
// HOST TEST canonical metadata only,never an execution/producer permission.
struct PioneerPairedPartProbe {unsigned bank,firstRow,groups,address,width,outputs;};
bool ProbePairedPart(unsigned layer,unsigned role,unsigned worker,PioneerPairedPartProbe&);
unsigned ProbePairedCoverage();
#endif
