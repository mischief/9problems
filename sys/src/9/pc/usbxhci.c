#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"../port/usb.h"

enum {
	/* Capability Registers */
	CAPLENGTH	= 0x00/4,	// 1
	HCIVERSION	= 0x02/4,	// 2
	HCSPARAMS1	= 0x04/4,
	HCSPARAMS2	= 0x08/4,
	HCSPARAMS3	= 0x0C/4,

	HCCPARAMS	= 0x10/4,
		AC64	= 1<<0,
		BNC	= 1<<1,
		CSZ	= 1<<2,
		PPC	= 1<<3,
		PIND	= 1<<4,
		LHRC	= 1<<5,
		LTC	= 1<<6,
		NSS	= 1<<7,

	DBOFF		= 0x14/4,
	RTSOFF		= 0x18/4,

	/* Operational Registers */
	USBCMD		= 0x00/4,	/* USB Command Register */
		RUNSTOP	= 1<<0,		/* Run/Stop - RW */
		HCRST	= 1<<1,		/* Host Controller Reset - RW */
		INTE	= 1<<2,		/* Interrupter Enable - RW */
		HSEE	= 1<<3,		/* Host System Error Enable - RW */
		LHCRST	= 1<<7,		/* Light Host Controller Reset - RO/RW */
		CSS	= 1<<8,		/* Controller Save State - RW */
		CRS	= 1<<9,		/* Controller Restore State - RW */
		EWE	= 1<<10,	/* Enable Wrap Event - RW */
		EU3S	= 1<<11,	/* Enable U3 MFINDEX Stop - RW */

	USBSTS		= 0x04/4,	/* USB Status Register */
		HCH	= 1<<0,		/* HCHalted - RO */
		HSE	= 1<<2,		/* Host System Error - RW1C */
		EINT	= 1<<3,		/* Event Interrupt - RW1C */
		PCD	= 1<<4,		/* Port Change Detect - RW1C */
		SSS	= 1<<8,		/* Save State Status - RO */
		RSS	= 1<<9,		/* Restore State Status - RO */
		SRE	= 1<<10,	/* Save/Restore Error - RW1C */
		CNR	= 1<<11,	/* Controller Not Ready - RO */
		HCE	= 1<<12,	/* Host Controller Error - RO */

	PAGESIZE	= 0x08/4,	/* Page Size - RO */

	DNCTRL		= 0x14/4,	/* Device Notification Control Register - RW */

	CRCR		= 0x18/4,	/* Command Ring Control Register - RW */
		RCS	= 1<<0,		/* Ring Cycle State - RW */
		CS	= 1<<1,		/* Command Stop - RW1S */
		CA	= 1<<2,		/* Command Abort - RW1S */
		CRR	= 1<<3,		/* Command Ring Running - RO */

	DCBAAP		= 0x30/4,	// 8

	CONFIG		= 0x38/4,	/* Configure Register (MaxSlotEn[7:0]) */

	/* Port Register Set */
	PORTSC		= 0x00/4,	/* Port tatus and Control Register */
		CCS	= 1<<0,		/* Current Connect Status - ROS */
		PED	= 1<<1,		/* Port Enable/Disabled - RW1CS */
		OCA	= 1<<3,		/* Over-current Active - RO */
		PR	= 1<<4,		/* Port Reset - RW1S */
		PLS	= 15<<5,	/* Port Link State - RWS */
		PP	= 1<<9,		/* Port Power - RWS */
		PS	= 15<<10,	/* Port Speed - ROS */
		PIC	= 3<<14,	/* Port Indicator Control - RWS */
		LWS	= 1<<16,	/* Port Link Write Strobe - RW */
		CSC	= 1<<17,	/* Connect Status Change - RW1CS */
		PEC	= 1<<18,	/* Port Enabled/Disabled Change - RW1CS */
		WRC	= 1<<19,	/* Warm Port Reset Change - RW1CS */
		OCC	= 1<<20,	/* Over-current Change - RW1CS */
		PRC	= 1<<21,	/* Port Reset Change - RW1CS */
		PLC	= 1<<22,	/* Port Link State Change - RW1CS */
		CEC	= 1<<23,	/* Port Config Error Change - RW1CS */
		CAS	= 1<<24,	/* Cold Attach Status - RO */
		WCE	= 1<<25,	/* Wake on Connect Enable - RWS */
		WDE	= 1<<26,	/* Wake on Disconnect Enable - RWS */
		WOE	= 1<<27,	/* Wake on Over-current Enable - RWS */
		DR	= 1<<30,	/* Device Removable - RO */
		WPR	= 1<<31,	/* Warm Port Reset - RW1S */

	PORTPMSC	= 0x04/4,
	PORTLI		= 0x08/4,

	/* Host Controller Runtime Register */
	MFINDEX		= 0x0000/4,	/* Microframe Index */
	IR0		= 0x0020/4,	/* Interrupt Register Set 0 */

	/* Interrupter Registers */
	IMAN		= 0x00/4,	/* Interrupter Management */
	IMOD		= 0x04/4,	/* Interrupter Moderation */
	ERSTSZ		= 0x08/4,	/* Event Ring Segment Table Size */
	ERSTBA		= 0x10/4,	/* Event Ring Segment Table Base Address */
	ERDP		= 0x18/4,	/* Event Ring Dequeue Pointer */

