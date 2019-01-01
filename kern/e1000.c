#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

volatile uint32_t *netaddr;

int attach(struct pci_func *pcif)
{
    pci_func_enable(pcif);
    netaddr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    assert(*(netaddr+2) == E1000_FULL_DUPLEX);
    return 1;
}
