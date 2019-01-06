#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/error.h>

// LAB 6: Your driver code here

struct e1000_tx_desc_t tx_descs[E1000_TX_DESC_LEN] __attribute__((aligned (16)));
uint8_t tx_buf[E1000_TX_DESC_LEN*E1000_MAX_PKT_LEN];
struct e1000_rx_desc_t rx_descs[E1000_RX_DESC_LEN] __attribute__((aligned (16)));
uint8_t rx_buf[E1000_RX_DESC_LEN*E1000_RX_BSIZE];

// Address not mapped to physical memory
volatile uint8_t *netaddr;
volatile uint16_t *tx_tail;
volatile uint16_t *rx_tail;

// Forward declare
void validate();
void tx_init();
void rx_init();

int attach(struct pci_func *pcif)
{
    pci_func_enable(pcif);
    netaddr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    validate();
    tx_init();
    rx_init();
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
    assert(sizeof(struct e1000_rctl_t) == 4);
    assert(sizeof(struct e1000_rx_desc_t) == 16);
    assert(!(sizeof(rx_descs)&0x7f));
    assert(!((uintptr_t)PADDR(rx_descs)&0xf));
}

// Initialize transmit descriptors
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

// Initialize receive descriptors
void rx_init()
{
    // Initialize receive address
    // Also set valid bit
	uint8_t mac[8] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56, 0x00, 0x80};
    *(uint64_t *)(netaddr+E1000_RA) = *(uint64_t *)mac;

    // FIXME: Initialize multicast table array
    // memset((void *)netaddr+E1000_MTA, 0, E1000_RA-E1000_MTA);

    // Initialize desc regs
    *(uint32_t *)(netaddr+E1000_RDBAL) = (uint32_t)PADDR(rx_descs);
    *(uint32_t *)(netaddr+E1000_RDLEN) = sizeof(rx_descs);
    assert(*(uint16_t *)(netaddr+E1000_RDH) == 0);
    rx_tail = (uint16_t *)(netaddr+E1000_RDT);
    assert(*rx_tail == 0);

    // Initialize desc
    for (int i = 0; i < E1000_RX_DESC_LEN; i++)
    {
        struct e1000_rx_desc_t *rx_desc = &rx_descs[i];
        memset(rx_desc, 0, sizeof(*rx_desc));
        rx_desc->addr = (uint32_t)PADDR(&rx_buf[i*E1000_RX_BSIZE]);
    }

    // Init tail
    *rx_tail = E1000_RX_DESC_LEN-1;

    // Initialzie rctl
    struct e1000_rctl_t rctl;
    memset(&rctl, 0, sizeof(rctl));
    rctl.en = 1;
    // rctl.bsize, rctl.bsex set to zero
    rctl.secrc = 1;
    *(struct e1000_rctl_t *)(netaddr+E1000_RCTL) = rctl;
}

// Return
//  -E_INVAL if the packet length is higher than expected
//  -E_RETRY if the transmit buffer is full
//  0 if SUCCESS (packet may still be dropped)
//  If the packet is added to a descriptor, it is considered SUCCESS
// FIXME: Change the definition to use jif_pkt
int tx_try_send(const struct jif_pkt *pkt)
{
    if (pkt->jp_len > E1000_MAX_PKT_LEN)
        return -E_INVAL;
    // Explicitly read since may not be optimized
    uint16_t tail = *tx_tail;

    // NOTE: Do not check immediate descriptor
    struct e1000_tx_desc_t *tx_desc;
    tx_desc = &tx_descs[(tail+1)%E1000_TX_DESC_LEN];
    if (!tx_desc->status.dd)
        return -E_RETRY;

    // Init next descriptor to transmit
    tx_desc = &tx_descs[tail%E1000_TX_DESC_LEN];
    memset(tx_desc, 0, sizeof(*tx_desc));
    tx_desc->addr = (uint32_t)(PADDR(&tx_buf[tail*E1000_MAX_PKT_LEN]));
    memcpy((void*)KADDR(tx_desc->addr), pkt->jp_data, pkt->jp_len);
    tx_desc->length = pkt->jp_len;
    tx_desc->cmd.eop = 1;
    tx_desc->cmd.rs = 1;
    // tx_desc->cmd.dext already set to zero
    // tx_desc->status.dd also set to zero
    *tx_tail = (tail+1)%E1000_TX_DESC_LEN;
    return 0;
}

// Return
//  -E_RETRY if no desc is free for reading
//  0 on SUCCESS
// Panics if the length is greater than maximum allowed
int rx_try_recv(struct jif_pkt *pkt)
{
    // Check desc available
    uint16_t tail = *rx_tail;
    uint16_t next = (tail+1)%E1000_RX_DESC_LEN;
    struct e1000_rx_desc_t *rx_desc;
    rx_desc = &rx_descs[next];
    if (!rx_desc->status.dd)
        return -E_RETRY;

    // Read from next descriptor
    assert(rx_desc->status.eop);
    assert(rx_desc->length <= E1000_RX_BSIZE);
    assert(rx_desc->status.eop);
    memcpy(pkt->jp_data, &rx_buf[next*E1000_RX_BSIZE], rx_desc->length);
    pkt->jp_len = rx_desc->length;

    // Release the descriptor
    // rx_desc = &rx_desc[tail];
    // memset(rx_desc, 0, sizeof(*rx_desc));
    // rx_desc->addr = (uint32_t)PADDR(&rx_buf[tail*E1000_RX_BSIZE]);
    *rx_tail = next;
    return 0;
}
