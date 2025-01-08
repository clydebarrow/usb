/*++

Copyright (c) 2006  HI-TECH Software

Module Name:

    bulkdev.c

Abstract:

    This file contains dispatch routines for create and close.
	Selective suspend on idle is not supported.

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

NTSTATUS
BulkUsb_PipeWithName(
    IN PUNICODE_STRING FileName,
	OUT PBULKUSB_PIPEINFO	Pipeinfo
    )
/*++
 
Routine Description:

    This routine will pass the string pipe name and
    fetch the pipe number.

Arguments:

    DeviceObject - pointer to DeviceObject
    FileName - string pipe name

Return Value:

    The device extension maintains a pipe context for 
    the pipes on the HI-TECH Software USB device.
    This routine returns the pointer to this context in
    the device extension for the "FileName" pipe.

	The syntax of filenames is:

		\<full path to device>\<pipeno>[#<timeout>]

		e.g.

		\\?\usb#vid_1725&pid_0002#00000060005#{37abc9bb-51fe-47bd-bacd-a974bc4111e8}\0#5000

		will reference the first bulk pipe on the device, with a timeout of 5 seconds.

		The parsing of the filename portion doesn't try too hard. It will succeed
		as long as the name starts with a digit.

--*/
{
    LONG                  ix;
    ULONG                 uval; 
    ULONG                 nameLength;

    nameLength = (FileName->Length / sizeof(WCHAR));

	if(nameLength > 1 && FileName->Buffer[0] == (WCHAR) '\\' &&
			FileName->Buffer[1] >= (WCHAR)'0' && FileName->Buffer[1] <= (WCHAR)'9') {

		BulkUsb_DbgPrint(3, ("Filename = %ws nameLength = %d\n", FileName->Buffer, nameLength));

		ix = 1;
		uval = 0;

		do {
			uval *= 10;
			uval += FileName->Buffer[ix] - (WCHAR)'0';
			ix++;
		} while(ix != nameLength &&
				(FileName->Buffer[ix] >= (WCHAR) '0' && FileName->Buffer[ix] <= (WCHAR) '9'));
		Pipeinfo->Pipeno = uval;
		Pipeinfo->Timeout = 0;
		if(ix != nameLength && FileName->Buffer[ix] == (WCHAR)'#') {
			uval = 0;
			ix++;
			while(ix != nameLength &&
					(FileName->Buffer[ix] >= (WCHAR) '0' && FileName->Buffer[ix] <= (WCHAR) '9')) {
				uval *= 10;
				uval += FileName->Buffer[ix] - (WCHAR)'0';
				ix++;
			}
			Pipeinfo->Timeout = uval;
		}
		return STATUS_SUCCESS;
	}
	return STATUS_INVALID_PARAMETER;
}
NTSTATUS
BulkUsb_DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    Dispatch routine for create.

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet.

Return Value:

    NT status value

--*/
{
    ULONG                       i;
    NTSTATUS                    ntStatus;
    PFILE_OBJECT                fileObject;
    PDEVICE_EXTENSION           deviceExtension;
    PIO_STACK_LOCATION          irpStack;
    PBULKUSB_PIPE_CONTEXT       pipeContext;
    BULKUSB_PIPEINFO       		pipeInfo, * ppipeInfo;
    PUSBD_INTERFACE_INFORMATION interface;

    PAGED_CODE();

    BulkUsb_DbgPrint(3, ("BulkUsb_DispatchCreate - begins\n"));

    //
    // initialize variables
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpStack->FileObject;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if(deviceExtension->DeviceState != Working) {

        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto BulkUsb_DispatchCreate_Exit;
    }

    if(deviceExtension->UsbInterface)
        interface = deviceExtension->UsbInterface;
    else {
        BulkUsb_DbgPrint(1, ("UsbInterface not found\n"));
        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto BulkUsb_DispatchCreate_Exit;
    }

    // FsContext is Null for the device
 
    if(fileObject)
        fileObject->FsContext = NULL; 
    else {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto BulkUsb_DispatchCreate_Exit;
    }

    if(0 == fileObject->FileName.Length) {

        // opening a device as opposed to pipe.
        //
        ntStatus = STATUS_SUCCESS;
	} else {
		ntStatus = BulkUsb_PipeWithName(&fileObject->FileName, &pipeInfo);
		if(ntStatus != STATUS_SUCCESS || pipeInfo.Pipeno >= interface->NumberOfPipes) {
			ntStatus = STATUS_INVALID_PARAMETER;
			goto BulkUsb_DispatchCreate_Exit;
		}
		pipeContext = &deviceExtension->PipeContext[pipeInfo.Pipeno];
		ppipeInfo = (PBULKUSB_PIPEINFO)ExAllocatePool(NonPagedPool,
				sizeof(BULKUSB_PIPEINFO));
		ppipeInfo->Timeout = pipeInfo.Timeout;
		ppipeInfo->Pipeno = pipeInfo.Pipeno;
		ppipeInfo->Pipe = interface->Pipes+pipeInfo.Pipeno;
		fileObject->FsContext = ppipeInfo;
		pipeContext->PipeOpen = TRUE;
	}
	// increment OpenHandleCounts
	InterlockedIncrement(&deviceExtension->OpenHandleCount);


BulkUsb_DispatchCreate_Exit:

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    BulkUsb_DbgPrint(3, ("BulkUsb_DispatchCreate - ends\n"));
    
    return ntStatus;
}

