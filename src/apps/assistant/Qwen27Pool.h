/* SPDX-License-Identifier: MIT */
#ifndef ASSISTANT_QWEN27_POOL_H
#define ASSISTANT_QWEN27_POOL_H
namespace Assistant27 {
// llama_attach_threadpool updates the context, NOT its CPU backend. That
// backend pauses its PREVIOUS pool at the next graph_compute. Retain that
// pool until a successful synchronized decode has bound the replacement.
// At most two pools: repeated idle changes free only never-bound pools.
template<typename Handle,void (*Release)(Handle)> class PoolOwner {
 Handle current=Handle(),bound=Handle();
public:
 PoolOwner()=default;
 PoolOwner(const PoolOwner&)=delete;
 PoolOwner& operator=(const PoolOwner&)=delete;
 Handle Current() const { return current; }
 bool NeedsRetirement() const { return bound && bound!=current; }
 bool Replace(Handle replacement) {
  if (!replacement || replacement==current || replacement==bound) return false;
  Handle previous=current; current=replacement;
  if (previous && previous!=bound) Release(previous);
  return true;
 }
 void AfterSuccessfulDecode() {
  if (bound && bound!=current) Release(bound);
  bound=current;
 }
 // Caller has freed the llama context/CPU backend FIRST, including failed
 // decode paths where the old pool may still be referenced by that backend.
 void AfterContextFree() {
  if (current) Release(current);
  if (bound && bound!=current) Release(bound);
  current=bound=Handle();
 }
};
}
#endif