	/* TRB types */
	TR_RESERVED	= 0<<10,
	TR_NORMAL	= 1<<10,
	TR_SETUPSTAGE	= 2<<10,
	TR_DATASTAGE	= 3<<10,
	TR_STATUSSTAGE	= 4<<10,
	TR_ISOCH	= 5<<10,
	TR_LINK		= 6<<10,
	TR_EVENTDATA	= 7<<10,
	TR_NOOP		= 8<<10,

	CR_ENABLESLOT	= 9<<10,
	CR_DISABLESLOT	= 10<<10,
	CR_ADDRESSDEV	= 11<<10,
	CR_CONFIGEP	= 12<<10,
	CR_EVALCTX	= 13<<10,
	CR_RESETEP	= 14<<10,
	CR_STOPEP	= 15<<10,
	CR_SETTRDQP	= 16<<10,
	CR_RESETDEV	= 17<<10,
	CR_FORCECMD	= 18<<10,
	CR_NEGBW	= 19<<10,
	CR_SETLAT	= 20<<10,
	CR_GETPORTBW	= 21<<10,
	CR_FORCEHDR	= 22<<10,
	CR_NOOP		= 23<<10,

	ER_TRANSFER	= 32<<10,
	ER_CMDCOMPL	= 33<<10,
	ER_PORTSC	= 34<<10,
	ER_BWREQ	= 35<<10,
	ER_DOORBELL	= 36<<10,
	ER_HCE		= 37<<10,
	ER_DEVNOTE	= 38<<10,
	ER_MFINDEXWRAP	= 39<<10,
};

typedef struct Ctlr Ctlr;
typedef struct Wait Wait;
typedef struct Ring Ring;
typedef struct Slot Slot;
typedef struct Epio Epio;

struct Wait
{
	Wait	*next;
	Ring	*ring;
	u32int	*td;
	u32int	er[4];
	Rendez	*z;
};

struct Ring
{
	int	id;

	Slot	*slot;

	u32int	*base;

	u32int	mask;
	u32int	shift;

	u32int	rp;
	u32int	wp;

	u32int	*ctx;
	u32int	*doorbell;

	Wait	*pending;
	Lock;
};

struct Slot
{
	int	id;

	Ctlr	*ctlr;
	Udev	*dev;

	u32int	*ibase;
	u32int	*obase;

	/* endpoint rings */
	int	nep;
	Ring	epr[32];
};

struct Ctlr
{
	Pcidev	*pcidev;

	u32int	*mmio;

	u32int	*opr;	/* operational registers */
	u32int	*prt;	/* port register set */
	u32int	*rts;	/* runtime registers */
	u32int	*dba;	/* doorbell array */

	u64int	*dcba;	/* device context base array */

	u64int	*sba;	/* scratchpad buffer array */
	void	*sbp;	/* scratchpad buffer pages */

	u32int	*erst[1];	/* event ring segment table */
	Ring	er[1];		/* event ring segment */
	Ring	cr[1];		/* command ring segment */

	u32int	mfwrap;

	QLock	slotlock;
	Slot	**slot;		/* slots by slot id */
	
	u32int	hccparams;

	int	csz;
	int	pagesize;
	int	nscratch;
	int	nintrs;
	int	nslots;

	void	(*setrptr)(u32int*, u64int);

	void	*active;
};

struct Epio
{
	QLock;

	Ring	*ring;
	Block	*b;

	/* iso */
	int	nleft;
	u32int	frame;
	u32int	incr;
	u32int	tdsz;
};

static char Ebadlen[] = "bad usb request length";
static char Enotconfig[] = "usb endpoint not configured";

static void
setrptr32(u32int *reg, u64int pa)
{
	reg[0] = pa;
	reg[1] = pa>>32;
}

static void
setrptr64(u32int *reg, u64int pa)
{
	*((u64int*)reg) = pa;
}

static u32int
mfindex(Ctlr *ctlr)
{
	u32int lo, hi;

	do {
		hi = ctlr->mfwrap;
		lo = ctlr->rts[MFINDEX];
	} while(hi != ctlr->mfwrap);
	return (lo & (1<<14)-1) | hi<<14;
}

static void
freering(Ring *r)
{
	if(r == nil)
		return;
	if(r->base != nil)
		free(r->base);
	memset(r, 0, sizeof(*r));
}

static Ring*
initring(Ring *r, int shift)
{
	r->id = 0;
	r->ctx = nil;
	r->slot = nil;
	r->doorbell = nil;
	r->pending = nil;
	r->shift = shift;
	r->mask = (1<<shift)-1;
	r->rp = r->wp = 0;
	r->base = mallocalign(4*4<<shift, 64, 0, 64*1024);
	if(r->base == nil){
		freering(r);
		error(Enomem);
	}
	return r;
}