NTSTATUS
BulkUsb_DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    Dispatch routine for close.

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet

Return Value:

    NT status value

--*/
{
    NTSTATUS               ntStatus;
    PFILE_OBJECT           fileObject;
    PDEVICE_EXTENSION      deviceExtension;
    PIO_STACK_LOCATION     irpStack;
    PBULKUSB_PIPE_CONTEXT  pipeContext;
    PBULKUSB_PIPEINFO		pipeInfo;
    
    PAGED_CODE();

    //
    // initialize variables
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpStack->FileObject;
    pipeContext = NULL;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    BulkUsb_DbgPrint(3, ("BulkUsb_DispatchClose - begins\n"));

    if(fileObject && fileObject->FsContext) {

		pipeInfo = fileObject->FsContext;
		pipeContext = &deviceExtension->PipeContext[pipeInfo->Pipeno];
        if(pipeContext && pipeContext->PipeOpen)
            pipeContext->PipeOpen = FALSE;
    }

    //
    // set ntStatus to STATUS_SUCCESS 
    //
    ntStatus = STATUS_SUCCESS;

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    InterlockedDecrement(&deviceExtension->OpenHandleCount);

    BulkUsb_DbgPrint(3, ("BulkUsb_DispatchClose - ends\n"));

    return ntStatus;
}

NTSTATUS
BulkUsb_DispatchDevCtrl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
 
Routine Description:

    Dispatch routine for IRP_MJ_DEVICE_CONTROL

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet

Return Value:

    NT status value

--*/
{
    ULONG              code;
    PVOID              ioBuffer;
    ULONG              inputBufferLength;
    ULONG              outputBufferLength;
    ULONG              info;
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;

    //
    // initialize variables
    //
    info = 0;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    code = irpStack->Parameters.DeviceIoControl.IoControlCode;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if(deviceExtension->DeviceState != Working) {

        BulkUsb_DbgPrint(1, ("Invalid device state\n"));

        Irp->IoStatus.Status = ntStatus = STATUS_INVALID_DEVICE_STATE;
        Irp->IoStatus.Information = info;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return ntStatus;
    }

    BulkUsb_DbgPrint(3, ("BulkUsb_DispatchDevCtrl::"));
    BulkUsb_IoIncrement(deviceExtension);

    switch(code) {

    case IOCTL_BULKUSB_RESET_PIPE:
    {
        PFILE_OBJECT			fileObject;
        PUSBD_PIPE_INFORMATION	pipe;

        pipe = NULL;
        fileObject = NULL;

        //
        // FileObject is the address of the kernel file object to
        // which the IRP is directed. Drivers use the FileObject
        // to correlate IRPs in a queue.
        //
        fileObject = irpStack->FileObject;

        if(fileObject == NULL || fileObject->FsContext == NULL) {
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
        }
        pipe = ((PBULKUSB_PIPEINFO)fileObject->FsContext)->Pipe;

        if(pipe == NULL)
            ntStatus = STATUS_INVALID_PARAMETER;
        else
            ntStatus = BulkUsb_ResetPipe(DeviceObject, pipe);

        break;
    }

    case IOCTL_BULKUSB_GET_CONFIG_DESCRIPTOR:
    {
        ULONG length;

        if(deviceExtension->UsbConfigurationDescriptor) {

            length = deviceExtension->UsbConfigurationDescriptor->wTotalLength;

            if(outputBufferLength >= length) {

                RtlCopyMemory(ioBuffer,
                              deviceExtension->UsbConfigurationDescriptor,
                              length);

                info = length;

                ntStatus = STATUS_SUCCESS;
            }
            else {
                
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }
        }
        else {
            
            ntStatus = STATUS_UNSUCCESSFUL;
        }

        break;
    }

    case IOCTL_BULKUSB_RESET_DEVICE:
        
        ntStatus = BulkUsb_ResetDevice(DeviceObject);

        break;

    default :

        ntStatus = STATUS_INVALID_DEVICE_REQUEST;

        break;
    }

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = info;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    BulkUsb_DbgPrint(3, ("BulkUsb_DispatchDevCtrl::"));
    BulkUsb_IoDecrement(deviceExtension);

    return ntStatus;
}

NTSTATUS
BulkUsb_ResetPipe(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PUSBD_PIPE_INFORMATION PipeInfo
    )
