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
	u32int	*base;

	u32int	size;
	u32int	mask;
	u32int	shift;

	u32int	rp;
	u32int	wp;

	struct {
		u32int	*r;
		u32int	v;
	}	doorbell;

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
	Block	*cb;
	Ring	*ring[2];	/* OREAD, OWRITE */
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

static void
freering(Ring *r)
{
	if(r == nil)
		return;
	if(r->base != nil)
		free(r->base);
	memset(r, 0, sizeof(*r));
}

static void
initring(Ring *r, int shift)
{
	r->doorbell.v = 0;
	r->doorbell.r = nil;
	r->pending = nil;
	r->shift = shift;
	r->size = 1<<shift;
	r->mask = r->size-1;
	r->rp = r->wp = 0;
	r->base = mallocalign(r->size*16, 64, 0, 64*1024);
	if(r->base == nil){
		freering(r);
		error(Enomem);
	}
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
	ctlr->cr->doorbell.r = &ctlr->dba[0];
	ctlr->cr->doorbell.v = 0;
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
		ctlr->erst[i][2] = ctlr->er[i].size;
		ctlr->erst[i][3] = 0;

		irs[ERSTSZ] = 1;	/* just one segment */
		coherence();
		ctlr->setrptr(&irs[ERDP], PADDR(ctlr->er[i].base));
		coherence();
		ctlr->setrptr(&irs[ERSTBA], PADDR(ctlr->erst[i]));

		irs[IMAN] = 3;
		irs[IMOD] = 0;
	}	

	ctlr->opr[USBCMD] = RUNSTOP|INTE|HSEE;
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

static void
kickring(Ring *r)
{
	coherence();
	*r->doorbell.r = r->doorbell.v;
}

static char*
waittd(Wait *w, u32int *er)
{
	while(!waitdone(w)){
		while(waserror())
			;
		kickring(w->ring);
		sleep(&up->sleep, waitdone, w);
		poperror();
	}
	if(er != nil)
		memmove(er, w->er, 4*4);
	return ccerrstr(w->er[2]>>24);
}

static char*
ctlrcmd(Ctlr *ctlr, u32int c, u32int s, u64int p, u32int *er)
{
	Wait w[1];

	ilock(ctlr->cr);
	queuetd(ctlr->cr, c, s, p, w);
	iunlock(ctlr->cr);
	return waittd(w, er);
}

