#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#define RGE_TX_LIST_CNT 4
#define RGE_RX_LIST_CNT 4
#define RGE_CMD 0
#define RGE_ISR 1
#define RGE_IMR 2
#define DEVNAME(sc) "mock"
#define letoh32(value) (value)
#define letoh64(value) (value)

struct tx_desc { uint32_t rge_cmdsts; };
struct rx_desc {
	struct { uint64_t rge_addr; } hi_qword0;
	struct { struct { uint32_t rge_cmdsts; } rx_qword4; } hi_qword1;
};
struct segment { uint64_t ds_addr; };
struct dma_map { struct segment dm_segs[1]; };
struct rge_queues {
	struct {
		int rge_txq_considx, rge_txq_prodidx;
		struct tx_desc *rge_tx_list;
		struct segment rge_tx_listseg;
		struct dma_map *rge_tx_list_map;
	} q_tx;
	struct {
		int rge_rxq_considx;
		struct rx_desc *rge_rx_list;
		struct segment rge_rx_listseg;
		struct dma_map *rge_rx_list_map;
	} q_rx;
};
struct rge_softc {
	struct rge_queues *sc_queues;
	uint32_t rge_diagnostic_ticks, rge_diagnostic_interrupts;
	int sc_timeout;
};
static unsigned reads, lines, schedules, links;
static int splnet(void) { return 7; }
static void splx(int value) { assert(value == 7); }
static void rge_link_state(struct rge_softc *sc) { (void)sc; links++; }
static void timeout_add_sec(int *timeout, int seconds)
{ (void)timeout; assert(seconds == 1); schedules++; }
static unsigned read_register(struct rge_softc *sc, int reg)
{ (void)sc; assert(reg >= 0 && reg <= 2); reads++; return 0x1234; }
#define RGE_READ_1 read_register
#define RGE_READ_4 read_register
static int diagnostic_printf(const char *format, ...)
{ (void)format; lines++; return 0; }
#define printf diagnostic_printf

#include "production_tick.inc"

int main(void)
{
	struct tx_desc tx[4] = {0};
	struct rx_desc rx[4] = {0};
	struct rge_queues q = {0};
	struct dma_map tx_map = {0}, rx_map = {0};
	q.q_tx.rge_tx_list = tx;
	q.q_rx.rge_rx_list = rx;
	q.q_tx.rge_tx_list_map = &tx_map;
	q.q_rx.rge_rx_list_map = &rx_map;
	struct rge_softc sc = {&q, 0, 123, 0};
	for (int i = 0; i < 20; i++) rge_tick(&sc);
	assert(schedules == 20 && links == 20);
	assert(sc.rge_diagnostic_interrupts == 123);
#if defined(HAIKU_PIONEER_RTL_DIAGNOSTICS) && defined(__riscv)
	unsigned limit = 16;
#if defined(__HAIKU__) && defined(HAIKU_PIONEER_RTL_SERIALIZE)
	limit = 900;
#endif
	unsigned count = limit < 20 ? limit : 20;
	assert(sc.rge_diagnostic_ticks == count && reads == count*3 && lines == count*2);
	for (unsigned i=20; i<1000; i++) rge_tick(&sc);
	assert(sc.rge_diagnostic_ticks == limit && reads == limit*3 && lines == limit*2);
	sc.rge_diagnostic_ticks = 0;
	q.q_tx.rge_txq_considx = -1;
	rge_tick(&sc);
	assert(lines == limit*2+1); /* Invalid index: never dereference descriptors. */
	q.q_tx.rge_txq_considx = 0;
	q.q_tx.rge_tx_list_map = NULL;
	rge_tick(&sc);
	assert(lines == limit*2+2); /* Incomplete DMA map must also be safe. */
	sc.sc_queues = NULL;
	rge_tick(&sc);
	assert(lines == limit*2+3);
#else
	assert(sc.rge_diagnostic_ticks == 0 && reads == 0 && lines == 0);
#endif
	return 0;
}