/*++
 
Routine Description:

    This routine synchronously submits a URB_FUNCTION_RESET_PIPE
    request down the stack.

Arguments:

    DeviceObject - pointer to device object
    PipeInfo - pointer to PipeInformation structure
               to retrieve the pipe handle

Return Value:

    NT status value

--*/
{
    PURB              urb;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    //
    // initialize variables
    //

    urb = NULL;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;


    urb = ExAllocatePool(NonPagedPool, 
                         sizeof(struct _URB_PIPE_REQUEST));

    if(urb) {

        urb->UrbHeader.Length = (USHORT) sizeof(struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
        urb->UrbPipeRequest.PipeHandle = PipeInfo->PipeHandle;

        ntStatus = CallUSBD(DeviceObject, urb);

        ExFreePool(urb);
    }
    else {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if(NT_SUCCESS(ntStatus)) {
    
        BulkUsb_DbgPrint(3, ("BulkUsb_ResetPipe - success\n"));
        ntStatus = STATUS_SUCCESS;
    }
    else {

        BulkUsb_DbgPrint(1, ("BulkUsb_ResetPipe - failed\n"));
    }

    return ntStatus;
}

NTSTATUS
BulkUsb_ResetDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
 
Routine Description:

    This routine invokes BulkUsb_ResetParentPort to reset the device

Arguments:

    DeviceObject - pointer to device object

Return Value:

    NT status value

--*/
{
    NTSTATUS ntStatus;
    ULONG    portStatus;

    BulkUsb_DbgPrint(3, ("BulkUsb_ResetDevice - begins\n"));

    ntStatus = BulkUsb_GetPortStatus(DeviceObject, &portStatus);

    if((NT_SUCCESS(ntStatus))                 &&
       (!(portStatus & USBD_PORT_ENABLED))    &&
       (portStatus & USBD_PORT_CONNECTED)) {

        ntStatus = BulkUsb_ResetParentPort(DeviceObject);
    }

    BulkUsb_DbgPrint(3, ("BulkUsb_ResetDevice - ends\n"));

    return ntStatus;
}

NTSTATUS
BulkUsb_GetPortStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PULONG     PortStatus
    )
/*++
 
Routine Description:

    This routine retrieves the status value

Arguments:

    DeviceObject - pointer to device object
    PortStatus - port status

Return Value:

    NT status value

--*/
{
    NTSTATUS           ntStatus;
    KEVENT             event;
    PIRP               irp;
    IO_STATUS_BLOCK    ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION  deviceExtension;

    //
    // initialize variables
    //
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    *PortStatus = 0;

    BulkUsb_DbgPrint(3, ("BulkUsb_GetPortStatus - begins\n"));

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                    IOCTL_INTERNAL_USB_GET_PORT_STATUS,
                    deviceExtension->TopOfStackDeviceObject,
                    NULL,
                    0,
                    NULL,
                    0,
                    TRUE,
                    &event,
                    &ioStatus);

    if(NULL == irp) {

        BulkUsb_DbgPrint(1, ("memory alloc for irp failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(irp);

    ASSERT(nextStack != NULL);

    nextStack->Parameters.Others.Argument1 = PortStatus;

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);

    if(STATUS_PENDING == ntStatus) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }
    else {

        ioStatus.Status = ntStatus;
    }

    ntStatus = ioStatus.Status;

    BulkUsb_DbgPrint(3, ("BulkUsb_GetPortStatus - ends\n"));

    return ntStatus;
}

NTSTATUS
BulkUsb_ResetParentPort(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
 
Routine Description:

    This routine sends an IOCTL_INTERNAL_USB_RESET_PORT
    synchronously down the stack.

Arguments:

Return Value:

--*/
{
    NTSTATUS           ntStatus;
    KEVENT             event;
    PIRP               irp;
    IO_STATUS_BLOCK    ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION  deviceExtension;

    //
    // initialize variables
    //
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    BulkUsb_DbgPrint(3, ("BulkUsb_ResetParentPort - begins\n"));

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                    IOCTL_INTERNAL_USB_RESET_PORT,
                    deviceExtension->TopOfStackDeviceObject,
                    NULL,
                    0,
                    NULL,
                    0,
                    TRUE,
                    &event,
                    &ioStatus);

    if(NULL == irp) {

        BulkUsb_DbgPrint(1, ("memory alloc for irp failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(irp);

    ASSERT(nextStack != NULL);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);

    if(STATUS_PENDING == ntStatus) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }
    else {

        ioStatus.Status = ntStatus;
    }

    ntStatus = ioStatus.Status;

    BulkUsb_DbgPrint(3, ("BulkUsb_ResetParentPort - ends\n"));

    return ntStatus;
}


VOID
WWIrpCompletionFunc(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++
 
Routine Description:

    Completion routine for PoRequest wait wake irp

Arguments:

    DeviceObject - pointer to device object
    MinorFunciton - minor function for the irp.
    PowerState - irp power state
    Context - context passed to the completion function
    IoStatus - status block.    

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    
    //
    // initialize variables
    //
    DeviceExtension = (PDEVICE_EXTENSION) Context;

    //
    // all we do is decrement the count
    //
    
    BulkUsb_DbgPrint(3, ("WWIrpCompletionFunc::"));
    BulkUsb_IoDecrement(DeviceExtension);

    return;
}


