#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_VEN_ID_82540EM                0x8086
#define E1000_DEV_ID_82540EM                0x100E
#define E1000_FULL_DUPLEX                   0x80080783

extern volatile uint32_t *netaddr;

int attach(struct pci_func *pcif);

#endif  // SOL >= 6