static void
init(Hci *hp)
{
	Ctlr *ctlr;
	uchar *p;
	int i;

	ctlr = hp->aux;

	ctlr->opr = &ctlr->mmio[(ctlr->mmio[CAPLENGTH]&0xFF)/4];
	ctlr->prt = &ctlr->opr[0x400/4];
	ctlr->dba = &ctlr->mmio[ctlr->mmio[DBOFF]/4];
	ctlr->rts = &ctlr->mmio[ctlr->mmio[RTSOFF]/4];

	while(ctlr->opr[USBSTS] & CNR)
		tsleep(&up->sleep, return0, nil, 10);

	ctlr->opr[USBCMD] = HCRST;
	while((ctlr->opr[USBSTS] & (CNR|HCH)) != HCH)
		tsleep(&up->sleep, return0, nil, 10);

	pcisetbme(ctlr->pcidev);
	pcisetpms(ctlr->pcidev, 0);
	intrenable(ctlr->pcidev->intl, hp->interrupt, hp, ctlr->pcidev->tbdf, hp->type);

	ctlr->hccparams = ctlr->mmio[HCCPARAMS];
	ctlr->csz = (ctlr->hccparams & CSZ) != 0;
	if(ctlr->hccparams & AC64)
		ctlr->setrptr = setrptr64;
	else
		ctlr->setrptr = setrptr32;

	ctlr->pagesize = ctlr->opr[PAGESIZE]<<12;

	ctlr->nscratch = (ctlr->mmio[HCSPARAMS2] >> 27) & 0x1F;
	ctlr->nintrs = (ctlr->mmio[HCSPARAMS1] >> 8) & 0x7FF;
	ctlr->nslots = (ctlr->mmio[HCSPARAMS1] >> 0) & 0xFF;
	hp->nports = (ctlr->mmio[HCSPARAMS1] >> 24) & 0xFF;

	ctlr->slot = malloc((1+ctlr->nslots)*sizeof(ctlr->slot[0]));
	ctlr->dcba = mallocalign((1+ctlr->nslots)*8, 64, 0, ctlr->pagesize);
	if(ctlr->nscratch != 0){
		ctlr->sba = mallocalign(ctlr->nscratch*8, 64, 0, ctlr->pagesize);
		ctlr->sbp = mallocalign(ctlr->nscratch*ctlr->pagesize, ctlr->pagesize, 0, 0);
		for(i=0, p = ctlr->sbp; i<ctlr->nscratch; i++, p += ctlr->pagesize){
			memset(p, 0, ctlr->pagesize);
			ctlr->sba[i] = PADDR(p);
		}
		ctlr->dcba[0] = PADDR(ctlr->sba);
	} else {
		ctlr->dcba[0] = 0;
	}
	for(i=1; i<=ctlr->nslots; i++)
		ctlr->dcba[i] = 0;
	ctlr->opr[CONFIG] = ctlr->nslots;	/* MaxSlotsEn */
	coherence();
	ctlr->setrptr(&ctlr->opr[DCBAAP], PADDR(ctlr->dcba));

	initring(ctlr->cr, 8);		/* 256 entries */
	ctlr->cr->id = 0;
	ctlr->cr->doorbell = &ctlr->dba[0];
	coherence();
	ctlr->setrptr(&ctlr->opr[CRCR], PADDR(ctlr->cr[0].base) | 1);

	for(i=0; i<ctlr->nintrs; i++){
		u32int *irs = &ctlr->rts[IR0 + i*8];

		if(i >= nelem(ctlr->er)){
			irs[ERSTSZ] = 0;	/* disable ring */
			continue;
		}

		/* allocate and link into event ring segment table */
		initring(&ctlr->er[i], 8);	/* 256 entries */
		ctlr->erst[i] = mallocalign(16, 64, 0, 0);
		*((u64int*)ctlr->erst[i]) = PADDR(ctlr->er[i].base);
		ctlr->erst[i][2] = ctlr->er[i].mask+1;
		ctlr->erst[i][3] = 0;

		irs[ERSTSZ] = 1;	/* just one segment */
		coherence();
		ctlr->setrptr(&irs[ERDP], PADDR(ctlr->er[i].base));
		coherence();
		ctlr->setrptr(&irs[ERSTBA], PADDR(ctlr->erst[i]));

		irs[IMAN] = 3;
		irs[IMOD] = 0;
	}	

	ctlr->mfwrap = 0;

	ctlr->opr[USBCMD] = RUNSTOP|INTE|HSEE|EWE;
	while(ctlr->opr[USBSTS] & (CNR|HCH))
		tsleep(&up->sleep, return0, nil, 10);
}

static void
dump(Hci *)
{
}

static void
queuetd(Ring *ring, u32int c, u32int s, u64int p, Wait *w)
{
	u32int *td, x;

	x = ring->wp++;
	if((x & ring->mask) == ring->mask){
		td = ring->base + 4*(x & ring->mask);
		*(u64int*)td = PADDR(ring->base);
		td[2] = 0;
		td[3] = ((~x>>ring->shift)&1) | (1<<1) | TR_LINK;
		x = ring->wp++;
	}
	td = ring->base + 4*(x & ring->mask);
	if(w != nil){
		memset(w->er, 0, 4*4);
		w->ring = ring;
		w->td = td;
		w->z = &up->sleep;
		w->next = ring->pending;
		ring->pending = w;
	}
	*(u64int*)td = p;
	td[2] = s;
	td[3] = ((~x>>ring->shift)&1) | c;
}

static char *ccerrtab[] = {
[2]	"Data Buffer Error",
[3]	"Babble Detected Error",
[4]	"USB Transaction Error",
[5]	"TRB Error",
[6]	"Stall Error",
[7]	"Resume Error",
[8]	"Bandwidth Error",
[9]	"No Slots Available",
[10]	"Invalid Stream Type",
[11]	"Slot Not Enabled",
[12]	"Endpoint Not Enabled",
[13]	"Short Packet",
[14]	"Ring Underrun",
[15]	"Ring Overrun",
[16]	"VF Event Ring Full",
[17]	"Parameter Error",
[18]	"Bandwidth Overrun Error",
[19]	"Context State Error",
[20]	"No Ping Response",
[21]	"Event Ring Full",
[22]	"Incompatible Device",
[23]	"Missed Service Error",
[24]	"Command Ring Stopped",
[25]	"Command Aborted",
[26]	"Stopped",
[27]	"Stoppe - Length Invalid",
[29]	"Max Exit Latency Too Large",
[31]	"Isoch Buffer Overrun",
[32]	"Event Lost Error",
[33]	"Undefined Error",
[34]	"Invalid Stream ID",
[35]	"Secondary Bandwidth Error",
[36]	"Split Transaction Error",
};

