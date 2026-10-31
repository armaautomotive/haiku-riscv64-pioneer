/* SPDX-License-Identifier: MIT */
#pragma once
// HOST diagnostic scalar math only. No model, device, launch or owner API.
bool Q38Layer29CardConv(const float*,const float*,float*);
bool Q38Layer29CardPrepare(const float*,const float*,float,float,float,float,float*,float*,float*);
bool Q38Layer29CardPreparePrecise(const float*,const float*,float,float,float,float,float*,float*,float*);
bool Q38Layer29CardGated(const float*,const float*,const float*,float*);
// Actual card projection quantizer on ONE local 256-value block. Output
// has 256 decoded integer lanes and eight (Q8_0) or one (Q8_K) scales.
// Component diagnostic only; never supplies products to a native history.
bool Q38Layer29CardQuantize(unsigned,const float*,int*,float*);