static void
completering(Ring *r, u32int *er)
{
	Wait *w, **wp;
	u32int *td;
	u64int pa;

	pa = (*(u64int*)er) & ~15ULL;
	ilock(r);

	while((int)(r->wp - r->rp) > 0){
		td = &r->base[4*(r->rp++ & r->mask)];
		if((u64int)PADDR(td) == pa)
			break;
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

		if(0) iprint("xhci interrupt: event %ud: %ux %ux %ux %ux\n",
			x, td[0], td[1], td[2], td[3]);

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
		case ER_PORTSC:
		case ER_BWREQ:
		case ER_DOORBELL:
		case ER_HCE:
		case ER_DEVNOTE:
		case ER_MFINDEXWRAP:
		default:
			break;
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
	if(slot->id <= 0 || slot->id > ctlr->nslots){
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
	Epio *io;

	if(ep->dev->isroot)
		return;

	io = ep->aux;
	if(io == nil)
		return;
	ep->aux = nil;

	ctlr = ep->hp->aux;
	slot = ep->dev->aux;

	if(ep->nb > 0 && (io->ring[OREAD] != nil || io->ring[OWRITE] != nil)){
		u32int *w;

		/* input control context */
		w = slot->ibase;
		memset(w, 0, 32<<ctlr->csz);
		w[1] = 1;
		if(io->ring[OREAD] != nil){
			w[0] |= 2 << ep->nb*2;
			if(ep->nb*2+1 == slot->nep)
				slot->nep--;
		}
		if(io->ring[OWRITE] != nil){
			w[0] |= 1 << ep->nb*2;
			if(ep->nb*2 == slot->nep)
				slot->nep--;
		}

		/* (input) slot context */
		w += 8<<ctlr->csz;
		w[0] = (w[0] & ~(0x1F<<27)) | slot->nep<<27;

		/* (input) ep context */
		w += ep->nb*2*8<<ctlr->csz;
		memset(w, 0, 2*32<<ctlr->csz);

		ctlrcmd(ctlr, CR_CONFIGEP | (slot->id<<24), 0, PADDR(slot->ibase), nil);

		freering(io->ring[OREAD]);
		freering(io->ring[OWRITE]);
	}
	freeb(io->cb);
	free(io);
}

static void
initep(Ep *ep)
{
	Epio *io;
	Ctlr *ctlr;
	Slot *slot;
	u32int *w;
	int ival;
	char *err;

	io = ep->aux;
	slot = ep->dev->aux;
	ctlr = ep->hp->aux;

	io->ring[OREAD] = io->ring[OWRITE] = nil;
	if(ep->nb == 0){
		io->ring[OWRITE] = &slot->epr[0];
		return;
	}

	/* (input) control context */
	w = slot->ibase;
	memset(w, 0, 32<<ctlr->csz);
	w[1] = 1;

	if(waserror()){
		freering(io->ring[OWRITE]), io->ring[OWRITE] = nil;
		freering(io->ring[OREAD]), io->ring[OREAD] = nil;
		nexterror();
	}
	if(ep->mode != OREAD){
		initring(io->ring[OWRITE] = &slot->epr[ep->nb*2-1], 8);
		io->ring[OWRITE]->doorbell.r = &ctlr->dba[slot->id];
		io->ring[OWRITE]->doorbell.v = ep->nb*2;

		w[1] |= 1 << ep->nb*2;
		if(ep->nb*2 > slot->nep)
			slot->nep = ep->nb*2;
	}
	if(ep->mode != OWRITE){
		initring(io->ring[OREAD] = &slot->epr[ep->nb*2], 8);
		io->ring[OREAD]->doorbell.r = &ctlr->dba[slot->id];
		io->ring[OREAD]->doorbell.v = ep->nb*2+1;

		w[1] |= 2 << ep->nb*2;
		if(ep->nb*2+1 > slot->nep)
			slot->nep = ep->nb*2+1;
	}

	/* (input) slot context */
	w += 8<<ctlr->csz;
	w[0] = (w[0] & ~(0x1F<<27)) | slot->nep<<27;

	/* (input) ep context */
	w += ep->nb*2*8<<ctlr->csz;
	memset(w, 0, 2*32<<ctlr->csz);

	if(ep->ttype == Tintr && (ep->dev->speed == Fullspeed || ep->dev->speed == Lowspeed)){
		for(ival=3; ival < 11 && (1<<ival) < ep->pollival; ival++)
			;
	} else {
		for(ival=0; ival < 15 && (1<<ival) < ep->pollival; ival++)
			;
	}

	/* out */
	if(io->ring[OWRITE] != nil){
		w[0] = ival<<16;
		w[1] = 3<<1 | ((ep->ttype - Tctl)|0)<<3 | ep->maxpkt<<16;
		*((u64int*)&w[2]) = PADDR(io->ring[OWRITE]->base) | 1;
		w[4] = 1;
	}

	/* in */
	w += 8<<ctlr->csz;
	if(io->ring[OREAD] != nil){
		w[0] = ival<<16;
		w[1] = 3<<1 | ((ep->ttype - Tctl)|4)<<3 | ep->maxpkt<<16;
		*((u64int*)&w[2]) = PADDR(io->ring[OREAD]->base) | 1;
		w[4] = 1;
	}

	if((err = ctlrcmd(ctlr, CR_CONFIGEP | (slot->id<<24), 0,
		PADDR(slot->ibase), nil)) != nil){
		error(err);
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
	Epio *io;
	Udev *dev;
	char *err;
	u32int *w;
	int i;

	if(ep->dev->isroot)
		return;

	io = malloc(sizeof(Epio));
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
	initring(io->ring[OWRITE] = &slot->epr[0], 8);
	io->ring[OWRITE]->doorbell.r = &ctlr->dba[slot->id];
	io->ring[OWRITE]->doorbell.v = 1;
	slot->nep = 1;

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
	w[1] = 3<<1 | 4<<3 | ep->maxpkt<<16;
	*((u64int*)&w[2]) = PADDR(io->ring[OWRITE]->base) | 1;

	if((err = ctlrcmd(ctlr, CR_ADDRESSDEV | (slot->id<<24), 0,
		PADDR(slot->ibase), nil)) != nil){
		error(err);
	}

	/* (output) slot context */
	w = slot->obase;
	ep->dev->addr = w[3] & 0xFF;
	poperror();
}

static void
resetring(Ring *r)
{
	ilock(r);
	r->rp--;	/* assume previous td halted */
	while((int)(r->wp - r->rp) > 0){
		u32int *td = &r->base[4*(--r->wp & r->mask)];
		td[0] = td[1] = td[2] = 0;
		td[3] = (r->wp>>r->shift)&1;
	}
	iunlock(r);
}

static int
epunstall(Ep *ep, int mode)
{
	Ctlr *ctlr;
	Slot *slot;
	Epio *io;
	u32int *w;
	int dci, ret;

	ret = 0;
	io = ep->aux;
	slot = ep->dev->aux;
	ctlr = slot->ctlr;

	if(ep->nb == 0)
		dci = 1;
	else
		dci = ep->nb*2;	

	/* (output) ep context */
	w = slot->obase;
	w += dci*8<<ctlr->csz;
	if(mode != OREAD && io->ring[OWRITE] != nil){
		switch(w[0]&7){
		case 2:
		case 4:
			resetring(io->ring[OWRITE]);
			ctlrcmd(ctlr, CR_RESETEP | (dci<<16) | (slot->id<<24), 0, 0, nil);
			ret++;
		}
	}
	w += 8<<ctlr->csz, dci++;
	if(mode != OWRITE && io->ring[OREAD] != nil){
		switch(w[0]&7){
		case 2:
		case 4:
			resetring(io->ring[OREAD]);
			ctlrcmd(ctlr, CR_RESETEP | (dci<<16) | (slot->id<<24), 0, 0, nil);
			ret++;
		}
	}
	return ret;
}

static long
epread(Ep *ep, void *va, long n)
{
	Epio *io;
	uchar *p;
	Ring *ring;
	char *err;
	Wait ws[1];

	p = va;
	io = ep->aux;
	if(ep->ttype == Tctl){
		qlock(io);
		if(io->cb == nil || BLEN(io->cb) == 0){
			qunlock(io);
			return 0;
		}
		if(n > BLEN(io->cb))
			n = BLEN(io->cb);
		memmove(p, io->cb->rp, n);
		io->cb->rp += n;
		qunlock(io);
		return n;
	}

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

	epunstall(ep, OREAD);
	ring = io->ring[OREAD];
	ilock(ring);
	queuetd(ring, TR_NORMAL | 1<<16 | 1<<5, n, PADDR(p), ws);
	iunlock(ring);

	if((err = waittd(ws, nil)) != nil)
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
	Ring *ring;
	Epio *io;
	uchar *p;
	char *err;

	p = va;
	io = ep->aux;
	if(ep->ttype == Tctl){
		int dir, len;

		if(n < 8)
			error(Eshort);

		if(p[0] == 0x00 && p[1] == 0x05)
			return n;

		qlock(io);
		if(waserror()){
			qunlock(io);
			nexterror();
		}
		if(io->cb != nil){
			freeb(io->cb);
			io->cb = nil;
		}
		len = GET2(&p[6]);
		dir = (p[0] & Rd2h) != 0;
		if(len > 0){
			io->cb = allocb(len);		
			if(dir == 0){	/* out */
				assert(len >= n-8);
				memmove(io->cb->wp, p+8, n-8);
			} else {
				memset(io->cb->wp, 0, len);
				io->cb->wp += len;
			}
		}

		epunstall(ep, OWRITE);
		ring = io->ring[OWRITE];
		ilock(ring);
		queuetd(ring, TR_SETUPSTAGE | (len > 0 ? 2+dir : 0)<<16 | 1<<6 | 1<<5, 8,
			p[0] | p[1]<<8 | GET2(&p[2])<<16 |
			(u64int)(GET2(&p[4]) | len<<16)<<32, &ws[0]);
		if(len > 0)
			queuetd(ring, TR_DATASTAGE | dir<<16 | 0<<6 | 1<<5 | 0<<2, len,
				PADDR(io->cb->rp), &ws[1]);
		queuetd(ring, TR_STATUSSTAGE | (len == 0 || !dir)<<16 | 1<<5, 0, 0, &ws[2]);
		iunlock(ring);

		if((err = waittd(&ws[0], nil)) != nil)
			error(err);
		if(len > 0){
			if((err = waittd(&ws[1], nil)) != nil)
				error(err);
			if(dir != 0){
				io->cb->wp -= (ws[1].er[2] & 0xFFFFFF);
				if(io->cb->wp < io->cb->rp)
					io->cb->wp = io->cb->rp;
			}
		}
		if((err = waittd(&ws[2], nil)) != nil)
			error(err);
		qunlock(io);
		poperror();
		return n;
	}

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

	epunstall(ep, OWRITE);
	ring = io->ring[OWRITE];
	ilock(ring);
	queuetd(ring, TR_NORMAL | 1<<16 | 1<<5, n, PADDR(p), ws);
	iunlock(ring);

	if((err = waittd(ws, nil)) != nil)
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
			p->vid, p->did, mmio, p->mem[0].size, p->intl);
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

	fmtinstall(L'H', encodefmt);

	scanpci();

	/*
	 * Any adapter matches if no hp->port is supplied,
	 * otherwise the ports must match.
	 */
	for(i = 0; i < nelem(ctlrs) && ctlrs[i] != nil; i++){
		ctlr = ctlrs[i];
		if(ctlr->active == nil)
		if(hp->port == 0 || hp->port == (uintptr)ctlr->mmio){
			ctlr->active = hp;
			goto Found;
		}
	}
	return -1;

Found:
	hp->aux = ctlr;
	hp->port = (uintptr)ctlr->mmio;
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
