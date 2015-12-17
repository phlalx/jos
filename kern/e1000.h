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
#define E1000_CTRL     0x00000      /* Device Control - RW */
#define E1000_STATUS   0x00008 / 4  /* Device Status - RO */
#define E1000_TDBAL    0x03800 / 4 
#define E1000_TDLEN    0x03808 / 4 /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810 / 4 /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818 / 4 /* TX Descripotr Tail - RW */
#define E1000_TCTL     0x00400 / 4 /* TX Control - RW */
#define E1000_TIPG     0x00410 / 4 /* TX Inter-packet gap -RW */

#define E1000_RCTL     0x00100 / 4  /* RX Control - RW */
#define E1000_RDBAL    0x02800 / 4 /* RX Descriptor Base Address Low (1) - RW */
#define E1000_RDLEN    0x02808 / 4 /* RX Descriptor Length (1) - RW */
#define E1000_RDH      0x02810 / 4 /* RX Descriptor Head (1) - RW */
#define E1000_RDT      0x02818 / 4 /* RX Descriptor Tail (1) - RW */

#define E1000_RAL       0x05400 / 4 /* Receive Address - RW Array */
#define E1000_RAH       0x05404 / 4 /* Receive Address - RW Array */
#define E1000_RA        0x05400 / 4 /* Receive Address - RW Array */

/* Transmit Control */
#define E1000_TCTL_RST    0x00000001    /* software reset */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_BCE    0x00000004    /* busy check enable */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_CT_SHIFT 4

#define E1000_IGPT_SHIFT 0
#define E1000_IGPR1_SHIFT 10 
#define E1000_IGPR2_SHIFT 20 

#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
#define E1000_TCTL_COLD_SHIFT 12
#define E1000_TCTL_SWXOFF 0x00400000    /* SW Xoff transmission */
#define E1000_TCTL_PBE    0x00800000    /* Packet Burst Enable */
#define E1000_TCTL_RTLC   0x01000000    /* Re-transmit on late collision */
#define E1000_TCTL_NRTU   0x02000000    /* No Re-transmit on underrun */
#define E1000_TCTL_MULR   0x10000000    /* Multiple request support */
 
#define E1000_IGPT_SHIFT 0
#define E1000_IGPR1_SHIFT 10 
#define E1000_IGPR2_SHIFT 20 

#define E1000_TXD_CMD_RS      0x08 /* Report Status */
#define E1000_TXD_CMD_EOP     0x01 /* Report Status */
#define E1000_TXD_STAT_DD    0x01 /* Descriptor Done */

#define E1000_RCTL_EN             0x00000002    
#define E1000_RCTL_LPE            0x00000020    /* long packet enable */	

// We don't probably don't need to set these as they depends on interruption
// being enabled	
#define E1000_RCTL_RDMTS_HALF     0x00000000    /* rx desc min threshold size */
#define E1000_RCTL_RDMTS_QUAT     0x00000100    /* rx desc min threshold size */
#define E1000_RCTL_RDMTS_EIGTH    0x00000200    /* rx desc min threshold size */	

#define E1000_RCTL_BAM            0x00008000    /* broadcast enable */	
#define E1000_RCTL_SZ_2048        0x00000000    /* rx buffer size 2048 */

/* Receive Address */
#define E1000_RAH_AV  0x80000000        /* Receive descriptor valid */

#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */

#define MTU 1518
#define RECBUFFER 2048  // one of 7 modes proposed by hardware, don't change this
 // without changing the corresponding bits in RCTL

struct tx_desc
{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
};

/* Receive Descriptor */
struct e1000_rx_desc {
    uint64_t buffer_addr; /* Address of the descriptor's data buffer */
    uint16_t length;     /* Length of data DMAed into data buffer */
    uint16_t csum;       /* Packet checksum */
    uint8_t status;      /* Descriptor status */
    uint8_t errors;      /* Descriptor Errors */
    uint16_t special;
};

int e1000_attach_fn(struct pci_func *pcif);

int e1000_send_packet(void *buffer, size_t length);

int e1000_receive_packet(uint8_t *buffer, size_t *length);

#endif	// JOS_KERN_E1000_H
