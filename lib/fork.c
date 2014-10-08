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


	// LAB 4: Your code here.
    //

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

    if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_P|PTE_U|PTE_W)) < 0)
        panic("error in fork pagefault\n");

    if ((r = sys_page_unmap(0, PFTEMP)) < 0)
        panic("error in fork pagefault\n");
}

// copie le contenu de la page addr dans mon environnement
// vers celui de dest
void duppage(void *addr, envid_t dest) {
//    assert(addr & ~(PGSIZE - 1) == 0);

    if ((sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
        goto error;

    memmove(PFTEMP, addr, PGSIZE);

    if ((sys_page_map(0, PFTEMP, dest, addr, PTE_P|PTE_U|PTE_W)) < 0) goto error;

    if ((sys_page_unmap(0, PFTEMP)) < 0) goto error;

    return;

error:
        panic("error in duppage\n");

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
static void
mark_cow(envid_t envid, uint8_t *addr)
{
    if ((sys_page_map(0, addr, 0, addr, PTE_P|PTE_U|PTE_COW)) < 0)
        panic("mark cow");
    if ((sys_page_map(0, addr, envid, addr, PTE_P|PTE_U|PTE_COW)) < 0)
        panic("mark cow");
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

    duppage((void *) (USTACKTOP - PGSIZE), envid);

    // set pgfault_handler
    set_pgfault_handler(pgfault);

    duppage((void *) (UXSTACKTOP - PGSIZE), envid);

    int pgdx;

    for (pgdx = 0; pgdx < UTOP >> 22; ++pgdx)   {
        pte_t pde = uvpd[pgdx];
        if (pde & PTE_P) {
            int ptx;
            for (ptx = 0; ptx < 1024; ++ptx) {
                pte_t pte = uvpt[pgdx * 1024 + ptx]; 
                uint8_t *addr = (uint8_t *) ((pgdx << 22) + (ptx << 12));
                if (addr == (void *) (USTACKTOP - PGSIZE)) continue; // TODO ugly hack
                if (addr == (void *) (UXSTACKTOP - PGSIZE)) continue; // TODO ugly hack
                if (!(pte & PTE_P)) continue; 
                if ((pte & PTE_W) || (pte & PTE_COW)) {
                   mark_cow(envid, addr);
               } else {
                   sys_page_map(0, addr, envid, addr, PTE_P|PTE_U);
               } 
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
