#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/cpu.h>
#include <kern/spinlock.h>

static struct Taskstate ts;

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case.
 */
static struct Trapframe *last_tf;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
		return "Hardware Interrupt";
	return "(unknown trap)";
}

extern void divide_handler();
extern void debug_handler();
extern void nmi_handler();
extern void brkpt_handler();
extern void oflow_handler();
extern void bound_handler();
extern void illop_handler();
extern void device_handler();
extern void dblflt_handler();
extern void tss_handler();
extern void segnp_handler();
extern void stack_handler();
extern void gpflt_handler();
extern void pgflt_handler();
extern void fperr_handler();
extern void align_handler();
extern void mchk_handler();
extern void simderr_handler();
extern void syscall_handler();
extern void default_handler();
extern void timer_handler();

void
trap_init(void)
{
	extern struct Segdesc gdt[]; // A quoi sert ce truc ?
    extern void divide_handler();
	// LAB 3: Your code here.
    SETGATE(idt[T_DIVIDE], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) divide_handler, 
            3 /* dpl */ )
    SETGATE(idt[T_DEBUG], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) debug_handler, 
            3 /* dpl */ )
    SETGATE(idt[T_NMI], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) nmi_handler, 
            3 /* dpl */ )
   SETGATE(idt[T_BRKPT], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) brkpt_handler, 
            3 /* dpl */ )
   SETGATE(idt[T_OFLOW], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) oflow_handler, 
            3 /* dpl */ )
   SETGATE(idt[T_BOUND], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) bound_handler, 
            3 /* dpl */ )
   SETGATE(idt[T_ILLOP], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) illop_handler, 
            3 /* dpl */ )
   SETGATE(idt[T_DEVICE], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) device_handler, 
            3 /* dpl */ )
   SETGATE(idt[T_DBLFLT], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) dblflt_handler, 
            3 /* dpl */ )
   SETGATE(idt[T_TSS], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) tss_handler, 
            3 /* dpl */ )
   SETGATE(idt[T_SEGNP], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) segnp_handler, 
            3 /* dpl */ )
   SETGATE(idt[T_STACK], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) stack_handler, 
            3 /* dpl */ )
   SETGATE(idt[T_GPFLT], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) gpflt_handler, 
            3 /* dpl */ )
    SETGATE(idt[T_PGFLT], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) pgflt_handler, 
            0 /* dpl */ )
    SETGATE(idt[T_FPERR], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) fperr_handler, 
            3 /* dpl */ )
    SETGATE(idt[T_ALIGN], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) align_handler, 
            3 /* dpl */ )
    SETGATE(idt[T_MCHK], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) mchk_handler, 
            3 /* dpl */ )
    SETGATE(idt[T_SIMDERR], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) simderr_handler, 
            3 /* dpl */ )
    SETGATE(idt[T_SYSCALL], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) syscall_handler, 
            3 /* dpl */ )
    SETGATE(idt[T_DEFAULT], 
            1 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) default_handler, 
            3 /* dpl */ )

    // IRQ SETTING
    SETGATE(idt[IRQ_OFFSET+IRQ_TIMER], 
            0 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) timer_handler, 
            3 /* dpl */ )

           // TODO : quel numero d'interruption généré par le default_handler 
            // si j'affiche le trapno ?
    SETGATE(idt[IRQ_OFFSET+IRQ_KBD], 
            0 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) default_handler, 
            3 /* dpl */ )
    SETGATE(idt[IRQ_OFFSET+IRQ_SERIAL], 
            0 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) default_handler, 
            3 /* dpl */ )
    SETGATE(idt[IRQ_OFFSET+IRQ_SPURIOUS], 
            0 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) default_handler, 
            3 /* dpl */ )
    SETGATE(idt[IRQ_OFFSET+IRQ_IDE], 
            0 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) default_handler, 
            3 /* dpl */ )
    SETGATE(idt[IRQ_OFFSET+IRQ_ERROR], 
            0 /* istrap */, 
            GD_KT /* segment selector */, 
            (void *) default_handler, 
            3 /* dpl */ )

	// Per-CPU setup 
	trap_init_percpu();
}

