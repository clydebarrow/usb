/*++

Copyright (c) 2006  HI-TECH Software

Module Name:

    bulkrwr.c

Abstract:

    This file has routines to perform reads and writes.
    The read and writes are for bulk transfers.

Environment:

    Kernel mode

Notes:

    Copyright (c) 2006 HI-TECH Software.  
    All Rights Reserved.

--*/

#include "htusbusb.h"
#include "htusbpwr.h"
#include "htusbpnp.h"
#include "htusbdev.h"
#include "htusbrwr.h"
#include "htusbwmi.h"
#include "htusbusr.h"


void
BulkUsb_Cleanup(PBULKUSB_RW_CONTEXT rwContext)

	//cleanup a rwContext after completion

{
	if(rwContext != NULL) {
		BulkUsb_IoDecrement(rwContext->DeviceExtension);
		ExFreePool(rwContext->Urb);
		IoFreeMdl(rwContext->Mdl);
		ExFreePool(rwContext);
	}
}

NTSTATUS
BulkUsb_DispatchReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    Dispatch routine for read and write.
    This routine creates a BULKUSB_RW_CONTEXT for a read/write.
    This read/write is performed in stages of BULKUSB_MAX_TRANSFER_SIZE.
    once a stage of transfer is complete, then the irp is circulated again, 
    until the requested length of tranfer is performed.

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet

Return Value:

    NT status value

