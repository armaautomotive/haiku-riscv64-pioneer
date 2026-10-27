/* SPDX-License-Identifier: MIT */
return B_NOT_ALLOWED; // existing root/FD admission refusal from the enclosing if
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
// Private renamed compile BODY retains the real bank/clock/FD constructors,
// so native scope checks cannot fold to constant false. No mixed launch,
// legacy compute or new public opcode exists. No conventional driver ABI.
if(operation==TT_PCI_PROBE_RELOAD_STATUS || operation==TT_PCI_PROBE_RELOAD_QUIESCE)
 return DriverReloadControl(instance,operation,data,length);
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE
if(operation==TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE)
 return Q38MixedLiveValidationControl(instance,operation,data,length);
#endif
if(operation==TT_PCI_PROBE_Q38_BANK_SUPPORT || operation==TT_PCI_PROBE_Q38_BANK_BEGIN || operation==TT_PCI_PROBE_Q38_BANK_END)
 return Q38BankControl(instance,operation,data,length);
if(operation==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || operation==TT_PCI_PROBE_Q38_SPAN_END)
 return Q38SpanControl(instance,operation,data,length);
return B_NOT_ALLOWED;
#endif
