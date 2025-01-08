/*++

Copyright (c) 2006  HI-TECH Software

Module Name:

    bulkdev.h

Abstract:

Environment:

    Kernel mode

Notes:

    Copyright (c) 2006 HI-TECH Software.  
    All Rights Reserved.

--*/

#ifndef _BULKUSB_DEV_H
#define _BULKUSB_DEV_H

NTSTATUS
BulkUsb_DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
BulkUsb_DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
BulkUsb_DispatchDevCtrl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
BulkUsb_ResetPipe(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PUSBD_PIPE_INFORMATION PipeInfo
    );

NTSTATUS
BulkUsb_ResetDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
BulkUsb_GetPortStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG PortStatus
    );

NTSTATUS
BulkUsb_ResetParentPort(
    IN IN PDEVICE_OBJECT DeviceObject
    );

VOID
WWIrpCompletionFunc(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

#endif