static char*
ccerrstr(u32int cc)
{
	char *s;

	if(cc == 1 || cc == 13)
		return nil;
	if(cc < nelem(ccerrtab) && ccerrtab[cc] != nil)
		s = ccerrtab[cc];
	else
		s = "???";
	return s;
}

static int
waitdone(void *a)
{
	return ((Wait*)a)->z == nil;
}

static char*
ctlrcmd(Ctlr *ctlr, u32int c, u32int s, u64int p, u32int *er);

static char*
waittd(Ctlr *ctlr, Wait *w, int tmout, u32int *er)
{
	Ring *ring = w->ring;

	coherence();
	*ring->doorbell = ring->id;

	while(waserror()){
		if(ring == ctlr->cr)
			ctlr->opr[CRCR] |= CA;
		else
			ctlrcmd(ctlr, CR_STOPEP | (ring->id<<16) | (ring->slot->id<<24), 0, 0, nil);
		tmout = 0;
	}
	if(tmout > 0){
		tsleep(&up->sleep, waitdone, w, tmout);
		if(!waitdone(w))
			error("timed out");
	} else {
		while(!waitdone(w))
			sleep(&up->sleep, waitdone, w);
	}
	poperror();

	if(er != nil)
		memmove(er, w->er, 4*4);
	return ccerrstr(w->er[2]>>24);
}

static char*
ctlrcmd(Ctlr *ctlr, u32int c, u32int s, u64int p, u32int *er)
{
	Wait ws[1];

	ilock(ctlr->cr);
	queuetd(ctlr->cr, c, s, p, ws);
	iunlock(ctlr->cr);

	return waittd(ctlr, ws, 5000, er);
}

static void
completering(Ring *r, u32int *er)
{
	Wait *w, **wp;
	u32int *td, x;
	u64int pa;

	pa = (*(u64int*)er) & ~15ULL;
	ilock(r);

	for(x = r->rp; (int)(r->wp - x) > 0; x++){
		td = &r->base[4*(x++ & r->mask)];
		if((u64int)PADDR(td) == pa){
			r->rp = x;
			break;
		}
	}

	wp = &r->pending;
	while(w = *wp){
		if((u64int)PADDR(w->td) == pa){
			Rendez *z = w->z;

			memmove(w->er, er, 4*4);
			*wp = w->next;
			w->next = nil;

			if(z != nil){
				w->z = nil;
				wakeup(z);
			}
			break;
		} else {
			wp = &w->next;
		}
	}

	iunlock(r);
}

static void
interrupt(Ureg*, void *arg)
{
	Hci *hp = arg;
	Ctlr *ctlr = hp->aux;
	Ring *ring = ctlr->er;
	Slot *slot;
	u32int *irs, *td, x;

	irs = &ctlr->rts[IR0];
	x = irs[IMAN];
	if(x & 1) irs[IMAN] = x & 3;

	for(x = ring->rp;; x=++ring->rp){
		td = ring->base + 4*(x & ring->mask);
		if((((x>>ring->shift)^td[3])&1) == 0)
			break;

		switch(td[3] & 0xFC00){
		case ER_CMDCOMPL:
			completering(ctlr->cr, td);
			break;
		case ER_TRANSFER:
			x = td[3]>>24;
			if(x == 0 || x > ctlr->nslots)
				break;
			slot = ctlr->slot[x];
			if(slot == nil)
				break;
			completering(&slot->epr[(td[3]>>16)-1&31], td);
			break;
		case ER_MFINDEXWRAP:
			ctlr->mfwrap++;
			break;
		case ER_HCE:
			iprint("xhci: host controller error: %ux %ux %ux %ux\n",
				td[0], td[1], td[2], td[3]);
			break;
		case ER_PORTSC:
			break;
		case ER_BWREQ:
		case ER_DOORBELL:
		case ER_DEVNOTE:
		default:
			iprint("xhci: event %ud: %ux %ux %ux %ux\n",
				x, td[0], td[1], td[2], td[3]);
		}
	}

	ctlr->setrptr(&irs[ERDP], PADDR(td) | (1<<3));
}

static void
freeslot(void *arg)
{
	Slot *slot;
	Ctlr *ctlr;

	if(arg == nil)
		return;
	slot = arg;
	ctlr = slot->ctlr;
	if(slot->id != 0){
		qlock(&ctlr->slotlock);
		ctlrcmd(ctlr, CR_DISABLESLOT | (slot->id<<24), 0, 0, nil);
		ctlr->dcba[slot->id] = 0;
		ctlr->slot[slot->id] = nil;
		qunlock(&ctlr->slotlock);
	}
	freering(&slot->epr[0]);
	free(slot->ibase);
	free(slot->obase);
	free(slot);
}

