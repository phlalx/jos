#include <kern/e1000.h>
#include <kern/pmap.h>

volatile uint32_t *e1000;

#define TDLEN 16 // TDLEN * sizeof(tx_desc) must be a multiple of 128

uint8_t send_buffer[TDLEN][MTU];
// align this on 16-bytes boundary

struct tx_desc descs[TDLEN];

int e1000_attach_fn(struct pci_func *pcif) {
	pci_func_enable(pcif);
	uint32_t base = pcif->reg_base[0];
	uint32_t size = pcif->reg_size[0];
	e1000 = mmio_map_region(base, size);
	uint32_t val = e1000[E1000_STATUS];
	cprintf("E1000 Status = %08x\n", val);

	e1000[E1000_TDBAL] = (uint32_t) PADDR(descs); 
	e1000[E1000_TDLEN] = TDLEN * sizeof(struct tx_desc);
	e1000[E1000_TDH] = 0;
	e1000[E1000_TDT] = 0;
	e1000[E1000_TCTL] = E1000_TCTL_EN | E1000_TCTL_PSP | 0x10 << E1000_TCTL_CT_SHIFT
	| 0x40 << E1000_TCTL_COLD_SHIFT; // cf. section 14.5 et exercise 5

	e1000[E1000_TIPG] = 10 << E1000_IGPT_SHIFT | 10 << E1000_IGPR1_SHIFT 
	| 10 << E1000_IGPR2_SHIFT; // TODO revoir totalement ça ! 10.4.34

	// TODO que renvoyer ?
	return 0;
}
