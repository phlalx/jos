#include <kern/e1000.h>

int e1000_attach_fn(struct pci_func *pcif) {
	pci_func_enable(pcif);
	return 0;
}