static Slot*
allocslot(Ctlr *ctlr, Udev *dev)
{
	u32int r[4];
	Slot *slot;
	char *err;

	slot = malloc(sizeof(Slot));
	if(slot == nil)
		error(Enomem);

	slot->ctlr = ctlr;
	slot->dev = dev;
	slot->nep = 0;
	slot->id = 0;

	qlock(&ctlr->slotlock);
	if(waserror()){
		qunlock(&ctlr->slotlock);
		freeslot(slot);
		nexterror();
	}
	slot->ibase = mallocalign(32*33 << ctlr->csz, 64, 0, ctlr->pagesize);
	slot->obase = mallocalign(32*32 << ctlr->csz, 64, 0, ctlr->pagesize);
	if(slot->ibase == nil || slot->obase == nil)
		error(Enomem);
	if((err = ctlrcmd(ctlr, CR_ENABLESLOT, 0, 0, r)) != nil)
		error(err);
	slot->id = r[3]>>24;
	if(slot->id <= 0 || slot->id > ctlr->nslots || ctlr->slot[slot->id] != nil){
		slot->id = 0;
		error("bad slot id from controller");
	}
	poperror();

	ctlr->dcba[slot->id] = PADDR(slot->obase);
	ctlr->slot[slot->id] = slot;
	qunlock(&ctlr->slotlock);

	dev->aux = slot;
	dev->free = freeslot;

	return slot;
}

static void
shutdown(Hci *)
{
}

static void
setdebug(Hci *, int)
{
}

static void
epclose(Ep *ep)
{
	Ctlr *ctlr;
	Slot *slot;
	Ring *ring;
	Epio *io;

	if(ep->dev->isroot)
		return;

	io = ep->aux;
	if(io == nil)
		return;
	ep->aux = nil;

	ctlr = ep->hp->aux;
	slot = ep->dev->aux;

	if(ep->nb > 0 && (io[OREAD].ring != nil || io[OWRITE].ring != nil)){
		u32int *w;

		/* input control context */
		w = slot->ibase;
		memset(w, 0, 32<<ctlr->csz);
		w[1] = 1;
		if((ring = io[OREAD].ring) != nil){
			w[0] |= 1 << ring->id;
			if(ring->id == slot->nep)
				slot->nep--;
			ctlrcmd(ctlr, CR_STOPEP | (ring->id<<16) | (slot->id<<24), 0, 0, nil);
		}
		if((ring = io[OWRITE].ring) != nil){
			w[0] |= 1 << ring->id;
			if(ring->id == slot->nep)
				slot->nep--;
			ctlrcmd(ctlr, CR_STOPEP | (ring->id<<16) | (slot->id<<24), 0, 0, nil);
		}

		/* (input) slot context */
		w += 8<<ctlr->csz;
		w[0] = (w[0] & ~(0x1F<<27)) | slot->nep<<27;

		/* (input) ep context */
		w += ep->nb*2*8<<ctlr->csz;
		memset(w, 0, 2*32<<ctlr->csz);

		ctlrcmd(ctlr, CR_CONFIGEP | (slot->id<<24), 0, PADDR(slot->ibase), nil);

		freering(io[OREAD].ring);
		freering(io[OWRITE].ring);
	}
	freeb(io[OREAD].b);
	freeb(io[OWRITE].b);
	free(io);
}

static void
initepctx(u32int *w, Ring *r, Ep *ep)
{
	int ival;

	if(ep->dev->speed == Lowspeed || ep->dev->speed == Fullspeed){
		for(ival=3; ival < 11 && (1<<ival) < ep->pollival; ival++)
			;
	} else {
		for(ival=0; ival < 15 && (1<<ival) < ep->pollival; ival++)
			;
	}
	w[0] = ival<<16;
	w[1] = ((ep->ttype-Tctl)|(r->id&1)<<2)<<3 | (ep->ntds-1)<<8 | ep->maxpkt<<16;
	if(ep->ttype != Tiso)
		w[1] |= 3<<1;
	*((u64int*)&w[2]) = PADDR(r->base) | 1;

	w[4] = ep->maxpkt;
	if(ep->ttype == Tintr || ep->ttype == Tiso)
		w[4] |= (ep->maxpkt*ep->ntds)<<16;
}

static void
initisoio(Epio *io, Ep *ep)
{
	if(io->ring == nil)
		return;
	io->frame = 0;
	io->incr = (ep->hz<<8)/1000;
	io->tdsz = (io->incr+255>>8)*ep->samplesz;
	if(io->tdsz > ep->maxpkt*ep->ntds)
		error(Egreg);
	io->b = allocb((io->ring->mask+1)*io->tdsz);
}

static void
initep(Ep *ep)
{
	Epio *io;
	Ctlr *ctlr;
	Slot *slot;
	Ring *ring;
	u32int *w;
	char *err;

	io = ep->aux;
	ctlr = ep->hp->aux;
	slot = ep->dev->aux;

	io[OREAD].ring = io[OWRITE].ring = nil;
	if(ep->nb == 0){
		io[OWRITE].ring = &slot->epr[0];
		return;
	}

	/* (input) control context */
	w = slot->ibase;
	memset(w, 0, 32<<ctlr->csz);
	w[1] = 1;

	if(waserror()){
		freering(io[OWRITE].ring), io[OWRITE].ring = nil;
		freering(io[OREAD].ring), io[OREAD].ring = nil;
		nexterror();
	}
	if(ep->mode != OREAD){
		ring = initring(io[OWRITE].ring = &slot->epr[ep->nb*2-1], 8);
		ring->id = ep->nb*2;
		if(ring->id > slot->nep)
			slot->nep = ring->id;
		ring->slot = slot;
		ring->doorbell = &ctlr->dba[slot->id];
		ring->ctx = &slot->obase[ring->id*8<<ctlr->csz];
		w[1] |= 1 << ring->id;
	}
	if(ep->mode != OWRITE){
		ring = initring(io[OREAD].ring = &slot->epr[ep->nb*2], 8);
		ring->id = ep->nb*2+1;
		if(ring->id > slot->nep)
			slot->nep = ring->id;
		ring->slot = slot;
		ring->doorbell = &ctlr->dba[slot->id];
		ring->ctx = &slot->obase[ring->id*8<<ctlr->csz];
		w[1] |= 1 << ring->id;
	}

	/* (input) slot context */
	w += 8<<ctlr->csz;
	w[0] = (w[0] & ~(0x1F<<27)) | slot->nep<<27;

	/* (input) ep context */
	w += ep->nb*2*8<<ctlr->csz;
	memset(w, 0, 2*32<<ctlr->csz);
	if(io[OWRITE].ring != nil)
		initepctx(w, io[OWRITE].ring, ep);

	w += 8<<ctlr->csz;
	if(io[OREAD].ring != nil)
		initepctx(w, io[OREAD].ring, ep);

	if((err = ctlrcmd(ctlr, CR_CONFIGEP | (slot->id<<24), 0,
		PADDR(slot->ibase), nil)) != nil){
		error(err);
	}
	if(ep->ttype == Tiso){
		initisoio(io+OWRITE, ep);
		initisoio(io+OREAD, ep);
	}
	poperror();
}

