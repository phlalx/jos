#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include "pci.h"

#define E1000_DEV_ID_82540EM 0x100E
#define E1000_VENDOR_ID	0x8086

/* Register Set. (82543, 82544)
 *
 * Registers are defined to be 32 bits and  should be accessed as 32 bit values.
 * These registers are physically located on the NIC, but are mapped into the
 * host memory address space.
 *
 * RW - register is both readable and writable
 * RO - register is read only
 * WO - register is write only
 * R/clr - register is read only and is cleared when read
 * A - register array
 */
#define E1000_CTRL     0x00000  /* Device Control - RW */
#define E1000_STATUS   0x00008  /* Device Status - RO */

int e1000_attach_fn(struct pci_func *pcif);

#endif	// JOS_KERN_E1000_H
