#include "ns.h"
#include <kern/e1000.h>
#include <inc/error.h>
#include <inc/lib.h>

#define debug 0

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	uint32_t req, whom;
	int perm, r;

	while (1) {
		perm = 0;
		req = ipc_recv((int32_t *) &whom, &nsipcbuf, &perm);
		if (debug)
			cprintf("ns req %d from %08x [page %08x: %s]\n",
				req, whom, uvpt[PGNUM((uintptr_t)&nsipcbuf)], (void*)&nsipcbuf);

        // Invalid request
        if (req != NSREQ_OUTPUT)
        {
            cprintf("Invalid request type %d\n", req);
            continue;
        }

		// All requests must contain an argument page
		if (!(perm & PTE_P))
        {
			cprintf("Invalid request from %08x: no argument page\n",
				whom);
			continue;
		}

        // Drop larger packets
        // FIXME: Better yet fragment the packet
        struct jif_pkt *pkt = &nsipcbuf.pkt; 
        if ((uint32_t)pkt->jp_len > E1000_MAX_PKT_LEN)
        {
            panic("Dropped packet\n");
            continue;
        }

        // Send a system call to the driver
        do {
            r = sys_net_try_send(pkt->jp_data, (size_t)pkt->jp_len);
            sys_yield();
        } while (r == E_RETRY);

        if (r < 0)
            panic("sys_net_try_send: %e\n", r);
	}
}