static int
speedid(int speed)
{
	switch(speed){
	case Fullspeed:		return 1;
	case Lowspeed:		return 2;
	case Highspeed:		return 3;
	case Superspeed:	return 4;
	}
	return 0;
}

static void
epopen(Ep *ep)
{
	Ctlr *ctlr = ep->hp->aux;
	Slot *slot, *hub;
	Ring *ring;
	Epio *io;
	Udev *dev;
	char *err;
	u32int *w;
	int i;

	if(ep->dev->isroot)
		return;

	io = malloc(sizeof(Epio)*2);
	if(io == nil)
		error(Enomem);
	ep->aux = io;
	if(waserror()){
		epclose(ep);
		nexterror();
	}
	dev = ep->dev;
	slot = dev->aux;
	if(slot != nil && slot->dev == dev){
		initep(ep);
		poperror();
		return;
	}

	/* first open has to be control endpoint */
	if(ep->nb != 0)
		error(Egreg);

	slot = allocslot(ctlr, dev);

	/* allocate control ep 0 ring */
	ring = initring(io[OWRITE].ring = &slot->epr[0], 4);
	ring->id = 1;
	slot->nep = 1;
	ring->slot = slot;
	ring->doorbell = &ctlr->dba[slot->id];
	ring->ctx = &slot->obase[8];

	/* (input) control context */
	w = slot->ibase;
	memset(w, 0, 3*32<<ctlr->csz);
	w[1] = 3;	/* A0, A1 */
		
	/* (input) slot context */
	w += 8<<ctlr->csz;
	w[2] = w[3] = 0;
	w[0] = dev->routestr | speedid(dev->speed)<<20 |
		(dev->speed == Highspeed && dev->ishub != 0
		||dev->speed < Highspeed && dev->ishub == 0)<<25 |
		(dev->ishub != 0)<<26 | slot->nep<<27;
	w[1] = dev->rootport<<16;

	/* find the parent hub that this device is conected to */
	qlock(&ctlr->slotlock);
	for(i=1; i<=ctlr->nslots; i++){
		hub = ctlr->slot[i];
		if(hub == nil || hub->dev == nil || hub->dev->aux != hub)
			continue;
		if(hub == slot || hub->dev == dev)
			continue;
		if(!hub->dev->ishub)
			continue;
		if(hub->dev->addr != dev->hub)
			continue;
		if(hub->dev->rootport != dev->rootport)
			continue;

		if(dev->speed < Highspeed && hub->dev->speed == Highspeed)
			w[2] = hub->id | dev->port<<8;
		break;
	}
	qunlock(&ctlr->slotlock);

	/* (input) ep context 0 */
	w += 8<<ctlr->csz;
	initepctx(w, io[OWRITE].ring, ep);

	if((err = ctlrcmd(ctlr, CR_ADDRESSDEV | (slot->id<<24), 0,
		PADDR(slot->ibase), nil)) != nil){
		error(err);
	}

	/* (output) slot context */
	w = slot->obase;
	ep->dev->addr = w[3] & 0xFF;
	poperror();
}

static char*
unstall(Ring *r)
{
	u64int qp;
	char *err;

	switch(r->ctx[0]&7){
	case 0:	/* disabled */
	case 1:	/* running */
	case 3:	/* stopped */
		break;
	case 2:	/* halted */
	case 4:	/* error */
		ilock(r);
		r->rp = r->wp;
		qp = PADDR(&r->base[4*(r->wp & r->mask)]) | ((~r->wp>>r->shift) & 1);
		iunlock(r);

		err = ctlrcmd(r->slot->ctlr, CR_RESETEP | (r->id<<16) | (r->slot->id<<24), 0, 0, nil);
		ctlrcmd(r->slot->ctlr, CR_SETTRDQP | (r->id<<16) | (r->slot->id<<24), 0, qp, nil);
		if(err != nil)
			return err;
	}

	if(r->wp - r->rp >= r->mask)
		return "Ring Full";

	return nil;
}

static long
isoread(Ep *, uchar *, long)
{
	error(Egreg);
	return 0;
}

