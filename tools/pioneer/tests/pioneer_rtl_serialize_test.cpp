#include <cassert>
#include <cstdio>
#include <cstdint>

// Execute the actual selection/helpers/compatibility interrupt thread.
// No fake recursive Giant: a second acquisition is an immediate failure.
using int32 = int;
using status_t = int;
using device_t = void*;
using driver_intr_t = int(void*);
enum { IPL_NET=0, IPL_MPSAFE=0x100, INTR_TYPE_NET=2, INTR_MPSAFE=4,
 SYS_RES_IRQ=1, RF_ACTIVE=1, RF_SHAREABLE=2, B_OK=0, MA_OWNED=1 };
struct resource {};
struct pci_intr_handle_t { int rid; };
struct ifnet { int if_snd; };
struct rge_softc { struct { ifnet ac_if; } sc_arpcom; int sc_task; };
struct internal_intr {
 device_t dev; int sem,flags,handling; driver_intr_t* handler; void* arg;
};
static int Giant,locks,unlocks,kicks,queued,resets,reenabled,acquires;
static bool held;
static internal_intr selected;
static resource irq;
static void mtx_lock(int* lock)
{ assert(lock==&Giant && !held); held=true; locks++; }
static void mtx_unlock(int* lock)
{ assert(lock==&Giant && held); held=false; unlocks++; }
static void mtx_assert(int* lock,int state)
{ assert(lock==&Giant && state==MA_OWNED && held); }
#define KERNEL_LOCK() mtx_lock(&Giant)
#define KERNEL_UNLOCK() mtx_unlock(&Giant)
static void rge_init(ifnet* ifp) { assert(ifp && held); resets++; }
static void rge_txstart(void* sc) { assert(sc && held); kicks++; }
static void ifq_serialize(int* queue,int* task)
{ assert(queue && task); queued++; }
static int acquire_sem(int) { return acquires++==0 ? B_OK : -1; }
static void atomic_or(int* address,int value) { *address|=value; }
static void atomic_and(int* address,int value) { *address&=value; }
static void HAIKU_REENABLE_INTERRUPTS(device_t) { reenabled++; }
static resource* bus_alloc_resource_any(device_t,int type,int* rid,int flags)
{ assert(type==SYS_RES_IRQ && *rid==1 && flags==RF_ACTIVE); return &irq; }
static int bus_setup_intr(device_t dev,resource* res,int flags,void*,
 driver_intr_t* handler,void* arg,void** cookie)
{ assert(res==&irq); selected={dev,1,flags,0,handler,arg}; *cookie=&selected; return 0; }

#include "rtl_serialize_helpers.inc"
#include "rtl_serialize_establish.inc"
#include "rtl_serialize_handler.inc"

static int level()
{
#include "rtl_serialize_level.inc"
 return interrupt_level;
}
static constexpr bool serial =
#if defined(__HAIKU__) && defined(__riscv) && defined(HAIKU_PIONEER_RTL_SERIALIZE)
 true;
#else
 false;
#endif
static int work(void* data)
{
 auto* sc=static_cast<rge_softc*>(data);
 assert(held==serial);
 rge_system_error(&sc->sc_arpcom.ac_if); // exercises nonrecursive reset path
 assert(held==serial);
 rge_kick_tx(sc);
 return 0;
}
int main()
{
 rge_softc sc={};
 mtx_lock(&Giant); // compatibility send/ioctl uses IFF_NEEDSGIANT
 rge_kick_tx(&sc);
 mtx_unlock(&Giant);
 assert(kicks==(serial?1:0) && queued==(serial?0:1));
 assert(level()==(serial?IPL_NET:IPL_NET|IPL_MPSAFE));
 auto* intr=static_cast<internal_intr*>(pci_intr_establish_openbsd(
  nullptr,{1},level(),work,&sc,"test"));
 assert(intr==&selected && bool(intr->flags&INTR_MPSAFE)==!serial);
 assert(intr_handler(intr)==0);
 assert(!held && intr->handling==0 && reenabled==1 && resets==1);
 assert(locks==2 && unlocks==2); // ONE interrupt or reset lock, never both
 assert(kicks==(serial?2:0) && queued==(serial?0:2));
 puts(serial ? "PASS: actual serialized RTL interrupt owns Giant,direct kicks,no queued NEEDSGIANT drain hazard,no recursive reset lock"
  : "PASS: default/foreign RTL keeps MPSAFE interrupt,deferred TX kick and existing reset lock");
}
