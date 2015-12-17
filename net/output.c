#include "ns.h"
#include "inc/lib.h"

#define DEBUG 0

extern union Nsipc nsipcbuf;

// Virtual address at which to receive page mappings containing client requests.
// union Nsipc *Nsipcreq = (union Nsipc *)0x0ffff000;
// TODO pourquoi c'est pas fait pareil que pour le FS ?

void
output_serve(void)
{
	uint32_t req, whom;
	int perm, r;

	while (1) {
		perm = 0;
		req = ipc_recv((int32_t *) &whom, &nsipcbuf, &perm);
		if (DEBUG)
			cprintf("output req %d from %08x\n", whom);

		// All requests must contain an argument page
		if (!(perm & PTE_P)) {
			cprintf("Invalid request from %08x: no argument page\n", whom);
			continue; // just leave it hanging...
		}
		if (req != NSREQ_OUTPUT) {
			cprintf("Invalid request from %08x\n", whom);
			continue;
		}
		int len = nsipcbuf.pkt.jp_len;
		char *buf = nsipcbuf.pkt.jp_data;
		r = sys_send_packet(buf, len);
		if (r < 0) {
			cprintf("failed to send packet\n");
		}
	}
}

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	cprintf("Output environment is alive and kicking! \n");
	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver

	output_serve();
}
