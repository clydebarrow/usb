
	; HI-TECH C COMPILER (Cypress PSOC) V9.60PL5
	; Copyright (C) 1984-2007 HI-TECH Software
	;Serial no. HCPSOC-00000
	;Licensed for FAE use only - not for an end-user.

	; Auto-generated runtime startup code for final link stage.

	;
	; Compiler options:
	;
	; -q --chip=CY8C24094 --ROM=default,-000-FFF -DUSER_START=0x1000 -g \
	; --asmlist -I. -ULOGGING -Dxdata= -DDEBUG=1 -opsoctest.hex \
	; -mpsoctest.map psoctest.p1 main.p1 lcd.p1 bootlib.as \
	; flashsecurity.txt
	;


	processor	CY8C24094
	macro	M8C_ClearWDT
	mov reg[0xE3],0x38
	endm

	psect	PD_startup,class=CODE
	psect	init,class=CODE
	psect	end_init,class=CODE
	psect	powerup,class=CODE
	psect	vectors,ovrld,class=CODE
	psect	text,class=CODE
	psect	maintext,class=CODE
	psect	intrtext,class=CODE
	psect	fnauto,class=RAM,space=1
	psect	bss,class=RAM,space=1
	psect	InterruptRAM,class=RAM,space=1
	psect	cdata,class=ROM,space=0,reloc=256
	psect	psoc_config,class=ROM
	psect	UserModules,class=ROM
	psect	strings,class=ROM
	psect	stackps,class=RAM
	global	__Lstackps, __stack_start__
__stack_start__:
	psect	bss0,class=RAM,space=1
	psect	nvram0,class=RAM,space=1
	psect	rbit0,bit,class=RAM,space=1
	psect	nvbit0,bit,class=RAM,space=1
	psect	ramdata0,class=RAM,space=1
	psect	romdata0,class=BANKROM,space=0
	psect	bss1,class=RAM,space=1
	psect	nvram1,class=RAM,space=1
	psect	rbit1,bit,class=RAM,space=1
	psect	nvbit1,bit,class=RAM,space=1
	psect	ramdata1,class=RAM,space=1
	psect	romdata1,class=BANKROM,space=0
	psect	bss2,class=RAM,space=1
	psect	nvram2,class=RAM,space=1
	psect	rbit2,bit,class=RAM,space=1
	psect	nvbit2,bit,class=RAM,space=1
	psect	ramdata2,class=RAM,space=1
	psect	romdata2,class=BANKROM,space=0
	psect	bss3,class=RAM,space=1
	psect	nvram3,class=RAM,space=1
	psect	rbit3,bit,class=RAM,space=1
	psect	nvbit3,bit,class=RAM,space=1
	psect	ramdata3,class=RAM,space=1
	psect	romdata3,class=BANKROM,space=0

	global	start,startup,_main
	global	reset_vec,intlevel0,intlevel1,intlevel2
intlevel0:
intlevel1:
intlevel2:		; for C funcs called from assembler

	fnconf	fnauto,??,?
	fnroot	_main
TMP_DR0	equ	108
TMP_DR1	equ	109
TMP_DR2	equ	110
TMP_DR3	equ	111
CUR_PP	equ	208
STK_PP	equ	209
IDX_PP	equ	211
MVR_PP	equ	212
MVW_PP	equ	213
CPU_F	equ	247
	psect	vectors
reset_vec:
start:
	ljmp	startup

	psect	init
startup:
	M8C_ClearWDT
	or	f, 0x80	;select multiple RAM page mode
	and	f, 0xBF

;	Clear uninitialized variables in bank 0
	global	__Lbss0
	mov	a,low __Lbss0
	swap	a,sp
	mov	a,0
	mov	x,99
bssloop0:
	push	a
	dec	x
	jnz	bssloop0
	mov	reg[STK_PP],3
	mov	a,low __Lstackps
	swap	a,sp

	ljmp	_main

	end	start
