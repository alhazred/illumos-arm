/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2017 Hayashi Naoyuki
 */

#include <sys/types.h>
#include <sys/hwrpb.h>

typedef uint64_t vaddr_t;

#define BUFMAX 4096

/*
 * Number of bytes for registers
 */
#define NUMREGBYTES 67*8

#define ALPHA_NOP 0x47ff041f
#define ALPHA_BPT 0x00000080

uint64_t gdb_regs[68];

#define VPTB (hwrpb->vptb)
#define SEG0_END (0x0000040000000000ul)
#define KSEG (0xfffffc0000000000ul)
#define SEG1 (0xfffffe0000000000ul)

static boolean_t isWritable(vaddr_t mem)
{
	if (mem < SEG0_END || SEG1 <= mem) {
		uint64_t l1idx = (mem >> 33) & 0x3ff;
		uint64_t l2idx = (mem >> 23) & 0x3ff;
		uint64_t l3idx = (mem >> 13) & 0x3ff;
		uint64_t idx = (VPTB >> (13 + 10 + 10)) & 0x3ff;
		uint64_t *ptbl;

		ptbl = (uint64_t *)
		    (VPTB | (idx << 33) | (idx << 23) | (idx << 13));
		if (!(ptbl[l1idx] & 1))	// valid
			return B_FALSE;

		ptbl = (uint64_t *)(KSEG | ((ptbl[l1idx] >> 32) << 13));
		if (!(ptbl[l2idx] & 1)) // valid
			return B_FALSE;

		ptbl = (uint64_t *)(KSEG | ((ptbl[l2idx] >> 32) << 13));
		if ((ptbl[l3idx] & 0x1101) != 0x1101) // valid, read, write
			return B_FALSE;

	} else if (mem < KSEG) {
		return B_FALSE;
	}

	return B_TRUE;
}

static boolean_t isReadable(vaddr_t mem)
{
	if (mem < SEG0_END || SEG1 <= mem) {
		uint64_t l1idx = (mem >> 33) & 0x3ff;
		uint64_t l2idx = (mem >> 23) & 0x3ff;
		uint64_t l3idx = (mem >> 13) & 0x3ff;
		uint64_t idx = (VPTB >> (13 + 10 + 10)) & 0x3ff;
		uint64_t *ptbl;

		ptbl = (uint64_t *)
		    (VPTB | (idx << 33) | (idx << 23) | (idx << 13));
		if (!(ptbl[l1idx] & 1))	// valid
			return B_FALSE;

		ptbl = (uint64_t *)(KSEG | ((ptbl[l1idx] >> 32) << 13));
		if (!(ptbl[l2idx] & 1)) // valid
			return B_FALSE;

		ptbl = (uint64_t *)(KSEG | ((ptbl[l2idx] >> 32) << 13));
		if ((ptbl[l3idx] & 0x101) != 0x101) // valid, read
			return B_FALSE;

	} else if (mem < KSEG) {
		return B_FALSE;
	}

	return B_TRUE;
}
/*
 * Forward declarations
 */

static int hex(unsigned char);
static int mem2hex(unsigned char *, unsigned char *, int);
static int hex2mem(unsigned char *, unsigned char *, int);
static int hexToInt(unsigned char **, uintptr_t *);
static unsigned char *getpacket(void);
static void putpacket(unsigned char *);
static int computeSignal(uint64_t);
void handle_exception(uint64_t);
static void init_serial(void);
static void putDebugChar(unsigned char);
static unsigned char getDebugChar(void);

/* debug > 0 prints ill-formed commands in valid packets & checksum errors */
static int remote_debug;

enum regnames {
	PC = 64
};

static struct {
	uint32_t *memAddr;
	uint32_t oldInstr;
} instrBuffer;

static int stepped;
static const char hexchars[] = "0123456789abcdef";
static unsigned char remcomInBuffer[BUFMAX] = {0};
static unsigned char remcomOutBuffer[BUFMAX] = {0};

static inline void
rom_strcpy(unsigned char *dst, const char *src)
{
	while ((*dst++ = *src++) != 0);
}

/*
 * Routines to handle hex data
 */
