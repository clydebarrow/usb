/*++

Copyright (c) 2006  HI-TECH Software

Module Name:

    sSUsr.h

Abstract:

Environment:

    Kernel mode

Notes:

    Copyright (c) 2006 HI-TECH Software.  
    All Rights Reserved.

--*/

#ifndef _BULKUSB_USER_H
#define _BULKUSB_USER_H

#include <initguid.h>

// {37ABC9BB-51FE-47bd-BACD-A974BC4111E8}
DEFINE_GUID(GUID_INTERFACE_HTSOFT, 
0x37abc9bb, 0x51fe, 0x47bd, 0xba, 0xcd, 0xa9, 0x74, 0xbc, 0x41, 0x11, 0xe8);

#define BULKUSB_IOCTL_INDEX             0x0000


#define IOCTL_BULKUSB_GET_CONFIG_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     BULKUSB_IOCTL_INDEX,     \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)
                                                   
#define IOCTL_BULKUSB_RESET_DEVICE          CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     BULKUSB_IOCTL_INDEX + 1, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_BULKUSB_RESET_PIPE            CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     BULKUSB_IOCTL_INDEX + 2, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#endif