// Initialize and load the per-CPU TSS and IDT
void
trap_init_percpu(void)
{
	// The example code here sets up the Task State Segment (TSS) and
	// the TSS descriptor for CPU 0. But it is incorrect if we are
	// running on other CPUs because each CPU has its own kernel stack.
	// Fix the code so that it works for all CPUs.
	//
	// Hints:
	//   - The macro "thiscpu" always refers to the current CPU's
	//     struct CpuInfo;
	//   - The ID of the current CPU is given by cpunum() or
	//     thiscpu->cpu_id;
	//   - Use "thiscpu->cpu_ts" as the TSS for the current CPU,
	//     rather than the global "ts" variable;
	//   - Use gdt[(GD_TSS0 >> 3) + i] for CPU i's TSS descriptor;
	//   - You mapped the per-CPU kernel stacks in mem_init_mp()
	//
	// ltr sets a 'busy' flag in the TSS selector, so if you
	// accidentally load the same TSS on more than one CPU, you'll
	// get a triple fault.  If you set up an individual CPU's TSS
	// wrong, you may not get a fault until you try to return from
	// user space on that CPU.
	//
	// LAB 4: Your code here:

	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.

    int i = cpunum(); 

	cpus[i].cpu_ts.ts_esp0 = KSTACKTOP - i * (KSTKSIZE + KSTKGAP);
	cpus[i].cpu_ts.ts_ss0 = GD_KD;

	// Initialize the TSS slot of the gdt.
	gdt[(GD_TSS0 >> 3) + i] = SEG16(STS_T32A, (uint32_t) (&cpus[i].cpu_ts),
					sizeof(struct Taskstate), 0);
	gdt[(GD_TSS0 >> 3) + i].sd_s = 0;

	// Load the TSS selector (like other segment selectors, the
	// bottom three bits are special; we leave them 0)
	ltr(((GD_TSS0 >> 3) + i) << 3);

	// Load the IDT
	lidt(&idt_pd);
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p from CPU %d\n", tf, cpunum());
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	if (tf == last_tf && tf->tf_trapno == T_PGFLT)
		cprintf("  cr2  0x%08x\n", rcr2());
	cprintf("  err  0x%08x", tf->tf_err);
	// For page faults, print decoded fault error code:
	// U/K=fault occurred in user/kernel mode
	// W/R=a write/read caused the fault
	// PR=a protection violation caused the fault (NP=page not present).
	if (tf->tf_trapno == T_PGFLT)
		cprintf(" [%s, %s, %s]\n",
			tf->tf_err & FEC_U ? "user" : "kernel",
			tf->tf_err & FEC_WR ? "write" : "read", 
			tf->tf_err & FEC_PR ? "protection" : "not-present");
	else
		cprintf("\n");
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	if ((tf->tf_cs & 3) != 0) {
		cprintf("  esp  0x%08x\n", tf->tf_esp);
		cprintf("  ss   0x----%04x\n", tf->tf_ss);
	}
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

static void
trap_dispatch(struct Trapframe *tf)
{
    // cprintf("dispatching a trap %d\n", tf->tf_trapno);
	// Handle processor exceptions.
	// LAB 3: Your code here.
    
    if (tf->tf_trapno == T_PGFLT) {
        page_fault_handler(tf); // can this function return?
        assert(false);
        return;
    } 
    if (tf->tf_trapno == T_BRKPT) {
        monitor(tf);
        return;
    } 
    if (tf->tf_trapno == T_SYSCALL) {
        tf->tf_regs.reg_eax = 
            syscall(tf->tf_regs.reg_eax, tf->tf_regs.reg_edx, tf->tf_regs.reg_ecx, 
                tf->tf_regs.reg_ebx, tf->tf_regs.reg_edi, tf->tf_regs.reg_esi);
        env_run(curenv);
        // TODO env_pop_tf ou env_run? ou même sched_yield?
        // uniformiser le cas des appels systemes qui rendent la main et ceux
        // qui redonnent la main au schedulers
        //env_pop_tf(tf);
    } 


	// Handle spurious interrupts
	// The hardware sometimes raises these because of noise on the
	// IRQ line or other reasons. We don't care.
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
		cprintf("Spurious interrupt on irq 7\n");
		print_trapframe(tf);
		return;
	}

	// Handle clock interrupts. Don't forget to acknowledge the
	// interrupt using lapic_eoi() before calling the scheduler!
	// LAB 4: Your code here.

    if (tf->tf_trapno == IRQ_OFFSET + IRQ_TIMER) {
		// cprintf("timer interrupt\n");
		lapic_eoi();
    	// print_trapframe(tf);
        sched_yield();
        assert(false);
        return;
    } 


	// Unexpected trap: The user process or the kernel has a bug.
	print_trapframe(tf);
	if (tf->tf_cs == GD_KT)
		panic("unhandled trap in kernel");
	else {
		env_destroy(curenv);
		return;
	}
}