--*/
{
    PMDL                   mdl;
    PURB                   urb;
    ULONG                  totalLength;
    ULONG                  stageLength;
    ULONG                  urbFlags;
    BOOLEAN                read;
    NTSTATUS               ntStatus;
    ULONG_PTR              virtualAddress;
    PFILE_OBJECT           fileObject;
    PDEVICE_EXTENSION      deviceExtension;
    PIO_STACK_LOCATION     irpStack;
    PIO_STACK_LOCATION     nextStack;
    PBULKUSB_RW_CONTEXT    rwContext;
    PUSBD_PIPE_INFORMATION pipeInformation;
	ULONG					timeoutVal;

    //
    // initialize variables
    //
    urb = NULL;
    mdl = NULL;
    rwContext = NULL;
    totalLength = 0;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpStack->FileObject;
    read = (irpStack->MajorFunction == IRP_MJ_READ);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    BulkUsb_DbgPrint(3, ("BulkUsb_DispatchReadWrite - begins\n"));

    if(deviceExtension->DeviceState != Working) {

        BulkUsb_DbgPrint(1, ("Invalid device state\n"));

        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto BulkUsb_DispatchReadWrite_Exit;
    }

    if(fileObject == NULL || fileObject->FsContext == NULL) {
        BulkUsb_DbgPrint(1, ("Invalid handle\n"));

        ntStatus = STATUS_INVALID_HANDLE;
        goto BulkUsb_DispatchReadWrite_Exit;
    }

	pipeInformation = ((PBULKUSB_PIPEINFO)fileObject->FsContext)->Pipe;
	if((UsbdPipeTypeBulk != pipeInformation->PipeType) &&
	   (UsbdPipeTypeInterrupt != pipeInformation->PipeType)) {
		
		BulkUsb_DbgPrint(1, ("Usbd pipe type is not bulk or interrupt\n"));

		ntStatus = STATUS_INVALID_HANDLE;
		goto BulkUsb_DispatchReadWrite_Exit;
	}
	timeoutVal = ((PBULKUSB_PIPEINFO)fileObject->FsContext)->Timeout;
    rwContext = (PBULKUSB_RW_CONTEXT)
                ExAllocatePool(NonPagedPool,
                               sizeof(BULKUSB_RW_CONTEXT));

    if(rwContext == NULL) {
        BulkUsb_DbgPrint(1, ("Failed to alloc mem for rwContext\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto BulkUsb_DispatchReadWrite_Exit;
    }

    if(Irp->MdlAddress)
        totalLength = MmGetMdlByteCount(Irp->MdlAddress);

    if(totalLength == 0) {
        BulkUsb_DbgPrint(1, ("Transfer data length = 0\n"));
        ntStatus = STATUS_SUCCESS;
        ExFreePool(rwContext);
        goto BulkUsb_DispatchReadWrite_Exit;
    }

    urbFlags = USBD_SHORT_TRANSFER_OK;
    virtualAddress = (ULONG_PTR) MmGetMdlVirtualAddress(Irp->MdlAddress);

    if(read) {
        urbFlags |= USBD_TRANSFER_DIRECTION_IN;
        BulkUsb_DbgPrint(3, ("Read operation\n"));
    } else {
        urbFlags |= USBD_TRANSFER_DIRECTION_OUT;
        BulkUsb_DbgPrint(3, ("Write operation\n"));
    }
    mdl = IoAllocateMdl((PVOID) virtualAddress,
                        totalLength,
                        FALSE,
                        FALSE,
                        NULL);

    if(mdl == NULL) {
        BulkUsb_DbgPrint(1, ("Failed to alloc mem for mdl\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(rwContext);
        goto BulkUsb_DispatchReadWrite_Exit;
    }

    //
    // map the portion of user-buffer described by an mdl to another mdl
    //
    IoBuildPartialMdl(Irp->MdlAddress,
                      mdl,
                      (PVOID) virtualAddress,
                      totalLength);

    urb = ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));

    if(urb == NULL) {
        BulkUsb_DbgPrint(1, ("Failed to alloc mem for urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(rwContext);
        IoFreeMdl(mdl);
        goto BulkUsb_DispatchReadWrite_Exit;
    }

    UsbBuildInterruptOrBulkTransferRequest(
                            urb,
                            sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                            pipeInformation->PipeHandle,
                            NULL,
                            mdl,
                            totalLength,
                            urbFlags,
                            NULL);

    //
    // set BULKUSB_RW_CONTEXT parameters.
    //
    
    rwContext->Urb				= urb;
    rwContext->Mdl				= mdl;
    rwContext->Numxfer			= 0;
    rwContext->DeviceExtension	= deviceExtension;
    rwContext->Lock				= IRPLOCK_NULL;
	KeInitializeEvent(&rwContext->Event, NotificationEvent, FALSE);
	if(timeoutVal != 0)
		rwContext->Lock = IRPLOCK_CANCELABLE;

    //
    // use the original read/write irp as an internal device control irp
    //

    nextStack = IoGetNextIrpStackLocation(Irp);
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.Others.Argument1 = (PVOID) urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = 
                                             IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(Irp, 
                           (PIO_COMPLETION_ROUTINE)BulkUsb_ReadWriteCompletion,
                           rwContext,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // since we return STATUS_PENDING call IoMarkIrpPending.
    // This is the boiler plate code.
    // This may cause extra overhead of an APC for the Irp completion
    // but this is the correct thing to do.
    //

    IoMarkIrpPending(Irp);

    BulkUsb_DbgPrint(3, ("BulkUsb_DispatchReadWrite::"));
    BulkUsb_IoIncrement(deviceExtension);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                            Irp);

	// do we need to wait for completion and timeout?
	// The logic is as follows:
	//
	// if Lock == IRPLOCK_NULL, then no timeout is set; this thread returns STATUS_PENDING
	// and the completion routine operates normally. Cleanup is done by the completion routine.
	//
	// if Lock  == IRPLOCK_CANCELABLE then we will wait in this thread until the Event
	// is signalled, or the timeout occurs.
	// signalling of the Event is done as the last action of the completion routine,
	// which will already have set Lock to COMPLETED. So on return from the WaitForSingleObject,
	// it is not necessary to check the return value. It suffices to check the Lock value.
	//
	// If the timeout occurred, we call IoCancelIrp to cancel the transfer. In most cases
	// this will successfully cancel the Irp and the completion routine will subsequently
	// be called (with an appropriate status).
	//
	// If however, the request completes between the return from the Wait and the Cancel
	// request, the Cancel may not occur. In this case the completion routine will have set
	// Lock to COMPLETED, but then detected the cancel-in-progress, and terminated processing.
	// In that instance, we have to call IOCompleteRequest
	//
	// Cleanup (deleting the memory allocated to rwContext and members) is done by
	// the completion routine if the request is asynchronous (no timeout) or by
	// the thread for a synchronous request with timeout. The completion routine
	// always signals the Event, so the cleanup in this thread is conditional on
	// the event. This ensures that the cleanup is done only when both the thread
	// and the completion routine are finished with the request.

	if(ntStatus == STATUS_PENDING && timeoutVal != 0) {
		LARGE_INTEGER	tmout;
		tmout.QuadPart = -(LONGLONG)timeoutVal*10000L;
		KeWaitForSingleObject(
				&rwContext->Event,
				Executive, // wait reason
				KernelMode, // To prevent stack from being paged out.
				FALSE,     // You are not alertable
				&tmout);   
		// do we need to cancel the request?
		if(InterlockedExchange(&rwContext->Lock, IRPLOCK_CANCEL_STARTED) == IRPLOCK_CANCELABLE) {
			IoCancelIrp(Irp);		// cancel it - we got here first
			if(InterlockedExchange(&rwContext->Lock, IRPLOCK_CANCEL_COMPLETE) == IRPLOCK_COMPLETED) {
				// but the completion routine sneaked in at the last moment, and will have
				// bailed, leaving us to finish up
				IoCompleteRequest(Irp, IO_NO_INCREMENT);
			}
			ntStatus = STATUS_TIMEOUT;
		}
	}
	if(timeoutVal != 0) {		// it was a synchronous request - we have to clean up.
		KeWaitForSingleObject(	// ensure the completion routine is completed.
				&rwContext->Event,
				Executive, // wait reason
				KernelMode, // To prevent stack from being paged out.
				FALSE,     // You are not alertable
				NULL);   
		BulkUsb_Cleanup(rwContext);
	}

	if(!NT_SUCCESS(ntStatus)) {

        BulkUsb_DbgPrint(1, ("IoCallDriver fails with status %X\n", ntStatus));

        //
        // if the device was yanked out, then the pipeInformation 
        // field is invalid.
        // similarly if the request was cancelled, then we need not
        // invoked reset pipe/device.
        //
        if((ntStatus != STATUS_CANCELLED) && 
           (ntStatus != STATUS_DEVICE_NOT_CONNECTED)) {
            
            ntStatus = BulkUsb_ResetPipe(DeviceObject,
                                     pipeInformation);
            if(!NT_SUCCESS(ntStatus)) {
                BulkUsb_DbgPrint(1, ("BulkUsb_ResetPipe failed\n"));
                ntStatus = BulkUsb_ResetDevice(DeviceObject);
            }
        }
        else
            BulkUsb_DbgPrint(3, ("ntStatus is STATUS_CANCELLED or "
                                 "STATUS_DEVICE_NOT_CONNECTED\n"));
		ntStatus = STATUS_PENDING;
    }

    //
    // we return STATUS_PENDING and not the status returned by the lower layer.
    //
    return STATUS_PENDING;

BulkUsb_DispatchReadWrite_Exit:

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    BulkUsb_DbgPrint(3, ("BulkUsb_DispatchReadWrite - ends\n"));

    return ntStatus;
}

NTSTATUS
BulkUsb_ReadWriteCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++
 
Routine Description:

    This is the completion routine for reads/writes
    If the irp completes with success, we check if we
    need to recirculate this irp for another stage of
    transfer. In this case return STATUS_MORE_PROCESSING_REQUIRED.
    if the irp completes in error, free all memory allocs and
    return the status.

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet
    Context - context passed to the completion routine.

Return Value:

    NT status value

--*/
{
    NTSTATUS            ntStatus;
    PIO_STACK_LOCATION  nextStack;
    PBULKUSB_RW_CONTEXT rwContext;
	IRPLOCK				lockVal;

    //
    // initialize variables
    //
    rwContext = (PBULKUSB_RW_CONTEXT) Context;
    ntStatus = Irp->IoStatus.Status;

    UNREFERENCED_PARAMETER(DeviceObject);
    BulkUsb_DbgPrint(3, ("BulkUsb_ReadWriteCompletion - begins\n"));

    // this should never be null
	if(rwContext != NULL) {
		lockVal = InterlockedExchange(&rwContext->Lock, IRPLOCK_COMPLETED);
		if(lockVal == IRPLOCK_CANCEL_STARTED)
			return STATUS_MORE_PROCESSING_REQUIRED;
		Irp->IoStatus.Information =
			rwContext->Urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
		if(lockVal == IRPLOCK_NULL)		// asynchronous request?
			BulkUsb_Cleanup(rwContext);	// we have to clean up
		else
			KeSetEvent(&rwContext->Event, IO_NO_INCREMENT, FALSE);
	}

    BulkUsb_DbgPrint(3, ("BulkUsb_ReadWriteCompletion - ends\n"));

    return ntStatus;
}

