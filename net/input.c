#include "ns.h"

extern union Nsipc nsipcbuf;
char mem[PGSIZE] __attribute__((aligned(PGSIZE)));

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
    int r;
    struct jif_pkt *pkt = &nsipcbuf.pkt;

	while (1)
    {
        r = sys_page_alloc(0, pkt, PTE_W|PTE_U|PTE_P);
        if (r < 0)
            panic("sys_page_alloc: %e\n", r);

        // Receive pkt from driver
        do {
            r = sys_net_try_recv(pkt);
            sys_yield();
        } while (r == -E_RETRY);

        if (r < 0)
            panic("sys_net_try_recv: %e\n", r);

        // Send pkt to server
        ipc_send(ns_envid, NSREQ_INPUT, pkt, PTE_W|PTE_U|PTE_P);
        r = sys_page_unmap(0, pkt);
        if (r < 0)
            panic("sys_page_unmap: %e\n", r);
	}
}
