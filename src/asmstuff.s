
	INCLUDE "exec/types.i"
	INCLUDE "hardware/custom.i"
	INCLUDE "hardware/intbits.i"

	XDEF    _isr_verticalBlank
	XDEF    _supervisor_enableInterrupts
	XDEF    _supervisor_disableInterrupts
	XDEF	_getSR
	XDEF	_tilemap
	XDEF	_protrackerModule_alien
	XDEF 	_testSoundEffect
	XREF	_custom
	XREF	_vBlankCounter
	XREF	_mouseCursor
	XREF	_supervisor_disableDataCache
	XREF	_supervisor_getCACR
	XREF	_testbob

    section code_c

_supervisor_enableInterrupts:
	ANDI.W   #$F0FF,SR  ;Mask off old IPL bits
	rte

_supervisor_disableInterrupts:
	ORI.W   #$0700,SR   ;Put in new IPL bits
	rte


_getSR:
	move.w SR,d0
	rte

_supervisor_getCACR:
	movec cacr,d0
	rte

_supervisor_disableDataCache:
	movec cacr,d0
	bset #0,d0 ; i cache on
	bset #4,d0 ; i burst on
	bclr #8,d0 ; d cache off
	bclr #12,d0 ; d burst off
	movec d0,cacr
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

_mouseCursor:
	incbin "../assets/mousecursor.bin"

_protrackerModule_alien:
	incbin "alien.mod"

_testSoundEffect:
	ds.w 0
	incbin "ironchg1.raw"

_testbob:
	incbin "../assets/testbob.bin"

    END