static long
isowrite(Ep *ep, uchar *p, long n)
{
	uchar *s, *d;
	Ctlr *ctlr;
	Epio *io;
	u32int x;
	long m;

	s = p;
	io = (Epio*)ep->aux + OWRITE;
	qlock(io);
	if(waserror()){
		qunlock(io);
		nexterror();
	}

	ctlr = ep->hp->aux;
	for(x = io->frame;; x++){
		for(;;){
			m = (int)(io->ring->wp - io->ring->rp);
			if(m <= 0)
				x = 10 + mfindex(ctlr)/8;
			if(m < io->ring->mask-1)
				break;
			coherence();
			*io->ring->doorbell = io->ring->id;
			tsleep(&up->sleep, return0, nil, 5);
		}
		m = ((io->incr + (x*io->incr&255))>>8)*ep->samplesz;
		d = io->b->rp + (x&io->ring->mask)*io->tdsz;
		m -= io->nleft, d += io->nleft;
		if(n < m){
			memmove(d, p, n);
			p += n;
			io->nleft += n;
			break;
		}
		memmove(d, p, m);
		p += m, n -= m;
		m += io->nleft, d -= io->nleft;
		io->nleft = 0;

		ilock(io->ring);
		queuetd(io->ring, TR_ISOCH | (x & 0x7ff)<<20 | 1<<5, m, PADDR(d), nil);
		iunlock(io->ring);
	}
	io->frame = x;
	coherence();
	*io->ring->doorbell = io->ring->id;
	qunlock(io);
	poperror();

	for(;;){
		int d = (int)(x - mfindex(ctlr)/8);
		d -= ep->sampledelay*1000 / ep->hz;
		if(d < 5)
			break;
		tsleep(&up->sleep, return0, nil, d);
	}

	return p - s;
}

static long
epread(Ep *ep, void *va, long n)
{
	Epio *io;
	uchar *p;
	char *err;
	Wait ws[1];

	p = va;
	if(ep->ttype == Tctl){
		io = (Epio*)ep->aux + OREAD;
		qlock(io);
		if(io->b == nil || BLEN(io->b) == 0){
			qunlock(io);
			return 0;
		}
		if(n > BLEN(io->b))
			n = BLEN(io->b);
		memmove(p, io->b->rp, n);
		io->b->rp += n;
		qunlock(io);
		return n;
	} else if(ep->ttype == Tiso)
		return isoread(ep, p, n);

	if((uintptr)p <= KZERO){
		Block *b;

		b = allocb(n);
		if(waserror()){
			freeb(b);
			nexterror();
		}
		n = epread(ep, b->rp, n);
		memmove(p, b->rp, n);
		freeb(b);
		poperror();
		return n;
	}

	io = (Epio*)ep->aux + OREAD;
	qlock(io);
	if((err = unstall(io->ring)) != nil){
		qunlock(io);
		error(err);
	}
	ilock(io->ring);
	queuetd(io->ring, TR_NORMAL | 1<<16 | 1<<5, n, PADDR(p), ws);
	iunlock(io->ring);
	qunlock(io);

	if((err = waittd((Ctlr*)ep->hp->aux, ws, ep->tmout, nil)) != nil)
		error(err);

	n -= (ws->er[2] & 0xFFFFFF);
	if(n < 0)
		n = 0;

	return n;
}

static long
epwrite(Ep *ep, void *va, long n)
{
	Wait ws[3];
	Epio *io;
	uchar *p;
	char *err;

	p = va;
	if(ep->ttype == Tctl){
		int dir, len;
		Ring *ring;

		if(n < 8)
			error(Eshort);

		if(p[0] == 0x00 && p[1] == 0x05)
			return n;

		io = (Epio*)ep->aux + OREAD;
		qlock(io);
		if(waserror()){
			qunlock(io);
			nexterror();
		}
		if(io->b != nil){
			freeb(io->b);
			io->b = nil;
		}
		len = GET2(&p[6]);
		dir = (p[0] & Rd2h) != 0;
		if(len > 0){
			io->b = allocb(len);		
			if(dir == 0){	/* out */
				assert(len >= n-8);
				memmove(io->b->wp, p+8, n-8);
			} else {
				memset(io->b->wp, 0, len);
				io->b->wp += len;
			}
		}

		ring = io[OWRITE-OREAD].ring;
		if((err = unstall(ring)) != nil)
			error(err);

		if((ring->ctx[1]>>16) != ep->maxpkt){
			Slot *slot = ring->slot;
			Ctlr *ctlr = slot->ctlr;
			u32int *w = slot->ibase;
			w[0] = 0;
			w[1] = 1<<ring->id;
			w += (ring->id+1)*8<<ctlr->csz;
			initepctx(w, ring, ep);
			if((err = ctlrcmd(ctlr, CR_EVALCTX | (slot->id<<24), 0, PADDR(slot->ibase), nil)) != nil)
				error(err);
		}

		ilock(ring);
		queuetd(ring, TR_SETUPSTAGE | (len > 0 ? 2+dir : 0)<<16 | 1<<6 | 1<<5, 8,
			p[0] | p[1]<<8 | GET2(&p[2])<<16 |
			(u64int)(GET2(&p[4]) | len<<16)<<32, &ws[0]);
		if(len > 0)
			queuetd(ring, TR_DATASTAGE | dir<<16 | 1<<5, len,
				PADDR(io->b->rp), &ws[1]);
		queuetd(ring, TR_STATUSSTAGE | (len == 0 || !dir)<<16 | 1<<5, 0, 0, &ws[2]);
		iunlock(ring);

		if((err = waittd((Ctlr*)ep->hp->aux, &ws[0], ep->tmout, nil)) != nil)
			error(err);
		if(len > 0){
			if((err = waittd((Ctlr*)ep->hp->aux, &ws[1], ep->tmout, nil)) != nil)
				error(err);
			if(dir != 0){
				io->b->wp -= (ws[1].er[2] & 0xFFFFFF);
				if(io->b->wp < io->b->rp)
					io->b->wp = io->b->rp;
			}
		}
		if((err = waittd((Ctlr*)ep->hp->aux, &ws[2], ep->tmout, nil)) != nil)
			error(err);
		qunlock(io);
		poperror();

		return n;
	} else if(ep->ttype == Tiso)
		return isowrite(ep, p, n);

	if((uintptr)p <= KZERO){
		Block *b;

		b = allocb(n);
		if(waserror()){
			freeb(b);
			nexterror();
		}
		memmove(b->wp, p, n);
		n = epwrite(ep, b->wp, n);
		freeb(b);
		poperror();
		return n;
	}

	io = (Epio*)ep->aux + OWRITE;
	qlock(io);
	if((err = unstall(io->ring)) != nil){
		qunlock(io);
		error(err);
	}
	ilock(io->ring);
	queuetd(io->ring, TR_NORMAL | 1<<16 | 1<<5, n, PADDR(p), ws);
	iunlock(io->ring);
	qunlock(io);

	if((err = waittd((Ctlr*)ep->hp->aux, ws, ep->tmout, nil)) != nil)
		error(err);

	return n;
}

