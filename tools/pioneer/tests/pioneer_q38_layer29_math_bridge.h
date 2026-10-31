/* SPDX-License-Identifier: MIT */
#pragma once
// HOST diagnostic scalar math only. No model, device, launch or owner API.
bool Q38Layer29CardConv(const float*,const float*,float*);
bool Q38Layer29CardPrepare(const float*,const float*,float,float,float,float,float*,float*,float*);
bool Q38Layer29CardPreparePrecise(const float*,const float*,float,float,float,float,float*,float*,float*);
bool Q38Layer29CardGated(const float*,const float*,const float*,float*);
