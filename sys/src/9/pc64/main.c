#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"tos.h"
#include	"ureg.h"
#include	"init.h"
#include	"pool.h"
#include	"reboot.h"

Conf conf;
int delaylink;
int idle_spin;

extern void (*i8237alloc)(void);
extern void bootscreeninit(void);

void
confinit(void)
{
	char *p;
	int i, userpcnt;
	ulong kpages;

	if(p = getconf("service")){
		if(strcmp(p, "cpu") == 0)
			cpuserver = 1;
		else if(strcmp(p,"terminal") == 0)
			cpuserver = 0;
	}

	if(p = getconf("*kernelpercent"))
		userpcnt = 100 - strtol(p, 0, 0);
	else
		userpcnt = 0;

	conf.npage = 0;
	for(i=0; i<nelem(conf.mem); i++)
		conf.npage += conf.mem[i].npage;

	conf.nproc = 100 + ((conf.npage*BY2PG)/MB)*5;
	if(cpuserver)
		conf.nproc *= 3;
	if(conf.nproc > 2000)
		conf.nproc = 2000;
	conf.nimage = 200;
	conf.nswap = conf.nproc*80;
	conf.nswppo = 4096;

	if(cpuserver) {
		if(userpcnt < 10)
			userpcnt = 70;
		kpages = conf.npage - (conf.npage*userpcnt)/100;
		conf.nimage = conf.nproc;
	} else {
		if(userpcnt < 10) {
			if(conf.npage*BY2PG < 16*MB)
				userpcnt = 50;
			else
				userpcnt = 60;
		}
		kpages = conf.npage - (conf.npage*userpcnt)/100;

		/*
		 * Make sure terminals with low memory get at least
		 * 4MB on the first Image chunk allocation.
		 */
		if(conf.npage*BY2PG < 16*MB)
			imagmem->minarena = 4*MB;
	}

	/*
	 * can't go past the end of virtual memory.
	 */
	if(kpages > ((uintptr)-KZERO)/BY2PG)
		kpages = ((uintptr)-KZERO)/BY2PG;

	conf.upages = conf.npage - kpages;
	conf.ialloc = (kpages/2)*BY2PG;

	/*
	 * Guess how much is taken by the large permanent
	 * datastructures. Mntcache and Mntrpc are not accounted for.
	 */
	kpages *= BY2PG;
	kpages -= conf.nproc*sizeof(Proc)
		+ conf.nimage*sizeof(Image)
		+ conf.nswap
		+ conf.nswppo*sizeof(Page*);
	mainmem->maxsize = kpages;

	/*
	 * the dynamic allocation will balance the load properly,
	 * hopefully. be careful with 32-bit overflow.
	 */
	imagmem->maxsize = kpages - (kpages/10);
	if(p = getconf("*imagemaxmb")){
		imagmem->maxsize = strtol(p, nil, 0)*MB;
		if(imagmem->maxsize > mainmem->maxsize)
			imagmem->maxsize = mainmem->maxsize;
	}
}

/*
 * The palloc.pages array can be a large chunk out of the 2GB
 * window above KZERO, so we allocate the array from
 * upages and map in the VMAP window before pageinit()
 */
static void
preallocpages(void)
{
	Pallocmem *pm;
	uintptr va, base, top;
	vlong size;
	ulong np;
	int i;

	np = 0;
	for(i=0; i<nelem(palloc.mem); i++){
		pm = &palloc.mem[i];
		np += pm->npage;
	}
	size = (uvlong)np * BY2PG;
	size += sizeof(Page) + BY2PG;	/* round up */
	size = (size / (sizeof(Page) + BY2PG)) * sizeof(Page);
	size = ROUND(size, PGLSZ(1));

	for(i=0; i<nelem(palloc.mem); i++){
		pm = &palloc.mem[i];
		base = ROUND(pm->base, PGLSZ(1));
		top = pm->base + (uvlong)pm->npage * BY2PG;
		if((base + size) <= VMAPSIZE && (vlong)(top - base) >= size){
			va = base + VMAP;
			pmap(m->pml4, base | PTEGLOBAL|PTEWRITE|PTEVALID, va, size);
			palloc.pages = (Page*)va;
			pm->base = base + size;
			pm->npage = (top - pm->base)/BY2PG;
			break;
		}
	}
}

