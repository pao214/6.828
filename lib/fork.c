// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Assembly language pgfault entrypoint defined in lib/pfentry.S.
extern void _pgfault_upcall(void);

// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int rc;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
    int pn = (int)((uintptr_t)addr/PGSIZE);
    if (!(err&FEC_U) || !(err&FEC_WR) || !(uvpt[pn]&PTE_COW))
        panic("[%08x] user handler fault va %08x ip %08x err %d flags %03x\n",
            thisenv->env_id, addr, utf->utf_eip, err, uvpt[pn]&0xfff);

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	// panic("pgfault not implemented");
    rc = sys_page_alloc(0, (void*)PFTEMP, PTE_W|PTE_U|PTE_P);
    if (rc < 0)
        panic("[%08x] user handler fault va %08x ip %08x\n",
            thisenv->env_id, addr, utf->utf_eip);
    addr = ROUNDDOWN(addr, PGSIZE);
    memcpy(PFTEMP, addr, PGSIZE);
    // FIXME: Should we unmap the page previously at PFTEMP?
    rc = sys_page_map(0, (void*)PFTEMP, 0, addr, PTE_W|PTE_U|PTE_P);
    if (rc < 0)
        panic("[%08x] user handler fault va %08x ip %08x\n",
            thisenv->env_id, addr, utf->utf_eip);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int rc;

	// LAB 4: Your code here.
	// panic("duppage not implemented");
    assert(uvpd[pn >> 10]&PTE_P);
    pte_t pte = uvpt[pn];
    assert(pte&PTE_P);

    // FIXME: COW on read only pages?
    if (!(pte&PTE_W) && !(pte&PTE_COW))
    {
        if (!envid || envid == thisenv->env_id)
            return 0;
        rc = sys_page_map(0, (void*)((uintptr_t)pn*PGSIZE), envid, (void*)((uintptr_t)pn*PGSIZE), PTE_U|PTE_P);
        return rc;
    }

    rc = sys_page_map(0, (void*)((uintptr_t)pn*PGSIZE), envid, (void*)((uintptr_t)pn*PGSIZE), PTE_COW|PTE_U|PTE_P);
	return rc;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// panic("fork not implemented");
    set_pgfault_handler(pgfault);
    int child;
    child = sys_exofork();
    if (child < 0)
        return child;
    else if (child == 0)
    {
        envid_t envid = sys_getenvid();
        thisenv = &envs[ENVX(envid)];
        return 0;
    }
    
    // Traverse the page tables
    // FIXME: Traverse through the directory
    int rc;
    for (int ptn = 0; ptn <= USTACKTOP/PTSIZE; ptn++)
    {
        pde_t pde = uvpd[ptn];
        if (!(pde&PTE_P))
            continue;
        for (int pn = (ptn<<10); (pn < ((ptn+1)<<10)) && (pn < USTACKTOP/PGSIZE); pn++)
        {
            pte_t pte = uvpt[pn];
            if (!(pte&PTE_P))
                continue;
            rc = duppage(child, pn);
            if (rc < 0)
                return rc;
            rc = duppage(0, pn);
            if (rc < 0)
                return rc;
        }
    }

    // Install handler for the child
    rc = sys_page_alloc(child, (void*)UXSTACKTOP-PGSIZE, PTE_W|PTE_U|PTE_P);
    if (rc < 0)
        return rc;
    rc = sys_env_set_pgfault_upcall(child, _pgfault_upcall);
    if (rc < 0)
        return rc;

    rc = sys_env_set_status(child, ENV_RUNNABLE);
    if (rc < 0)
        return rc;
    return child;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
