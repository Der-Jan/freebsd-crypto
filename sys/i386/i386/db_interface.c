/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 * $FreeBSD$
 */

/*
 * Interface to new debugger.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/cons.h>
#include <sys/linker_set.h>
#include <sys/ktr.h>

#include <machine/cpu.h>
#ifdef SMP
#include <machine/smp.h>
#include <machine/smptests.h>	/** CPUSTOP_ON_DDBBREAK */
#endif

#include <vm/vm.h>
#include <vm/pmap.h>

#include <ddb/ddb.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>

#include <setjmp.h>

static jmp_buf *db_nofault = 0;
extern jmp_buf	db_jmpbuf;

extern void	gdb_handle_exception __P((db_regs_t *, int, int));

int	db_active;
db_regs_t ddb_regs;

static jmp_buf	db_global_jmpbuf;
static int	db_global_jmpbuf_valid;

#ifdef __GNUC__
#define	rss() ({u_short ss; __asm __volatile("mov %%ss,%0" : "=r" (ss)); ss;})
#endif

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(type, code, regs)
	int	type, code;
	register struct i386_saved_state *regs;
{
	volatile int ddb_mode = !(boothowto & RB_GDB);

	/*
	 * XXX try to do nothing if the console is in graphics mode.
	 * Handle trace traps (and hardware breakpoints...) by ignoring
	 * them except for forgetting about them.  Return 0 for other
	 * traps to say that we haven't done anything.  The trap handler
	 * will usually panic.  We should handle breakpoint traps for
	 * our breakpoints by disarming our breakpoints and fixing up
	 * %eip.
	 */
	if (cons_unavail && ddb_mode) {
	    if (type == T_TRCTRAP) {
		regs->tf_eflags &= ~PSL_T;
		return (1);
	    }
	    return (0);
	}

	switch (type) {
	    case T_BPTFLT:	/* breakpoint */
	    case T_TRCTRAP:	/* debug exception */
		break;

	    default:
		/*
		 * XXX this is almost useless now.  In most cases,
		 * trap_fatal() has already printed a much more verbose
		 * message.  However, it is dangerous to print things in
		 * trap_fatal() - printf() might be reentered and trap.
		 * The debugger should be given control first.
		 */
		if (ddb_mode)
		    db_printf("kernel: type %d trap, code=%x\n", type, code);

		if (db_nofault) {
		    jmp_buf *no_fault = db_nofault;
		    db_nofault = 0;
		    longjmp(*no_fault, 1);
		}
	}

	/*
	 * This handles unexpected traps in ddb commands, including calls to
	 * non-ddb functions.  db_nofault only applies to memory accesses by
	 * internal ddb commands.
	 */
	if (db_global_jmpbuf_valid)
	    longjmp(db_global_jmpbuf, 1);

	/*
	 * XXX We really should switch to a local stack here.
	 */
	ddb_regs = *regs;

	/*
	 * If in kernel mode, esp and ss are not saved, so dummy them up.
	 */
	if (ISPL(regs->tf_cs) == 0) {
	    ddb_regs.tf_esp = (int)&regs->tf_esp;
	    ddb_regs.tf_ss = rss();
	}

#ifdef SMP
#ifdef CPUSTOP_ON_DDBBREAK

#if defined(VERBOSE_CPUSTOP_ON_DDBBREAK)
	db_printf("\nCPU%d stopping CPUs: 0x%08x...", cpuid, other_cpus);
#endif /* VERBOSE_CPUSTOP_ON_DDBBREAK */

	/* We stop all CPUs except ourselves (obviously) */
	stop_cpus(other_cpus);

#if defined(VERBOSE_CPUSTOP_ON_DDBBREAK)
	db_printf(" stopped.\n");
#endif /* VERBOSE_CPUSTOP_ON_DDBBREAK */

#endif /* CPUSTOP_ON_DDBBREAK */
#endif /* SMP */

	(void) setjmp(db_global_jmpbuf);
	db_global_jmpbuf_valid = TRUE;
	db_active++;
	if (ddb_mode) {
	    cndbctl(TRUE);
	    db_trap(type, code);
	    cndbctl(FALSE);
	} else
	    gdb_handle_exception(&ddb_regs, type, code);
	db_active--;
	db_global_jmpbuf_valid = FALSE;

#ifdef SMP
#ifdef CPUSTOP_ON_DDBBREAK

#if defined(VERBOSE_CPUSTOP_ON_DDBBREAK)
	db_printf("\nCPU%d restarting CPUs: 0x%08x...", cpuid, stopped_cpus);
#endif /* VERBOSE_CPUSTOP_ON_DDBBREAK */

	/* Restart all the CPUs we previously stopped */
	if (stopped_cpus != other_cpus && smp_started != 0) {
		db_printf("whoa, other_cpus: 0x%08x, stopped_cpus: 0x%08x\n",
			  other_cpus, stopped_cpus);
		panic("stop_cpus() failed");
	}
	restart_cpus(stopped_cpus);

#if defined(VERBOSE_CPUSTOP_ON_DDBBREAK)
	db_printf(" restarted.\n");
#endif /* VERBOSE_CPUSTOP_ON_DDBBREAK */

#endif /* CPUSTOP_ON_DDBBREAK */
#endif /* SMP */

	regs->tf_eip    = ddb_regs.tf_eip;
	regs->tf_eflags = ddb_regs.tf_eflags;
	regs->tf_eax    = ddb_regs.tf_eax;
	regs->tf_ecx    = ddb_regs.tf_ecx;
	regs->tf_edx    = ddb_regs.tf_edx;
	regs->tf_ebx    = ddb_regs.tf_ebx;

	/*
	 * If in user mode, the saved ESP and SS were valid, restore them.
	 */
	if (ISPL(regs->tf_cs)) {
	    regs->tf_esp = ddb_regs.tf_esp;
	    regs->tf_ss  = ddb_regs.tf_ss & 0xffff;
	}

	regs->tf_ebp    = ddb_regs.tf_ebp;
	regs->tf_esi    = ddb_regs.tf_esi;
	regs->tf_edi    = ddb_regs.tf_edi;
	regs->tf_es     = ddb_regs.tf_es & 0xffff;
	regs->tf_fs     = ddb_regs.tf_fs & 0xffff;
	regs->tf_cs     = ddb_regs.tf_cs & 0xffff;
	regs->tf_ds     = ddb_regs.tf_ds & 0xffff;
	return (1);
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(addr, size, data)
	vm_offset_t	addr;
	register size_t	size;
	register char	*data;
{
	register char	*src;

	db_nofault = &db_jmpbuf;

	src = (char *)addr;
	while (size-- > 0)
	    *data++ = *src++;

	db_nofault = 0;
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(addr, size, data)
	vm_offset_t	addr;
	register size_t	size;
	register char	*data;
{
	register char	*dst;

	unsigned	*ptep0 = NULL;
	unsigned	oldmap0 = 0;
	vm_offset_t	addr1;
	unsigned	*ptep1 = NULL;
	unsigned	oldmap1 = 0;

	db_nofault = &db_jmpbuf;

	if (addr > trunc_page((vm_offset_t)btext) - size &&
	    addr < round_page((vm_offset_t)etext)) {

	    ptep0 = pmap_pte(kernel_pmap, addr);
	    oldmap0 = *ptep0;
	    *ptep0 |= PG_RW;

	    /* Map another page if the data crosses a page boundary. */
	    if ((*ptep0 & PG_PS) == 0) {
	    	addr1 = trunc_page(addr + size - 1);
	    	if (trunc_page(addr) != addr1) {
		    ptep1 = pmap_pte(kernel_pmap, addr1);
		    oldmap1 = *ptep1;
		    *ptep1 |= PG_RW;
	    	}
	    } else {
		addr1 = trunc_4mpage(addr + size - 1);
		if (trunc_4mpage(addr) != addr1) {
		    ptep1 = pmap_pte(kernel_pmap, addr1);
		    oldmap1 = *ptep1;
		    *ptep1 |= PG_RW;
		}
	    }

	    invltlb();
	}

	dst = (char *)addr;

	while (size-- > 0)
	    *dst++ = *data++;

	db_nofault = 0;

	if (ptep0) {
	    *ptep0 = oldmap0;

	    if (ptep1)
		*ptep1 = oldmap1;

	    invltlb();
	}
}

/*
 * XXX
 * Move this to machdep.c and allow it to be called if any debugger is
 * installed.
 */
void
Debugger(msg)
	const char *msg;
{
	static volatile	u_int in_Debugger;
	int		flags;
	/*
	 * XXX
	 * Do nothing if the console is in graphics mode.  This is
	 * OK if the call is for the debugger hotkey but not if the call
	 * is a weak form of panicing.
	 */
	if (cons_unavail && !(boothowto & RB_GDB))
	    return;

	if (atomic_cmpset_int(&in_Debugger, 0, 1)) {
	    flags = save_intr();
	    disable_intr();
	    db_printf("Debugger(\"%s\")\n", msg);
	    breakpoint();
            restore_intr(flags);
	    in_Debugger = 0;
	}
}

#if defined(KTR)

struct tstate {
	int	cur;
	int	first;
};
static struct tstate tstate;
static struct timespec lastt;
static int db_mach_vtrace(void);

DB_COMMAND(tbuf, db_mach_tbuf)
{
	struct ktr_entry	*k1, *ck, *kend;
	struct timespec		newk;

	k1 = ktr_buf;
	ck = k1;
	timespecclear(&newk);
	kend = ktr_buf + KTR_ENTRIES;
	while (k1 != kend) {
		if (timespecisset(&k1->ktr_tv) &&
		    timespeccmp(&k1->ktr_tv, &newk, >)) {
			newk = k1->ktr_tv;
			ck = k1;
		}
		k1++;
	}
	tstate.cur = ck - ktr_buf;
	tstate.first = tstate.cur | 0x80000000;
	timespecclear(&lastt);
	db_mach_vtrace();

	return;
}

DB_COMMAND(tall, db_mach_tall)
{
	int	c;

	db_mach_tbuf(addr, have_addr, count, modif);
	while (db_mach_vtrace()) {
		c = cncheckc();
		if (c != -1)
			break;
	}

	return;
}

DB_COMMAND(tnext, db_mach_tnext)
{
	db_mach_vtrace();
}

static int
db_mach_vtrace(void)
{
	struct ktr_entry	*kp;
	struct timespec		ts;
	char			*d;

	kp = NULL;
	if (tstate.cur != tstate.first)
		kp = ktr_buf + tstate.cur;
	else
		kp = NULL;

	if (!kp) {
		db_printf("--- End of trace buffer ---\n");
		return (0);
	}

	d = kp->ktr_desc;
	if (d == NULL) 
		d = "*** Empty ***";
	else if (lastt.tv_sec == 0) {
		db_printf("Newest entry at clock %ld.%06ld\n",
			  kp->ktr_tv.tv_sec,
			  kp->ktr_tv.tv_nsec / 1000);
		lastt = kp->ktr_tv;
	}
	ts = lastt;
	db_printf("%4ld.%06ld: ", ts.tv_sec, ts.tv_nsec / 1000);
	lastt = kp->ktr_tv;
#ifdef KTR_EXTEND
	db_printf("cpu%d %s.%d\t%s", kp->ktr_cpu, kp->ktr_filename,
		  kp->ktr_line, kp->ktr_desc);
#else
	db_printf(d, kp->ktr_parm1, kp->ktr_parm2, kp->ktr_parm3,
		    kp->ktr_parm4, kp->ktr_parm5);
#endif
	db_printf("\n");
	tstate.first &= ~0x80000000;
	if (--tstate.cur < 0)
		tstate.cur = KTR_ENTRIES - 1;

	return (1);
}

#endif
