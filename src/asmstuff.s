
	INCLUDE "exec/types.i"
	INCLUDE "hardware/custom.i"
	INCLUDE "hardware/intbits.i"

	XDEF    _isr_verticalBlank
	XDEF    _supervisor_enableInterrupts
	XDEF    _supervisor_disableInterrupts
	XDEF	_getSR
	XDEF	_tilemap
	XDEF	_protrackerModule_alien
	XREF	_custom
	XREF	_vBlankCounter

    section code


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
	move.l	a6,-(sp)
	lea.l	_custom,a6
	move.w	#INTF_COPER, intreq(a6)		;Interrupt-Request abschalten

	addi.w	#1,_vBlankCounter
	
	move.l	(sp)+,d6
	
	rte

_tilemap:
	incbin "../assets/tilemap.bin"

_protrackerModule_alien:
	incbin "alien.mod"

    END


