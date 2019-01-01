#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

// Forward declare
struct pci_func;

void pci_func_enable(struct pci_func *pcif);
int attach (struct pci_func *pcif);

#endif  // SOL >= 6