void
trap(struct Trapframe *tf)
{
	// cprintf("Trap: = %s\n", trapname(tf->tf_trapno) );
	// The environment may have set DF and some versions
	// of GCC rely on DF being clear
	asm volatile("cld" ::: "cc");

	// Halt the CPU if some other CPU has called panic()
	extern char *panicstr;
	if (panicstr)
		asm volatile("hlt");

	// Re-acquire the big kernel lock if we were halted in
	// sched_yield()
	if (xchg(&thiscpu->cpu_status, CPU_STARTED) == CPU_HALTED)
		lock_kernel();
	// Check that interrupts are disabled.  If this assertion
	// fails, DO NOT be tempted to fix it by inserting a "cli" in
	// the interrupt path.
    //
	assert(!(read_eflags() & FL_IF));

	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		// Acquire the big kernel lock before doing any
		// serious kernel work.
		// LAB 4: Your code here.
        lock_kernel();
		assert(curenv);
		// Garbage collect if current enviroment is a zombie
		if (curenv->env_status == ENV_DYING) {
			env_free(curenv);
			curenv = NULL;
			sched_yield();
		}

		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}

	// Record that tf is the last real trapframe so
	// print_trapframe can print some additional information.
	last_tf = tf;

	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// If we made it to this point, then no other environment was
	// scheduled, so we should return to the current environment
	// if doing so makes sense.
	if (curenv && curenv->env_status == ENV_RUNNING)
		env_run(curenv);
	else
		sched_yield();
}


void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();
//    cprintf("trap handler: faulting at address %p\n", fault_va);

	// Handle kernel-mode page faults.
    if ((tf->tf_cs & 3) == 0) {
        panic("page fault in kernel mode");
    }

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Call the environment's page fault upcall, if one exists.  Set up a
	// page fault stack frame on the user exception stack (below
	// UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
	//
	// The page fault upcall might cause another page fault, in which case
	// we branch to the page fault upcall recursively, pushing another
	// page fault stack frame on top of the user exception stack.
	//
	// The trap handler needs one word of scratch space at the top of the
	// trap-time stack in order to return.  In the non-recursive case, we
	// don't have to worry about this because the top of the regular user
	// stack is free.  In the recursive case, this means we have to leave
	// an extra word between the current top of the exception stack and
	// the new stack frame because the exception stack _is_ the trap-time
	// stack.
	//
	// If there's no page fault upcall, the environment didn't allocate a
	// page for its exception stack or can't write to it, or the exception
	// stack overflows, then destroy the environment that caused the fault.
	// Note that the grade script assumes you will first check for the page
	// fault upcall and print the "user fault va" message below if there is
	// none.  The remaining three checks can be combined into a single test.
	//
	// Hints:
	//   user_mem_assert() and env_run() are useful here.
	//   To change what the user environment runs, modify 'curenv->env_tf'
	//   (the 'tf' variable points at 'curenv->env_tf').

	// LAB 4: Your code here.

    // if no handler, destroy the environment that caused the fault.
    if (!curenv->env_pgfault_upcall) {
        cprintf("[%08x] user fault va %08x ip %08x\n",
            curenv->env_id, fault_va, tf->tf_eip);
        print_trapframe(tf);
        env_destroy(curenv);
        return;
    }

    uint32_t *stack = ((uint32_t *) UXSTACKTOP) - 1;
    // if this is recursive page fault, we add an extra word
    if (tf->tf_esp < UXSTACKTOP && tf->tf_esp >= UXSTACKTOP - PGSIZE) {
        stack = ((uint32_t *) tf->tf_esp) - 1;
    } 

    // TODO: vérifier ces bornes
    user_mem_assert(curenv, stack - sizeof(struct UTrapframe) + 1, sizeof(struct UTrapframe), PTE_W);

    // Call the environment's page fault upcall, if one exists.  Set up a
	// page fault stack frame on the user exception stack (below
	// UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
   
    // TODO rewrite this with less instructions
    //
    *stack = tf->tf_esp; stack--;
    *stack = tf->tf_eflags; stack--;
    *stack = tf->tf_eip; stack--;
    *stack = tf->tf_regs.reg_eax; stack--;
    *stack = tf->tf_regs.reg_ecx; stack--;
    *stack = tf->tf_regs.reg_edx; stack--;
    *stack = tf->tf_regs.reg_ebx; stack--;
    *stack = tf->tf_regs.reg_oesp; stack--;
    *stack = tf->tf_regs.reg_ebp; stack--;
    *stack = tf->tf_regs.reg_esi; stack--;
    *stack = tf->tf_regs.reg_edi; stack--;
    *stack = tf->tf_err; stack--; 
    *stack = fault_va; 

    curenv->env_tf.tf_eip = (uintptr_t) curenv->env_pgfault_upcall;
    curenv->env_tf.tf_esp = (uintptr_t) stack;

    // TODO env_pop ou env_run ?
    env_run(curenv); 
}

