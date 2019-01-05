#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/error.h>

// LAB 6: Your driver code here

struct e1000_tx_desc_t tx_descs[E1000_TX_DESC_LEN] __attribute__((aligned (16)));
uint8_t tx_buf[E1000_TX_DESC_LEN*E1000_MAX_PKT_LEN];

// Address not mapped to physical memory
volatile uint8_t *netaddr;
volatile uint16_t *tx_tail;

int attach(struct pci_func *pcif)
{
    pci_func_enable(pcif);
    netaddr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    validate();
    tx_init();
    return 1;
}

// Sanity checks
void validate()
{
    assert(*(uint32_t *)(netaddr+E1000_STATUS) == E1000_FULL_DUPLEX);
    assert(sizeof(struct e1000_tctl_t) == 4);
    assert(sizeof(struct e1000_tipg_t) == 4);
    assert(sizeof(struct e1000_tx_desc_t) == 16);
    assert(!(sizeof(tx_descs)&0x7f));
    assert(!((uintptr_t)PADDR(tx_descs)&0xf));
}

// Initialize the transmit descriptors
void tx_init()
{
    // Device only accepts physical contiguous memory
    *(uint32_t *)(netaddr+E1000_TDBAL) = (uint32_t)PADDR(tx_descs);
    *(uint32_t *)(netaddr+E1000_TDLEN) = sizeof(tx_descs);
    assert(*(uint16_t *)(netaddr+E1000_TDH) == 0);
    tx_tail = (uint16_t *)(netaddr+E1000_TDT);
    assert(*tx_tail == 0);

    // Initialize descriptors
    for (int i = 0; i < E1000_TX_DESC_LEN; i++)
    {
        // FIXME: don't need to zero init
        struct e1000_tx_desc_t *tx_desc = &tx_descs[i];
        memset(tx_desc, 0, sizeof(*tx_desc));
        tx_desc->status.dd = 1; // all desc are free to use
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

// Return
//  -E_INVAL if the packet length is higher than expected
//  -E_RETRY if the transmit buffer is full
//  0 if SUCCESS (packet may still be dropped)
//  If the packet is added to a descriptor, it is considered SUCCESS
int tx_try_send(const char *pkt, size_t len)
{
    if (len > E1000_MAX_PKT_LEN)
        return -E_INVAL;
    // Explicitly read since may not be optimized
    uint16_t tail = *tx_tail;
    // NOTE: Do not check the immediate descriptor
    if (!tx_descs[(tail+1)%E1000_TX_DESC_LEN].status.dd)
        return -E_RETRY;
    struct e1000_tx_desc_t *tx_desc = &tx_descs[tail%E1000_TX_DESC_LEN];
    memset(tx_desc, 0, sizeof(*tx_desc));
    tx_desc->addr = (uint32_t)(PADDR(&tx_buf[tail*E1000_MAX_PKT_LEN]));
    memcpy((void*)KADDR(tx_desc->addr), pkt, len);
    tx_desc->length = len;
    tx_desc->cmd.eop = 1;
    tx_desc->cmd.rs = 1;
    // tx_desc->cmd.dext already set to zero
    // tx_desc->status.dd also set to zero
    *tx_tail = (tail+1)%E1000_TX_DESC_LEN;
    return 0;
}
