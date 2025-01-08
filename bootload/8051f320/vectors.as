;	Reassign vectors to the user program space at USER_START and above
;	with the exception of the USB interrupt vector.


	psect	vectors,global,ovrld,class=CODE
	org		0x03
	ljmp	USER_START+0x03
	org		0x0B
	ljmp	USER_START+0x0B
	org		0x13
	ljmp	USER_START+0x13
	org		0x1B
	ljmp	USER_START+0x1B
	org		0x23
	ljmp	USER_START+0x23
	org		0x2B
	ljmp	USER_START+0x2B
	org		0x33
	ljmp	USER_START+0x33
	org		0x3B
	ljmp	USER_START+0x3B

	;	USB vector (0x43) claimed by bootloader

	org		0x4B
	ljmp	USER_START+0x4B
	org		0x53
	ljmp	USER_START+0x53
	org		0x5B
	ljmp	USER_START+0x5B
	org		0x63
	ljmp	USER_START+0x63
	org		0x6B
	ljmp	USER_START+0x6B
	org		0x73
	ljmp	USER_START+0x73
	org		0x7B
	ljmp	USER_START+0x7B


;	Jump table to USB routines - interleave them with the interrupt vectors

	global	_USB_init
	global	_USB_detach
	global	_USB_control
	global	_USB_read_packet
	global	_USB_flushout
	global	_USB_flushin
	global	_USB_send_byte
	global	_USB_halt


	org		0x06
	ljmp	_USB_init
	org		0x0E
	ljmp	_USB_detach
	org		0x16
	ljmp	_USB_control
	org		0x1E
	ljmp	_USB_read_packet
	org		0x26
	ljmp	_USB_flushout
	org		0x2E
	ljmp	_USB_flushin
	org		0x36
	ljmp	_USB_send_byte
	org		0x3E
	ljmp	_USB_halt
