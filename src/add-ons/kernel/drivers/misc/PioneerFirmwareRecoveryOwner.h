/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_FIRMWARE_RECOVERY_OWNER_H
#define PIONEER_FIRMWARE_RECOVERY_OWNER_H

// The caller owns the pre-reset mapping and must retain this owner together
// with the device session until verified retirement. Guard references are
// borrowed only for each method call, while the driver lock is held.
template<class OwnerIO,class Guard> class PioneerFirmwareRecoveryOwner {
	using Transport=PioneerFirmwareResetTransport<OwnerIO,Guard>;
	Transport transport;
	PioneerFirmwareReset<Transport> reset;
	uint64 ticket;
	bool startAttempted=false;
public:
	PioneerFirmwareRecoveryOwner(PioneerPostResetMapping& mapping,OwnerIO& io,uint64 recoveryTicket)
		:transport(mapping,io,NULL,recoveryTicket),reset(transport),ticket(recoveryTicket) {}
	PioneerFirmwareRecoveryOwner(const PioneerFirmwareRecoveryOwner&)=delete;
	PioneerFirmwareRecoveryOwner& operator=(const PioneerFirmwareRecoveryOwner&)=delete;
	~PioneerFirmwareRecoveryOwner() = default; // no device IO or unmap
	bool SetTrace(PioneerFirmwareResetTrace callback,void* context) {
		return !startAttempted && transport.SetTrace(callback,context);
	}

	bool Start(Guard* guard,uint64 expected,const tt_arc_scratch_result& arc,
		const tt_arc_telemetry_result& health) {
		if (startAttempted || expected!=ticket || !transport.BindGuard(guard,expected)) return false;
		startAttempted=true;
		const bool started=transport.Open() && reset.Start(arc,health);
		transport.DetachGuard();return started;
	}
	bool Resume(Guard* guard,uint64 expected,unsigned polls) {
		if (!startAttempted || expected!=ticket || !transport.BindGuard(guard,expected)) return false;
		const bool done=reset.ResumeWait(polls);
		transport.DetachGuard();return done;
	}
	status_t Retire(Guard* guard,uint64 expected,const PioneerDriverRecoveryProof& proof) {
		if (!startAttempted || expected!=ticket || !transport.BindGuard(guard,expected)) return B_BAD_VALUE;
		const status_t status=transport.DisposeAfterVerifiedReset(proof,reset);
		transport.DetachGuard();return status;
	}
	const PioneerFirmwareResetResult& Result() const { return reset.Result(); }
	bool Retired() const { return transport.DisposalComplete(); }
};
#endif
