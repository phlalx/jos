#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

volatile uint32_t *e1000;

#define TDLEN 16 // TDLEN * sizeof(tx_desc) must be a multiple of 128
#define RDLEN 128 // RDLEN * sizeof(e1000_rx_desc) must be a multiple of 128

uint8_t send_buffer[TDLEN][MTU];


uint8_t receive_buffer[RDLEN][MTU];

// TODO align this on 16-bytes boundary
struct tx_desc descs[TDLEN] = { {0} };
// TODO align this on 16-bytes boundary
struct e1000_rx_desc rx_descs[RDLEN] = { {0} };

static int cur = 0; 
	// descriptor to use for sending a message (only if DD
	// isn't set)

static int rx_cur = 0; 
	// descriptor to receive the next message

int e1000_receive_packet(uint8_t *buffer, size_t *length); 

int e1000_attach_fn(struct pci_func *pcif) {
	pci_func_enable(pcif);
	uint32_t base = pcif->reg_base[0];
	uint32_t size = pcif->reg_size[0];
	e1000 = mmio_map_region(base, size);
	uint32_t val = e1000[E1000_STATUS];
	cprintf("E1000 Status = %08x\n", val);

	// init sending
	e1000[E1000_TDBAL] = (uint32_t) PADDR(descs); 
	e1000[E1000_TDLEN] = TDLEN * sizeof(struct tx_desc);
	e1000[E1000_TDH] = 0;
	e1000[E1000_TDT] = 0;
	e1000[E1000_TCTL] = E1000_TCTL_EN | E1000_TCTL_PSP | 0x10 << E1000_TCTL_CT_SHIFT
	| 0x40 << E1000_TCTL_COLD_SHIFT; // cf. section 14.5 et exercise 5

	e1000[E1000_TIPG] = 10 << E1000_IGPT_SHIFT | 10 << E1000_IGPR1_SHIFT 
	| 10 << E1000_IGPR2_SHIFT; // TODO revoir totalement ça ! 10.4.34


	int i = 0;
	for (i = 0; i < TDLEN; i++) {
		// has to be set the first time we use a descriptor
		// we use this bit to make sure a descriptor is available
		// set by the hardware after the first use
		descs[i].status = E1000_TXD_STAT_DD;
	}

	//e1000[E1000_RA][0] = 0x52;
	// 0x54, 0x00, 0x12, 0x34, 0x56};

	uint8_t *mac_addr = (uint8_t *) &e1000[E1000_RA];
	e1000[E1000_RAL] = 0x12005452;
	e1000[E1000_RAH] = 0x5634 | E1000_RAH_AV;

	// init reception
	e1000[E1000_RDBAL] = (uint32_t) PADDR(rx_descs); 
	e1000[E1000_RDLEN] = RDLEN * sizeof(struct e1000_rx_desc);
	e1000[E1000_RDH] = 0;
	e1000[E1000_RDT] = 0;
 	e1000[E1000_RCTL] = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SZ_2048;

 	// The head pointer points to the next descriptor that is written back
 	// HARDWARE OWNS ALL DESCRIPTORS BETWEEN [HEAD AND TAIL]

	for (i = 0; i < TDLEN; i++) {
		// has to be set the first time we use a descriptor
		// we use this bit to make sure a descriptor is available
		// set by the hardware after the first use
		rx_descs[i].buffer_addr = (uint32_t) PADDR(receive_buffer[i]);
	}


//	uint8_t buffer[3000];
//	size_t length;
//	e1000_receive_packet(buffer, &length);
	
	return 0;
}

int e1000_send_packet(void *buffer, size_t length) {
	assert(length <= MTU);

	if (!(descs[cur].status & E1000_TXD_STAT_DD)) {
		cprintf("can't reuse %d\n", cur);
		return -1;
	}	

	memset(&descs[cur], 0, sizeof(struct tx_desc));
	memcpy(send_buffer[cur], buffer, length);

	descs[cur].addr = (uint64_t) PADDR(send_buffer[cur]);
	descs[cur].length = (uint32_t) length;

	// set RS bit to know when descriptor has been used and can be recycled
	// (DD bit is set when that happens)
	uint8_t cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
	descs[cur].cmd = cmd; 

	cur = (cur + 1) % TDLEN;
	e1000[E1000_TDT] = cur;

	return 0;	
}

// buffer should be at least MTU bytes
// return 0 is messaged received, -1 othewise
int e1000_receive_packet(uint8_t *buffer, size_t *length) {
	if (!(rx_descs[rx_cur].status & E1000_RXD_STAT_DD)) {
		return -1;
	}
	int i = 0;
	*length = rx_descs[rx_cur].length;
	assert(*length < MTU);
	uint32_t buffer_addr = rx_descs[rx_cur].buffer_addr;
	memcpy(buffer, (uint8_t *) (buffer_addr), *length);

	rx_cur = (rx_cur + 1) % RDLEN;
	e1000[E1000_RDT] = rx_cur;
	rx_descs[rx_cur].status = 0;

	return 0;	
}


