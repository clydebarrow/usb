;	Reassign vectors to the user program space at USER_START and above
;	with the exception of the USB interrupt vector.

redirect	macro	where
	org	where
	ljmp	USER_START+where
	endm
	
entrypoint	macro	where,to
	global	to
	org		where
	ljmp	to
	endm
	

	psect	vectors,global,ovrld,class=CODE
	redirect	0x04
	redirect	0x08
	redirect	0x0C
	redirect	0x10
	redirect	0x14
	redirect	0x18
	redirect	0x1C
	redirect	0x20
	redirect	0x24
	redirect	0x28
	redirect	0x2C
	redirect	0x30
	redirect	0x34
	redirect	0x38
	redirect	0x3C
	redirect	0x60
	redirect	0x64

;	Jump table to USB routines - interleave them with the interrupt vectors

	global	_USB_init
	global	_USB_detach
	global	_USB_control
	global	_USB_read_packet
	global	_USB_send_bytes
	global 	_bl_main
	
	org	0x70
	ljmp	_USB_init
	
	org	0x74
	ljmp	_USB_detach
	
	org	0x78
	ljmp	_USB_control
	
	org	0x7C
	ljmp	_USB_read_packet
	
	
	org	0x80
	ljmp	_USB_send_bytes
	
	
	org	0x84
	ljmp	_bl_main
	
	
	
	
	
	
	




