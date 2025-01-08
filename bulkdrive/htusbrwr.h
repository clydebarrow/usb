/*++

Copyright (c) 2006  HI-TECH Software

Module Name:

    bulkrwr.h

Abstract:

Environment:

    Kernel mode

Notes:

    Copyright (c) 2006 HI-TECH Software.  
    All Rights Reserved.

--*/
#ifndef _BULKUSB_RWR_H
#define _BULKUSB_RWR_H

typedef enum {

	IRPLOCK_NULL,
	IRPLOCK_CANCELABLE,
	IRPLOCK_CANCEL_STARTED,
	IRPLOCK_CANCEL_COMPLETE,
	IRPLOCK_COMPLETED

} IRPLOCK;

typedef struct _BULKUSB_RW_CONTEXT {

    PURB              Urb;
    PMDL              Mdl;
    ULONG             Length;         // remaining to xfer
    ULONG             Numxfer;        // cumulate xfer
    ULONG_PTR         VirtualAddress; // va for next segment of xfer.
	KEVENT				Event;			// event to signal on completion
	LONG				Lock;		// lock for cancellation processing
    PDEVICE_EXTENSION DeviceExtension;

} BULKUSB_RW_CONTEXT, * PBULKUSB_RW_CONTEXT;

NTSTATUS
BulkUsb_PipeWithName(
    IN PUNICODE_STRING FileName,
	OUT	PBULKUSB_PIPEINFO Pipeinfo
    );

NTSTATUS
BulkUsb_DispatchReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
BulkUsb_ReadWriteCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

#endif
