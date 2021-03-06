// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;


	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
    //
    if (!(utf->utf_err & FEC_WR)) {
        panic("user space page fault\n");
    }
    // est-ce que tous les autres cas garantissent d'avoir une pte valide ?
    pte_t pte = uvpt[(uint32_t)addr >> PGSHIFT];
    assert(pte & PTE_P); 
    if (!(pte & PTE_COW)) { 
        panic("user space page fault\n");
    }


	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

    addr = (void *) ROUNDDOWN(addr, PGSIZE);

	// LAB 4: Your code here.
    if ((r = sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
            panic("error in fork pagefault\n");

    memmove(PFTEMP, addr, PGSIZE);

    // TODO que devient l'ancienne page mappé a addr ?
    // je pense qu'elle est simplement déréférencée
    if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_P|PTE_U|PTE_W)) < 0)
        panic("error in fork pagefault\n");

    if ((r = sys_page_unmap(0, PFTEMP)) < 0)
        panic("error in fork pagefault\n");
}


// Copie la page à l'adresse addr dans l'espace memoire de dest 
// à la même adresse
// panique en cas d'erreur
// void copy_page(void *addr, envid_t dest) {

//     if ((sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
//         goto error;

//     memmove(PFTEMP, addr, PGSIZE);

//     if ((sys_page_map(0, PFTEMP, dest, addr, PTE_P|PTE_U|PTE_W)) < 0) goto error;

//     if ((sys_page_unmap(0, PFTEMP)) < 0) goto error;

//     return;

// error:
//         panic("error in copy_page\n");

// }


//
// void duppage(uint32_t pn, envid_t dest) {

//     void *addr = (void *) (pn * PGSIZE);
//     if ((sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
//         goto error;

//     memmove(PFTEMP, addr, PGSIZE);

//     if ((sys_page_map(0, PFTEMP, dest, addr, PTE_P|PTE_U|PTE_W)) < 0) goto error;

//     if ((sys_page_unmap(0, PFTEMP)) < 0) goto error;

//     return;

// error:
//         panic("error in duppage\n");

// }


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
// TODO: je ne comprends pas bien le découpage proposé,
// on est obligé de chercher deux fois le pte
static void
duppage(envid_t envid, uint32_t pn)
{
    pte_t pte = uvpt[pn]; 
    void *addr = (void *) (pn << PGSHIFT);
    if (((pte & PTE_W) || (pte & PTE_COW)) && !(pte & PTE_SHARE)) {
       if ((sys_page_map(0, addr, envid, addr, PTE_P|PTE_U|PTE_COW)) < 0) 
          panic("mark cow");
       if ((sys_page_map(0, addr, 0, addr, PTE_P|PTE_U|PTE_COW)) < 0) 
          panic("mark cow");
    } else {
       sys_page_map(0, addr, envid, addr, (PTE_SYSCALL | PTE_AVAIL) & pte);
    } 
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
   	envid_t envid;
    unsigned pn;
	int r;
	
    set_pgfault_handler(pgfault);

	// Allocate a new child environment.
	// The kernel will initialize it with a copy of our register state,
	// so that the child will appear to have called sys_exofork() too -
	// except that in the child, this "fake" call to sys_exofork()
	// will return 0 instead of the envid of the child.
	envid = sys_exofork();
	if (envid < 0) {
        // unable to create new environment
        return -1;
    }
	if (envid == 0) {
    	// We're the child.
		// The copied value of the global variable 'thisenv'
		// is no longer valid (it refers to the parent!).
		// Fix it and return 0.
		thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
	}

    uint32_t pn_uxstacktop = (UXSTACKTOP >> PGSHIFT) - 1;

    if ((sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
        panic ("Can't alloc exception stack in child process");

    int pgdx;

    for (pgdx = 0; pgdx < UTOP >> 22; ++pgdx)   {
        pte_t pde = uvpd[pgdx];
        if (pde & PTE_P) {
            int ptx;
            for (ptx = 0; ptx < 1024; ++ptx) {
                uint32_t pn = (pgdx << 10) + ptx;
                pte_t pte = uvpt[pn]; 
                if (!(pte & PTE_P)) continue; 
                if (pn == pn_uxstacktop) continue; 
                duppage(envid, pn);
           }
        }
    }
    register uint32_t esp asm("esp");

    r = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
    if (r == -1) {
        panic("Can't set child process pgfault handler\n");
    }

	// Start the child environment running
    if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
        return -1;
    }

    return envid;
}


// Challenge!
int
sfork(void)
{
	panic("sfork not implemented\n");
	return -E_INVAL;
}
