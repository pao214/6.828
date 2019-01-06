#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <inc/string.h>
#include <kern/pci.h>
#include <inc/ns.h>

#define E1000_VEN_ID_82540EM                0x8086
#define E1000_DEV_ID_82540EM                0x100E
#define E1000_FULL_DUPLEX                   0x80080783
#define E1000_TX_DESC_LEN                   0x40
#define E1000_MAX_PKT_LEN                   0x5EE
#define E1000_RX_DESC_LEN                   0x80
#define E1000_RX_BSIZE                      0x800

// Register set
#define E1000_STATUS                        0x00008     // Device status - RO
#define E1000_TCTL                          0x00400     // TX control - RW
#define E1000_TIPG                          0x00410     // TX inter-packet gap - RW
#define E1000_TDBAL                         0x03800     // TX descriptor base addr - RW
#define E1000_TDLEN                         0x03808     // TX descriptor size - RW
#define E1000_TDH                           0x03810     // TX descriptor head - RW
#define E1000_TDT                           0x03818     // TX descritor tail - RW
#define E1000_RCTL                          0x00100     // RX Control - RW
#define E1000_MTA                           0x05200     // Multicast table array - RW array
#define E1000_RA                            0x05400     // Receive address - RW array
#define E1000_RDBAL                         0x02800     // RX descriptor base addr - RW
#define E1000_RDLEN                         0x02808     // RX descriptor length - RW
#define E1000_RDH                           0x02810     // RX decsriptor head - RW
#define E1000_RDT                           0x02818     // RX descriptor tail - RW

#pragma pack(push, 1)

// TCTL register
struct e1000_tctl_t
{
    unsigned rsv    : 1;
    unsigned en     : 1;
    unsigned rsv2   : 1;
    unsigned psp    : 1;
    unsigned ct     : 8;
    unsigned cold   : 10;
    unsigned swxoff : 1;
    unsigned rsv3   : 1;
    unsigned rtlc   : 1;
    unsigned rsv4   : 1;
    unsigned rsv5   : 6;
};

// TIPG register
struct e1000_tipg_t
{
    unsigned ipgt   : 10;
    unsigned ipgr1  : 10;
    unsigned ipgr2  : 10;
    unsigned rsv    : 2;
};

// Transmit command
struct e1000_tx_cmd_t
{
    unsigned eop    : 1;
    unsigned ifcs   : 1;
    unsigned ic     : 1;
    unsigned rs     : 1;
    unsigned rsv    : 1;
    unsigned dext   : 1;
    unsigned vle    : 1;
    unsigned ide    : 1;
};

// Transmit status
struct e1000_tx_stat_t
{
    unsigned dd     : 1;
    unsigned ec     : 1;
    unsigned lc     : 1;
    unsigned rsv    : 1;
    unsigned rsv2   : 4;
};

// Transmit descriptor
struct e1000_tx_desc_t
{
    uint32_t                addr;
    uint32_t                high;
    uint16_t                length;
    uint8_t                 cso;
    struct e1000_tx_cmd_t   cmd;
    struct e1000_tx_stat_t  status;
    uint8_t                 css;
    uint16_t                special;
};

// RCTL
struct e1000_rctl_t
{
    unsigned rsv    : 1;
    unsigned en     : 1;
    unsigned sbp    : 1;
    unsigned upe    : 1;
    unsigned mpe    : 1;
    unsigned lpe    : 1;
    unsigned lbm    : 2;
    unsigned rdmts  : 2;
    unsigned rsv2   : 2;
    unsigned mo     : 2;
    unsigned rsv3   : 1;
    unsigned bam    : 1;
    unsigned bsize  : 2;
    unsigned vfe    : 1;
    unsigned cfien  : 1;
    unsigned cfi    : 1;
    unsigned rsv4   : 1;
    unsigned dpf    : 1;
    unsigned pmcf   : 1;
    unsigned rsv5   : 1;
    unsigned bsex   : 1;
    unsigned secrc  : 1;
    unsigned rsv6   : 5;
};

// Receive status
struct e1000_rx_stat_t
{
    unsigned dd     : 1;
    unsigned eop    : 1;
    unsigned ixsm   : 1;
    unsigned vp     : 1;
    unsigned rsv    : 1;
    unsigned tcpcs  : 1;
    unsigned ipcs   : 1;
    unsigned pif    : 1;
};

// Receive descriptor
struct e1000_rx_desc_t
{
    uint32_t                addr;
    uint32_t                high;
    uint16_t                length;
    uint16_t                csum;
    struct e1000_rx_stat_t  status;
    uint8_t                 errors;
    uint16_t                special;
};

#pragma pack(pop)

int attach(struct pci_func *pcif);
int tx_try_send(const struct jif_pkt *pkt);
int rx_try_recv(struct jif_pkt *pkt);

#endif  // SOL >= 6
