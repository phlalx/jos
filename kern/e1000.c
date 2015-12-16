#include <kern/e1000.h>
#include <kern/pmap.h>

volatile uint32_t *e1000;

// static void
// e1000w(int index, int value)
// {
// 	e1000[index] = value;
// }

int e1000_attach_fn(struct pci_func *pcif) {
	pci_func_enable(pcif);
	uint32_t base = pcif->reg_base[0];
	uint32_t size = pcif->reg_size[0];
	e1000 = mmio_map_region(base, size);
	uint32_t val = e1000[E1000_STATUS >> 2];
	cprintf("E1000 Status = %08x\n", val);

	return 0;
}