void
machinit(void)
{
	int machno;
	Segdesc *gdt;
	uintptr *pml4;

	machno = m->machno;
	pml4 = m->pml4;
	gdt = m->gdt;
	memset(m, 0, sizeof(Mach));
	m->machno = machno;
	m->pml4 = pml4;
	m->gdt = gdt;
	m->perf.period = 1;

	/*
	 * For polled uart output at boot, need
	 * a default delay constant. 100000 should
	 * be enough for a while. Cpuidentify will
	 * calculate the real value later.
	 */
	m->loopconst = 100000;
}

void
mach0init(void)
{
	conf.nmach = 1;

	MACHP(0) = (Mach*)CPU0MACH;

	m->machno = 0;
	m->pml4 = (u64int*)CPU0PML4;
	m->gdt = (Segdesc*)CPU0GDT;

	machinit();

	active.machs[0] = 1;
	active.exiting = 0;
}

void
init0(void)
{
	char buf[2*KNAMELEN], **sp;

	up->nerrlab = 0;

	spllo();

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	up->slash = namec("#/", Atodir, 0, 0);
	pathclose(up->slash->path);
	up->slash->path = newpath("/");
	up->dot = cclone(up->slash);

	chandevinit();

	if(!waserror()){
		snprint(buf, sizeof(buf), "%s %s", arch->id, conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "amd64", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		setconfenv();
		poperror();
	}
	kproc("alarm", alarmkproc, 0);

	sp = (char**)(USTKTOP - sizeof(Tos) - 8 - sizeof(sp[0])*4);
	sp[3] = sp[2] = nil;
	strcpy(sp[1] = (char*)&sp[4], "boot");
	sp[0] = nil;
	touser(sp);
}

void
userinit(void)
{
	void *v;
	Proc *p;
	Segment *s;
	Page *pg;

	p = newproc();
	p->pgrp = newpgrp();
	p->egrp = smalloc(sizeof(Egrp));
	p->egrp->ref = 1;
	p->fgrp = dupfgrp(nil);
	p->rgrp = newrgrp();
	p->procmode = 0640;

	kstrdup(&eve, "");
	kstrdup(&p->text, "*init*");
	kstrdup(&p->user, eve);

	procsetup(p);

	/*
	 * Kernel Stack
	 *
	 * N.B. make sure there's enough space for syscall to check
	 *	for valid args and 
	 *	8 bytes for gotolabel's return PC
	 */
	p->sched.pc = (uintptr)init0;
	p->sched.sp = (uintptr)p->kstack+KSTACK-(sizeof(Sargs)+BY2WD);

	/* temporarily set up for kmap() */
	up = p;

	/*
	 * User Stack
	 */
	s = newseg(SG_STACK, USTKTOP-USTKSIZE, USTKSIZE/BY2PG);
	p->seg[SSEG] = s;
	pg = newpage(0, 0, USTKTOP-BY2PG);
	segpage(s, pg);
	v = kmap(pg);
	memset(v, 0, BY2PG);
	kunmap(v);

	/*
	 * Text
	 */
	s = newseg(SG_TEXT, UTZERO, 1);
	s->flushme++;
	p->seg[TSEG] = s;
	pg = newpage(0, 0, UTZERO);
	pg->txtflush = ~0;
	segpage(s, pg);
	v = kmap(pg);
	memset(v, 0, BY2PG);
	memmove(v, initcode, sizeof initcode);
	kunmap(v);

	/* free kmap */
	mmurelease(p);
	up = nil;

	ready(p);
}

void
main()
{
	mach0init();
	bootargsinit();
	ioinit();
	i8250console();
	quotefmtinstall();
	screeninit();
	print("\nPlan 9\n");
	trapinit0();
	i8253init();
	cpuidentify();
	meminit();
	confinit();
	xinit();
	archinit();
	bootscreeninit();
	if(i8237alloc != nil)
		i8237alloc();
	trapinit();
	printinit();
	cpuidprint();
	mmuinit();
	if(arch->intrinit)
		arch->intrinit();
	timersinit();
	mathinit();
	if(arch->clockenable)
		arch->clockenable();
	procinit0();
	initseg();
	if(delaylink){
		bootlinks();
		pcimatch(0, 0, 0);
	}else
		links();
	chandevreset();
	netconsole();
	preallocpages();
	pageinit();
	swapinit();
	userinit();
	schedinit();
}

void
exit(int)
{
	cpushutdown();
	arch->reset();
}

void
reboot(void *entry, void *code, ulong size)
{
	void (*f)(uintptr, uintptr, ulong);

	writeconf();

	/*
	 * the boot processor is cpu0.  execute this function on it
	 * so that the new kernel has the same cpu0.  this only matters
	 * because the hardware has a notion of which processor was the
	 * boot processor and we look at it at start up.
	 */
	if (m->machno != 0) {
		procwired(up, 0);
		sched();
	}
	cpushutdown();

	splhi();

	/* turn off buffered serial console */
	serialoq = nil;

	/* shutdown devices */
	chandevshutdown();
	arch->introff();

	/*
	 * This allows the reboot code to turn off the page mapping
	 */
	*mmuwalk(m->pml4, 0, 3, 0) = *mmuwalk(m->pml4, KZERO, 3, 0);
	*mmuwalk(m->pml4, 0, 2, 0) = *mmuwalk(m->pml4, KZERO, 2, 0);
	mmuflushtlb();

	/* setup reboot trampoline function */
	f = (void*)REBOOTADDR;
	memmove(f, rebootcode, sizeof(rebootcode));

	/* off we go - never to return */
	coherence();
	(*f)((uintptr)entry & ~0xF0000000UL, (uintptr)PADDR(code), size);
}

/*
 * SIMD Floating Point.
 * Assembler support to get at the individual instructions
 * is in l.s.
 * There are opportunities to be lazier about saving and
 * restoring the state and allocating the storage needed.
 */
extern void _clts(void);
extern void _fldcw(u16int);
extern void _fnclex(void);
extern void _fninit(void);
extern void _fxrstor(Fxsave*);
extern void _fxsave(Fxsave*);
extern void _fwait(void);
extern void _ldmxcsr(u32int);
extern void _stts(void);

/*
 * not used, AMD64 mandated SSE
 */
void
fpx87save(FPsave*)
{
}
void
fpx87restore(FPsave*)
{
}

void
fpssesave(FPsave *fps)
{
	Fxsave *fx = (Fxsave*)ROUND(((uintptr)fps), FPalign);

	_fxsave(fx);
	_stts();
	if(fx != (Fxsave*)fps)
		memmove((Fxsave*)fps, fx, sizeof(Fxsave));
}
void
fpsserestore(FPsave *fps)
{
	Fxsave *fx = (Fxsave*)ROUND(((uintptr)fps), FPalign);

	if(fx != (Fxsave*)fps)
		memmove(fx, (Fxsave*)fps, sizeof(Fxsave));
	_clts();
	_fxrstor(fx);
}

static char* mathmsg[] =
{
	nil,	/* handled below */
	"denormalized operand",
	"division by zero",
	"numeric overflow",
	"numeric underflow",
	"precision loss",
};

static void
mathnote(ulong status, uintptr pc)
{
	char *msg, note[ERRMAX];
	int i;

	/*
	 * Some attention should probably be paid here to the
	 * exception masks and error summary.
	 */
	msg = "unknown exception";
	for(i = 1; i <= 5; i++){
		if(!((1<<i) & status))
			continue;
		msg = mathmsg[i];
		break;
	}
	if(status & 0x01){
		if(status & 0x40){
			if(status & 0x200)
				msg = "stack overflow";
			else
				msg = "stack underflow";
		}else
			msg = "invalid operation";
	}
	snprint(note, sizeof note, "sys: fp: %s fppc=%#p status=0x%lux",
		msg, pc, status);
	postnote(up, 1, note, NDebug);
}

/*
 *  math coprocessor error
 */
static void
matherror(Ureg*, void*)
{
	/*
	 * Save FPU state to check out the error.
	 */
	fpsave(&up->fpsave);
	up->fpstate = FPinactive;
	mathnote(up->fpsave.fsw, up->fpsave.rip);
}

/*
 *  SIMD error
 */
static void
simderror(Ureg *ureg, void*)
{
	fpsave(&up->fpsave);
	up->fpstate = FPinactive;
	mathnote(up->fpsave.mxcsr & 0x3f, ureg->pc);
}

void
fpinit(void)
{
	/*
	 * A process tries to use the FPU for the
	 * first time and generates a 'device not available'
	 * exception.
	 * Turn the FPU on and initialise it for use.
	 * Set the precision and mask the exceptions
	 * we don't care about from the generic Mach value.
	 */
	_clts();
	_fninit();
	_fwait();
	_fldcw(0x0232);
	_ldmxcsr(0x1900);
}

/*
 *  math coprocessor emulation fault
 */
static void
mathemu(Ureg *ureg, void*)
{
	ulong status, control;

	if(up->fpstate & FPillegal){
		/* someone did floating point in a note handler */
		postnote(up, 1, "sys: floating point in note handler", NDebug);
		return;
	}
	switch(up->fpstate){
	case FPinit:
		fpinit();
		up->fpstate = FPactive;
		break;
	case FPinactive:
		/*
		 * Before restoring the state, check for any pending
		 * exceptions, there's no way to restore the state without
		 * generating an unmasked exception.
		 * More attention should probably be paid here to the
		 * exception masks and error summary.
		 */
		status = up->fpsave.fsw;
		control = up->fpsave.fcw;
		if((status & ~control) & 0x07F){
			mathnote(status, up->fpsave.rip);
			break;
		}
		fprestore(&up->fpsave);
		up->fpstate = FPactive;
		break;
	case FPactive:
		panic("math emu pid %ld %s pc %#p", 
			up->pid, up->text, ureg->pc);
		break;
	}
}

/*
 *  math coprocessor segment overrun
 */
static void
mathover(Ureg*, void*)
{
	pexit("math overrun", 0);
}

void
mathinit(void)
{
	trapenable(VectorCERR, matherror, 0, "matherror");
	if(X86FAMILY(m->cpuidax) == 3)
		intrenable(IrqIRQ13, matherror, 0, BUSUNKNOWN, "matherror");
	trapenable(VectorCNA, mathemu, 0, "mathemu");
	trapenable(VectorCSO, mathover, 0, "mathover");
	trapenable(VectorSIMD, simderror, 0, "simderror");
}

void
procsetup(Proc *p)
{
	p->fpstate = FPinit;
	_stts();
	cycles(&p->kentry);
	p->pcycles = -p->kentry;
}

void
procfork(Proc *p)
{
	int s;

	p->kentry = up->kentry;
	p->pcycles = -p->kentry;

	/* save floating point state */
	s = splhi();
	switch(up->fpstate & ~FPillegal){
	case FPactive:
		fpsave(&up->fpsave);
		up->fpstate = FPinactive;
	case FPinactive:
		p->fpsave = up->fpsave;
		p->fpstate = FPinactive;
	}
	splx(s);

}

void
procrestore(Proc *p)
{
	uvlong t;
	
	if(p->dr[7] != 0){
		m->dr7 = p->dr[7];
		putdr(p->dr);
	}

	if(p->kp)
		return;

	cycles(&t);
	p->kentry += t;
	p->pcycles -= t;
}

void
procsave(Proc *p)
{
	uvlong t;
	
	if(m->dr7 != 0){
		m->dr7 = 0;
		putdr7(0);
	}

	cycles(&t);
	p->kentry -= t;
	p->pcycles += t;

	if(p->fpstate == FPactive){
		if(p->state == Moribund){
			_clts();
			_fnclex();
			_stts();
		}
		else{
			/*
			 * Fpsave() stores without handling pending
			 * unmasked exeptions. Postnote() can't be called
			 * here as sleep() already has up->rlock, so
			 * the handling of pending exceptions is delayed
			 * until the process runs again and generates an
			 * emulation fault to activate the FPU.
			 */
			fpsave(&p->fpsave);
		}
		p->fpstate = FPinactive;
	}

	/*
	 * While this processor is in the scheduler, the process could run
	 * on another processor and exit, returning the page tables to
	 * the free list where they could be reallocated and overwritten.
	 * When this processor eventually has to get an entry from the
	 * trashed page tables it will crash.
	 *
	 * If there's only one processor, this can't happen.
	 * You might think it would be a win not to do this in that case,
	 * especially on VMware, but it turns out not to matter.
	 */
	mmuflushtlb();
}