static inline int
hex(unsigned char ch)
{
	if (ch >= 'a' && ch <= 'f')
		return ch-'a'+10;
	if (ch >= '0' && ch <= '9')
		return ch-'0';
	if (ch >= 'A' && ch <= 'F')
		return ch-'A'+10;
	return -1;
}

/* convert the memory, pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
static inline int
mem2hex(unsigned char *mem, unsigned char *buf, int count)
{
	unsigned char ch;

	while (count-- > 0) {
		if (!isReadable((vaddr_t)mem))
			return -1;
		ch = *mem++;
		*buf++ = hexchars[(ch >> 4) & 0xf];
		*buf++ = hexchars[ch & 0xf];
	}

	*buf = 0;
	return 0;
}

/* convert the hex array pointed to by buf into binary, to be placed in mem */
/* return a pointer to the character after the last byte written */
static inline int
hex2mem(unsigned char *buf, unsigned char *mem, int count)
{
	int i;
	unsigned char ch;

	for (i = 0; i < count; i++) {
		ch = hex(*buf++) << 4;
		ch |= hex(*buf++);
		if (!isWritable((vaddr_t)mem))
			return -1;
		*mem++ = ch;
	}
	return 0;
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
static inline int
hexToInt(unsigned char **ptr, uintptr_t *intValue)
{
	int numChars = 0;
	int hexValue;

	*intValue = 0;

	while (**ptr) {
		hexValue = hex(**ptr);
		if (hexValue < 0)
			break;

		*intValue = (*intValue << 4) | hexValue;
		numChars ++;

		(*ptr)++;
	}

	return (numChars);
}


/*
 * Routines to get and put packets
 */

/* scan for the sequence $<data>#<checksum>     */

static unsigned char *
getpacket(void)
{
	unsigned char *buffer = &remcomInBuffer[0];
	unsigned char checksum;
	unsigned char xmitcsum;
	int count;
	unsigned char ch;

	for (;;) {
		/* wait around for the start character, ignore all other characters */
		while ((ch = getDebugChar()) != '$');

retry:
		checksum = 0;
		xmitcsum = -1;
		count = 0;

		/* now, read until a # or end of buffer is found */
		while (count < BUFMAX - 1) {
			ch = getDebugChar();
			if (ch == '$')
				goto retry;
			if (ch == '#')
				break;
			checksum = checksum + ch;
			buffer[count] = ch;
			count = count + 1;
		}
		buffer[count] = 0;

		if (ch == '#') {
			ch = getDebugChar();
			xmitcsum = hex(ch) << 4;
			ch = getDebugChar();
			xmitcsum += hex(ch);

			if (checksum != xmitcsum) {
				/* failed checksum */
				putDebugChar('-');
			} else {
				/* successful transfer */
				putDebugChar('+');

				/* if a sequence char is present, reply the sequence ID */
				if (buffer[2] == ':') {
					putDebugChar(buffer[0]);
					putDebugChar(buffer[1]);
					return &buffer[3];
				}
				return &buffer[0];
			}
		}
	}
}

/* send the packet in buffer.  */
static void
putpacket(unsigned char *buffer)
{
	unsigned char checksum;
	int count;
	unsigned char ch;

	/*  $<packet info>#<checksum>. */
	do {
		putDebugChar('$');
		checksum = 0;
		count = 0;

		while ((ch = buffer[count]) != 0) {
			putDebugChar(ch);
			checksum += ch;
			count += 1;
		}

		putDebugChar('#');
		putDebugChar(hexchars[(checksum >> 4) & 0xf]);
		putDebugChar(hexchars[checksum & 0xf]);

	} while (getDebugChar() != '+');
}



/*
 * this function takes the SH-1 exception number and attempts to
 * translate this number into a unix compatible signal value
 */
static int
computeSignal(uint64_t exceptionVector)
{
	return exceptionVector;
}

static uint64_t
next_pc(void)
{
	unsigned int insn;
	unsigned int op;
	int regno;
	int offset;
	int64_t rav;

	insn = *(uint32_t *)(gdb_regs[PC]);

	/* Opcode is top 6 bits. */
	op = (insn >> 26) & 0x3f;

	if (op == 0x1a) {
		/* Jump format: target PC is:
		   RB & ~3  */
		return gdb_regs[((insn >> 16) & 0x1f)] & ~3;
	}

	if ((op & 0x30) == 0x30) {
		if (op == 0x30		/* BR */
		    || op == 0x34) {	/* BSR */
			goto branch_taken;
		}

		/* Need to determine if branch is taken; read RA.  */
		regno = (insn >> 21) & 0x1f;

		rav = (int64_t)gdb_regs[regno];

		switch (op) {
		case 0x38:		/* BLBC */
			if ((rav & 1) == 0)
				goto branch_taken;
			break;
		case 0x3c:		/* BLBS */
			if (rav & 1)
				goto branch_taken;
			break;
		case 0x39:		/* BEQ */
			if (rav == 0)
				goto branch_taken;
			break;
		case 0x3d:		/* BNE */
			if (rav != 0)
				goto branch_taken;
			break;
		case 0x3a:		/* BLT */
			if (rav < 0)
				goto branch_taken;
			break;
		case 0x3b:		/* BLE */
			if (rav <= 0)
				goto branch_taken;
			break;
		case 0x3f:		/* BGT */
			if (rav > 0)
				goto branch_taken;
			break;
		case 0x3e:		/* BGE */
			if (rav >= 0)
				goto branch_taken;
			break;
		}
	}

	/* Not a branch or branch not taken; target PC is:
	   pc + 4  */
	return (gdb_regs[PC] + 4);
branch_taken:
	/* Branch format: target PC is:
	   (new PC) + (4 * sext(displacement))  */
	offset = (insn & 0x001fffff);
	if (offset & 0x00100000)
		offset  |= 0xffe00000;
	offset *= 4;
	return (gdb_regs[PC] + 4 + offset);
}


static inline void
doSStep(void)
{
	uint32_t *instrMem = (uint32_t *)next_pc();

	stepped = 1;

	instrBuffer.memAddr = instrMem;
	instrBuffer.oldInstr = *instrMem;
	*instrMem = ALPHA_BPT;
}


/* Undo the effect of a previous doSStep.  If we single stepped,
   restore the old instruction. */
static inline void
undoSStep(void)
{
	if (stepped) {
		*instrBuffer.memAddr = instrBuffer.oldInstr;
	}
	stepped = 0;
}

/*
   This function does all exception handling.  It only does two things -
   it figures out why it was called and tells gdb, and then it reacts
   to gdb's requests.

   When in the monitor mode we talk a human on the serial line rather than gdb.
*/
static void
gdb_handle_exception(uint64_t exceptionVector)
{
	int sigval;
	uintptr_t addr, length;
	unsigned char *ptr;

	/* reply to host that an exception has occurred */
	sigval = computeSignal(exceptionVector);
	remcomOutBuffer[0] = 'S';
	remcomOutBuffer[1] = hexchars[(sigval >> 4) & 0xf];
	remcomOutBuffer[2] = hexchars[sigval & 0xf];
	remcomOutBuffer[3] = 0;
	putpacket(remcomOutBuffer);

	/*
	 * Do the thangs needed to undo
	 * any stepping we may have done!
	 */
	undoSStep();

	for (;;) {
		remcomOutBuffer[0] = 0;
		ptr = getpacket();

		switch (*ptr++) {
		case '?':
			remcomOutBuffer[0] = 'S';
			remcomOutBuffer[1] = hexchars[(sigval >> 4) & 0xf];
			remcomOutBuffer[2] = hexchars[sigval & 0xf];
			remcomOutBuffer[3] = 0;
			break;
		case 'd':
			remote_debug = !(remote_debug);	/* toggle debug flag */
			break;
		case 'g':
			/* return the value of the CPU registers */
			mem2hex((unsigned char *)gdb_regs, remcomOutBuffer, NUMREGBYTES);
			break;
		case 'G':		/* set the value of the CPU registers - return OK */
			hex2mem(ptr, (unsigned char *)gdb_regs, NUMREGBYTES);
			rom_strcpy(remcomOutBuffer, "OK");
			break;

		case 'm':
			/* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
			/* TRY, TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
			if (hexToInt(&ptr, &addr))
				if (*(ptr++) == ',')
					if (hexToInt(&ptr, &length)) {
						ptr = 0;
						if(mem2hex((unsigned char *)addr, remcomOutBuffer, length)<0)
							rom_strcpy(remcomOutBuffer, "E03");
					}
			if (ptr)
				rom_strcpy(remcomOutBuffer, "E01");

			/* restore handler for bus error */
			break;

		case 'M':
			/* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */

			/* TRY, TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
			if (hexToInt(&ptr, &addr))
				if (*(ptr++) == ',')
					if (hexToInt(&ptr, &length))
						if (*(ptr++) == ':') {
							if (hex2mem(ptr, (unsigned char *) addr, length) < 0)
								rom_strcpy(remcomOutBuffer, "E03");
							else
								rom_strcpy(remcomOutBuffer, "OK");
							ptr = 0;
						}
			if (ptr)
				rom_strcpy(remcomOutBuffer, "E02");

			/* restore handler for bus error */
			break;

		case 's':
			/* sAA..AA   Step one instruction from AA..AA(optional) */
			doSStep();
			return;
			break;
		case 'c':
			/* cAA..AA    Continue at address AA..AA(optional) */
			/* tRY, to read optional parameter, pc unchanged if no parm */
			if (hexToInt(&ptr, &addr))
				gdb_regs[PC] = addr;
			return;
			break;

		case 'k':
			/* kill the program */
			/* do nothing */
			break;
		}			/* switch */

		/* reply to the request */
		putpacket(remcomOutBuffer);
	}
}


void handle_exception(uint64_t exceptionVector)
{
	gdb_handle_exception(exceptionVector);
}

void
INIT(void)
{
	extern void dbg_start(void);
	stepped = 1;
	instrBuffer.memAddr = (uint32_t *)dbg_start;
	instrBuffer.oldInstr = ALPHA_NOP;
	init_serial();
}

struct com_reg {
	union {
		volatile unsigned char data;
		volatile unsigned char dlbl;
	} u0;
	union {
		volatile unsigned char ier;
		volatile unsigned char dlbh;
	} u1;
	union {
		volatile unsigned char iir;
		volatile unsigned char fcr;
	} u2;
	volatile unsigned char lcr;
	volatile unsigned char mcr;
	volatile unsigned char lsr;
	volatile unsigned char msr;
};

static struct com_reg *com1 = (struct com_reg *)0xFFFFFC89000003F8UL;
static struct com_reg *com2 = (struct com_reg *)0xFFFFFC89000002F8UL;

static void
init_serial(void)
{
	unsigned int baud = 115200;
	unsigned int freq = 1843200;
	unsigned int divisor = freq / (baud * 16);

	com2->lcr = 0x80;
	__asm __volatile__("mb":::"memory");
	com2->u0.dlbl = divisor & 0xff;
	com2->u1.dlbh = (divisor>>8) & 0xff;
	__asm __volatile__("mb":::"memory");
	com2->lcr = 0x03;
	com2->u1.ier = 0x00;
	com2->u2.fcr = 0x00;
	com2->mcr = com1->mcr;
	__asm __volatile__("mb":::"memory");
}

static inline void
getDebugCharWait(void)
{
	while ((com2->lsr & 0x01) == 0)
		__asm __volatile__("mb":::"memory");
	__asm __volatile__("mb":::"memory");
}

static unsigned char
getDebugChar(void)
{
	volatile unsigned char ch;

	getDebugCharWait();

	ch = com2->u0.data;
	__asm __volatile__("mb":::"memory");
	com2->lsr;
	__asm __volatile__("mb":::"memory");

	return ch;
}

static inline void
putDebugCharReady(void)
{
	while ((com2->lsr & 0x20) == 0)
		__asm __volatile__("mb":::"memory");
	__asm __volatile__("mb":::"memory");
}

static void
putDebugChar(unsigned char ch)
{
	putDebugCharReady();

	com2->u0.data = ch;
	__asm __volatile__("mb":::"memory");
}

