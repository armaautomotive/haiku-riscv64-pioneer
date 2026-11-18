/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PRELOAD_H
#define PIONEER_QWEN35_PRELOAD_H

#include "PioneerQwen35DeviceLayoutBuild.h"
#include <cmath>

namespace PioneerQwen35 {

constexpr unsigned LoadPageBytes=4096;
constexpr unsigned LoadSpanBytes=1024*1024;
constexpr float RopeBase=10000000.0f;

// One text position supplies all four MRoPE coordinates. The iterative step
// follows ggml_mrope_cache_init for the 64 rotated channels of this model.
inline bool FillRope(Range rope, unsigned offset, unsigned bytes, uint8_t* output)
{
	constexpr unsigned rowBytes=Rotary*sizeof(float);
	if (!output || !bytes || bytes>LoadSpanBytes || ((offset|bytes)&(LoadPageBytes-1))
		|| !rope.bytes || rope.bytes>MaxPositions*rowBytes || rope.bytes%rowBytes)
		return false;
	const unsigned allocated=(rope.bytes+LoadPageBytes-1)&~(LoadPageBytes-1);
	if (offset>=allocated || bytes>allocated-offset) return false;
	const float step=std::pow(RopeBase,-2.0f/Rotary);
	memset(output,0,bytes);
	for (unsigned at=0; at<bytes && offset+at<rope.bytes; at+=rowBytes) {
		float angle=float((offset+at)/rowBytes);
		for (unsigned lane=0; lane<Rotary; lane+=2) {
			const float pair[]={std::cos(angle),std::sin(angle)};
			memcpy(output+at+lane*sizeof(float),pair,sizeof(pair));
			angle*=step;
		}
	}
	return true;
}

inline bool ResolveRope(const DeviceLayout& manifest, const PioneerQwen35PackedPlan& plan,
	Range& output)
{
	if (!plan.Ready() || manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
		|| !manifest.context || manifest.context>MaxPositions || manifest.reserved)
		return false;
	const unsigned bytes=manifest.context*Rotary*sizeof(float);
	for (const auto& part:plan.Parts()) {
		if (part.kind!=PIONEER_Q35_WORKSPACE || part.bank!=0) continue;
		if (part.allocated>=bytes && !(part.proposedAddress&(LoadPageBytes-1))
			&& part.proposedAddress>=BankBase && part.proposedAddress<BankLimit
			&& part.allocated<=BankLimit-part.proposedAddress) {
			output={part.bank,part.proposedAddress,bytes};
			return true;
		}
		return false;
	}
	return false;
}

// The caller authenticates the full file on the same open handle and owns the
// resident session. The upload operation accepts logical bank offsets and
// returns success only after the device has proved the span and parked.
template<class Upload>
bool UploadModel(PioneerQwen35Gguf& source, const PioneerQwen35PackedPlan& plan,
	const DeviceLayout& manifest, uint8_t* buffer, unsigned capacity, Upload&& upload,
	uint64_t& uploadedBytes)
{
	Range rope{};
	uploadedBytes=0;
	if (!buffer || capacity<LoadSpanBytes || !plan.MatchesSource(source)
		|| !ResolveRope(manifest,plan,rope)) return false;
	const auto& parts=plan.Parts();
	for (unsigned fragment=0; fragment<parts.size(); fragment++) {
		const auto& part=parts[fragment];
		if (part.kind!=PIONEER_Q35_WEIGHT) continue;
		for (unsigned offset=0; offset<part.allocated;) {
			const unsigned bytes=std::min(LoadSpanBytes,part.allocated-offset);
			for (unsigned at=0; at<bytes; at+=LoadPageBytes)
				if (!plan.ReadWeightPage(source,fragment,(offset+at)/LoadPageBytes,
					buffer+at,LoadPageBytes)) return false;
			if (!source.Stable() || !upload(part.bank,part.proposedAddress-BankBase+offset,bytes,buffer))
				return false;
			uploadedBytes+=bytes;
			offset+=bytes;
		}
	}
	const unsigned allocated=(rope.bytes+LoadPageBytes-1)&~(LoadPageBytes-1);
	for (unsigned offset=0; offset<allocated;) {
		const unsigned bytes=std::min(LoadSpanBytes,allocated-offset);
		if (!FillRope(rope,offset,bytes,buffer) || !source.Stable()
			|| !upload(rope.bank,rope.address-BankBase+offset,bytes,buffer))
			return false;
		uploadedBytes+=bytes;
		offset+=bytes;
	}
	return plan.MatchesSource(source);
}
}
#endif
