
	INCLUDE "exec/types.i"
	INCLUDE "hardware/custom.i"
	INCLUDE "hardware/intbits.i"

	XDEF    _isr_verticalBlank
	XDEF    _supervisor_enableInterrupts
	XDEF    _supervisor_disableInterrupts
	XDEF	_getSR
	XREF	_custom
	XREF	_vBlankCounter

    section .data_chip

_supervisor_enableInterrupts:
	ANDI.W   #$F0FF,SR  ;Mask off old IPL bits
	rte

_supervisor_disableInterrupts:
	ORI.W   #$0700,SR   ;Put in new IPL bits
	rte


_getSR:
	move.w SR,d0
	rte

_isr_verticalBlank:
	;Preserve used registers
	MOVE.W  SR, -(SP)
	move.l	a6,-(sp)

	lea.l	_custom,a6
	move.w	#INTF_COPER, intreq(a6)		;Interrupt-Request abschalten

	addi.w	#1,_vBlankCounter
	
	;Restore used registers
	move.l	(sp)+,a6
	MOVE.W  (SP)+, SR
	
	rte

    END


