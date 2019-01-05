#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

volatile uint8_t *netaddr;
struct e1000_tx_desc_t tx_descs[E1000_TX_DESC_LEN] __attribute__ ((aligned (16)));
uint8_t tx_buf[E1000_TX_DESC_LEN*E1000_MAX_PKT_LEN];

int attach(struct pci_func *pcif)
{
    pci_func_enable(pcif);
    netaddr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    validate();
    tx_init();
    return 1;
}

void tx_init()
{
    // Device only accepts physical contiguous memory
    *(uint32_t *)(netaddr+E1000_TDBAL) = (uintptr_t)PADDR(tx_descs);
    *(uint32_t *)(netaddr+E1000_TDLEN) = sizeof(tx_descs);
    *(uint16_t *)(netaddr+E1000_TDH) = 0;
    *(uint64_t *)(netaddr+E1000_TDT) = 0;

    // Initialize descriptors
    for (int i = 0; i < E1000_TX_DESC_LEN; i++)
    {
        struct e1000_tx_desc_t *tx_desc = &tx_descs[i];
        memset(tx_desc, 0, sizeof(*tx_desc));
        tx_desc->addr = (uint64_t)(PADDR(&tx_buf[i*E1000_MAX_PKT_LEN]));
        // tx_desc->length <= E1000_MAX_PKT_LEN;
        // tx_desc->cmd.eop
        tx_desc->cmd.rs = 1;
        // tx_desc->cmd.dext already set to zero
    }
    
    // Initialize TCTL
    struct e1000_tctl_t tctl;
    memset(&tctl, 0, sizeof(tctl));
    tctl.en = 1;
    tctl.psp = 1;
    tctl.ct = 0x10;
    tctl.cold = 0x40;
    *(struct e1000_tctl_t *)(netaddr+E1000_TCTL) = tctl;

    // Initialize TIPG
    struct e1000_tipg_t tipg;
    memset(&tipg, 0, sizeof(tipg));
    tipg.ipgt = 10;
    tipg.ipgr1 = 8;
    tipg.ipgr2 = 6;
    *(struct e1000_tipg_t *)(netaddr+E1000_TIPG) = tipg;
}

void validate()
{
    assert(*(uint32_t *)(netaddr+E1000_STATUS) == E1000_FULL_DUPLEX);
    assert(sizeof(struct e1000_tctl_t) == 4);
    assert(sizeof(struct e1000_tipg_t) == 4);
    assert(sizeof(struct e1000_tx_desc_t) == 16);
    assert(!(sizeof(tx_descs)&0x7f));
    assert(!((uintptr_t)PADDR(tx_descs)&0xf));
}
