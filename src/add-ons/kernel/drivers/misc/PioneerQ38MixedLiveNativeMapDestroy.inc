/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
for(auto area:q38MixedLiveMemoryArea)if(area>=B_OK)delete_area(area); // clean destruction only,not quarantine cleanup
#endif
for(unsigned i=0;i<5;i++)if(runtimeAreas[i]>=B_OK)delete_area(runtimeAreas[i]);
