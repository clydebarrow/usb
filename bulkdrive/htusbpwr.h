/*++

Copyright (c) 2006  HI-TECH Software

Module Name:

    bulkpwr.h

Abstract:

Environment:

    Kernel mode

Notes:

  	Copyright (c) 2006 HI-TECH Software.  
    All Rights Reserved.

--*/

#ifndef _BULKUSB_POWER_H
#define _BULKUSB_POWER_H

typedef struct _POWER_COMPLETION_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    PIRP           SIrp;
} POWER_COMPLETION_CONTEXT, *PPOWER_COMPLETION_CONTEXT;

typedef struct _WORKER_THREAD_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    PIRP           Irp;
    PIO_WORKITEM   WorkItem;
} WORKER_THREAD_CONTEXT, *PWORKER_THREAD_CONTEXT;

NTSTATUS
BulkUsb_DispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
HandleSystemQueryPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
HandleSystemSetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
HandleDeviceQueryPower(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    );

NTSTATUS
SysPoCompletionRoutine(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp,
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
SendDeviceIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
DevPoCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject, 
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
HandleDeviceSetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
FinishDevPoUpIrp(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp,
    IN PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
SetDeviceFunctional(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp,
    IN PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
FinishDevPoDnIrp(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp,
    IN PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
HoldIoRequests(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
HoldIoRequestsWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          Context
    );

#endif
