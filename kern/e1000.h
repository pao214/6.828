#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <inc/string.h>
#include <kern/pci.h>

#define E1000_VEN_ID_82540EM                0x8086
#define E1000_DEV_ID_82540EM                0x100E
#define E1000_FULL_DUPLEX                   0x80080783
#define E1000_TX_DESC_LEN                   0x40
#define E1000_MAX_PKT_LEN                   0x5EE

// Register set
#define E1000_STATUS                        0x00008     // Device status - RO
#define E1000_TCTL                          0x00400     // TX control - RW
#define E1000_TIPG                          0x00410     // TX inter-packet gap - RW
#define E1000_TDBAL                         0x03800     // TX descriptor base addr - RW
#define E1000_TDLEN                         0x03808     // TX descriptor size - RW
#define E1000_TDH                           0x03810     // TX descriptor head - RW
#define E1000_TDT                           0x03818     // TX descritor tail - RW

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

// Descriptor command
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

// Transmit descriptor
struct e1000_tx_desc_t
{
    uint64_t                addr;
    uint16_t                length;
    uint8_t                 cso;
    struct e1000_tx_cmd_t   cmd;
    uint8_t                 status;
    uint8_t                 css;
    uint16_t                special;
};

#pragma pack(pop)

extern volatile uint8_t *netaddr;

int attach(struct pci_func *pcif);
void tx_init();
void validate();

#endif  // SOL >= 6
