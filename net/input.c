#include "ns.h"
#include "inc/lib.h"

#define DEBUG 1

extern union Nsipc nsipcbuf;

// TODO ? pourquoi le kernel n'arrive pas à écire à cette addresse ?
// parce qu'elle n'appartient plus forcément à ce processus quand
// il essaye d'écrire dedans ? 
// uint8_t buffer[PGSIZE] __attribute__((aligned(PGSIZE)));

void
input_serve(void)
{
	uint32_t req, whom;
	int perm, r;
	size_t length;
	envid_t ns_envid = 0;
	while (ns_envid == 0) {
		ns_envid = ipc_find_env(ENV_TYPE_NS);
	}


	r = sys_page_alloc(0, UTEMP, PTE_P|PTE_W|PTE_U); 
	if (r) {
		assert(false);
	}

	while (1) {
//		r = sys_receive_packet(nsipcbuf.pkt.jp_data, &length);
		r = sys_receive_packet(UTEMP, &length);
		if (r == -E_ETH_EMPTY) {
			// no message to receive
			continue;
		}
		if (r < 0) {
			panic("invalid argument\n");
		}

		nsipcbuf.pkt.jp_len = length;
		cprintf("message received (len = %d) forwarding to ns server\n", length);
		assert(length <= PGSIZE);
		memcpy(nsipcbuf.pkt.jp_data, UTEMP, length);
		ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_W | PTE_U);
		long int pause = 100000000;
		cprintf("pause...");
		int x = 0;
		while (pause--) { x = x + 1; };
		cprintf("%d\n", x);
	}
}


void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	cprintf("Input environment %08x is alive and kicking! \n", thisenv->env_id);

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	input_serve();

}