static char*
seprintep(char *s, char*, Ep*)
{
	return s;
}

static int
portstatus(Hci *hp, int port)
{
	Ctlr *ctlr = hp->aux;
	u32int psc = ctlr->prt[PORTSC + (port-1)*4];
	int ps = 0;

	if(psc & CCS)	ps |= HPpresent;
	if(psc & PED)	ps |= HPenable;
	if(psc & OCA)	ps |= HPovercurrent;
	if(psc & PR)	ps |= HPreset;
	else {
		switch((psc>>10)&0xF){
		case 1:
			/* full speed */
			break;
		case 2:
			ps |= HPslow;
			break;
		case 3:
			ps |= HPhigh;
			break;
		case 4:
			/* super speed */
			break;
		}
	}
	if(psc & PP)	ps |= HPpower;

	if(psc & CSC)	ps |= HPstatuschg;
	if(psc & PRC)	ps |= HPchange;

	return ps;
}
	
static int
portenable(Hci *, int, int)
{
	return 0;
}

static int
portreset(Hci *hp, int port, int on)
{
	Ctlr *ctlr = hp->aux;
	u32int *r = &ctlr->prt[PORTSC + (port-1)*4];

	if(on){
		*r |= PR;
		tsleep(&up->sleep, return0, nil, 200);
	}

	return 0;
}


static Ctlr *ctlrs[Nhcis];

static void
scanpci(void)
{
	static int already = 0;
	int i;
	uintptr io;
	Ctlr *ctlr;
	Pcidev *p;
	u32int *mmio; 

	if(already)
		return;
	already = 1;
	p = nil;
	while ((p = pcimatch(p, 0, 0)) != nil) {
		/*
		 * Find XHCI controllers (Programming Interface = 0x30).
		 */
		if(p->ccrb != Pcibcserial || p->ccru != Pciscusb || p->ccrp != 0x30)
			continue;
		io = p->mem[0].bar & ~0x0f;
		if(io == 0)
			continue;
		mmio = vmap(io, p->mem[0].size);
		if(mmio == nil){
			print("usbxhci: cannot map registers\n");
			continue;
		}
		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil){
			print("usbxhci: no memory\n");
			vunmap(mmio, p->mem[0].size);
			continue;
		}
		print("usbxhci: %#x %#x: port %#p size %#x irq %d\n",
			p->vid, p->did, io, p->mem[0].size, p->intl);
		ctlr->active = nil;
		ctlr->pcidev = p;
		ctlr->mmio = mmio;
		for(i = 0; i < nelem(ctlrs); i++)
			if(ctlrs[i] == nil){
				ctlrs[i] = ctlr;
				break;
			}
		if(i >= nelem(ctlrs))
			print("xhci: bug: more than %d controllers\n", nelem(ctlrs));
	}
}

static int
reset(Hci *hp)
{
	Ctlr *ctlr;
	int i;

	if(getconf("*nousbxhci"))
		return -1;

	scanpci();

	/*
	 * Any adapter matches if no hp->port is supplied,
	 * otherwise the ports must match.
	 */
	for(i = 0; i < nelem(ctlrs) && ctlrs[i] != nil; i++){
		ctlr = ctlrs[i];
		if(ctlr->active == nil)
		if(hp->port == 0 || hp->port == PADDR(ctlr->mmio)){
			ctlr->active = hp;
			goto Found;
		}
	}
	return -1;

Found:
	hp->aux = ctlr;
	hp->port = PADDR(ctlr->mmio);
	hp->irq = ctlr->pcidev->intl;
	hp->tbdf = ctlr->pcidev->tbdf;

	hp->init = init;
	hp->dump = dump;
	hp->interrupt = interrupt;
	hp->epopen = epopen;
	hp->epclose = epclose;
	hp->epread = epread;
	hp->epwrite = epwrite;
	hp->seprintep = seprintep;
	hp->portenable = portenable;
	hp->portreset = portreset;
	hp->portstatus = portstatus;
	hp->shutdown = shutdown;
	hp->debug = setdebug;
	hp->type = "xhci";

	return 0;
}

void
usbxhcilink(void)
{
	addhcitype("xhci", reset);
}
