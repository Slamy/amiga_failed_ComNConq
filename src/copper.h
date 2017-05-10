
//Registers for Copper usage

#define INTREQ	0x09C
#define BPL1PTH 0x0E0
#define BPL1PTL 0x0E2
#define BPL2PTH 0x0E4
#define BPL2PTL 0x0E6
#define BPL3PTH 0x0E8
#define BPL3PTL 0x0EA
#define BPL4PTH 0x0EC
#define BPL4PTL 0x0EE
#define BPL5PTH 0x0F0
#define BPL5PTL 0x0F2

#define SPR0POS 0x140
#define SPR0CTL 0x142

#define SPR0PTH 0x120
#define SPR1PTH 0x124
#define SPR2PTH 0x128
#define SPR3PTH 0x12C
#define SPR4PTH 0x130
#define SPR5PTH 0x134
#define SPR6PTH 0x138
#define SPR7PTH 0x13C


#define COPPER_WRITE_32(ADR,VAL) \
	copperlist[i++]=ADR; \
	copperlist[i++]=((uint32_t)VAL)>>16; \
	copperlist[i++]=ADR+2; \
	copperlist[i++]=((uint32_t)VAL);

#define COPPER_WRITE_16(ADR,VAL) \
	copperlist[i++]=ADR; \
	copperlist[i++]=((uint32_t)VAL);


#define COPPER_WAIT_VERTICAL(LINE) \
	copperlist[i++]=0x0001 | (LINE)<<8; \
	copperlist[i++]=0xFF00;


#define COPPER_WAIT_VERTICAL_HORIZONTAL(ROW, LINE) \
	copperlist[i++]=0x0001 | (LINE)<<8 | ROW; \
	copperlist[i++]=0xFFFE;
