;	Jump table to USB routines - interleave them with the interrupt vectors

	global	_USB_init
	global	_USB_detach
	global	_USB_control
	global	_USB_read_packet
	global	_USB_send_bytes
	global	_bl_main


_USB_init		equ	0x70
_USB_detach		equ	0x74
_USB_control		equ	0x78
_USB_read_packet	equ	0x7c
_USB_send_bytes		equ	0x80
_bl_main		equ	0x84

global			_USB_status

_USB_status			equ	0x3F7


