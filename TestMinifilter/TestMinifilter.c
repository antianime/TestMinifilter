#include "fh.h"

UNICODE_STRING TheFileName = RTL_CONSTANT_STRING(L"C:\\Users\\WDKRemoteUser\\Desktop\\TEST\\test.txt");
UNICODE_STRING TheFileName2 = RTL_CONSTANT_STRING(L"C:\\Users\\WDKRemoteUser\\Desktop\\TEST\\TEST\\test.txt");

PFLT_FILTER FilterHandle = NULL;
UNICODE_STRING PortName = RTL_CONSTANT_STRING(L"\\Port_nb666");
PFLT_PORT ServerPort;
BOOLEAN FilterSwitch = TRUE;

NTSTATUS
FilterUnload(
    FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);
    FltCloseCommunicationPort(ServerPort);
    FltUnregisterFilter(FilterHandle);

    PDosNode pdn;
    for (DWORD i = 0; i < 26; i++)
    {
        if (ps.DosList[i].num == 0)
            continue;
        pdn = ps.DosList[i].Head;
        do
        {
            ExFreePoolWithTag(pdn->FileName.Buffer, 'DosN');
            ExFreePoolWithTag(pdn->ParentDir.Buffer, 'DosP');
            ExFreePoolWithTag(pdn, 'Node');
            pdn = pdn->Next;
        } while (pdn != NULL);
    }


    return STATUS_SUCCESS;
}

FLT_POSTOP_CALLBACK_STATUS nb666Post(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(FltObjects);

    PFLT_FILE_NAME_INFORMATION pInfo;
    FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pInfo);
    FltParseFileNameInformation(pInfo);

    USHORT ThisNTSub = pInfo->Volume.Buffer[pInfo->Volume.Length / sizeof(WCHAR) - 1] - L'0';
    UNICODE_STRING ConstantNTHeader = RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume");

    if (RtlCompareMemory(pInfo->Volume.Buffer, ConstantNTHeader.Buffer, ConstantNTHeader.Length) != ConstantNTHeader.Length || ps.SubToDos[ThisNTSub] == 0)//是否属于\\Device\\HarddiskVolume
    {
        return FLT_POSTOP_FINISHED_PROCESSING;
    }
    USHORT ThisDosSub = ps.SubToDos[ThisNTSub];
    

    PDosNode pdn = ps.DosList[ThisDosSub].Head;
    for (; pdn != NULL; pdn = pdn->Next)
    {
		USHORT templen = pInfo->Name.Length - pInfo->Volume.Length;
        UNICODE_STRING Tempstr = { templen, templen + 2, (PWCHAR)((PUCHAR)pInfo->Name.Buffer + pInfo->Volume.Length) };
        if (templen == pdn->ParentDir.Length && RtlEqualUnicodeString(&Tempstr, &pdn->ParentDir, TRUE))
            break;

        if (pdn == NULL || pdn->Next == NULL)
			return FLT_POSTOP_FINISHED_PROCESSING;
            
    }

    //先获取偏移
    ULONG ulOffset_NextEntryOffset = 0;
    ULONG ulOffset_FileName = 0;
    ULONG ulOffset_FileNameLen = 0;
    ULONG* Destination;

	switch (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass)    //根据查询的消息类型获取偏移//赞美微软-1秒
    {
    case FileDirectoryInformation:
        Destination = (ULONG[])FileDirectoryInformationDefinition;
        break;
    case FileFullDirectoryInformation:
        Destination = (ULONG[])FileFullDirectoryInformationDefinition;
        break;
    case FileBothDirectoryInformation:
        Destination = (ULONG[])FileBothDirectoryInformationDefinition;
        break;
    case FileNamesInformation:
        Destination = (ULONG[])FileNamesInformationDefinition;
        break;
    case FileIdBothDirectoryInformation:
        Destination = (ULONG[])FileIdBothDirectoryInformationDefinition;
        break;
    case FileIdFullDirectoryInformation:
        Destination = (ULONG[])FileIdFullDirectoryInformationDefinition;
        break;
    case FileIdExtdDirectoryInformation:
        Destination = (ULONG[])FileIdExtdDirectoryInformationDefinition;
        break;

    default:
        return FLT_POSTOP_FINISHED_PROCESSING;
        break;
    }
    ulOffset_NextEntryOffset = Destination[1];
    ulOffset_FileName = Destination[2];
    ulOffset_FileNameLen = Destination[3];



    PVOID Base = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;
    ULONG NextEntryOffset;
    PWCHAR FileName;
    ULONG FileNameLen;
    PVOID last = NULL;

    for (;;)
    {
        NextEntryOffset = *(ULONG*)((PUCHAR)Base + ulOffset_NextEntryOffset);

        FileName = (PWCHAR)((PUCHAR)Base + ulOffset_FileName);
        FileNameLen = *(ULONG*)((PUCHAR)Base + ulOffset_FileNameLen);
        if (FileNameLen == pdn->FileName.Length && RtlCompareMemory(FileName, pdn->FileName.Buffer, FileNameLen) == FileNameLen)
        {
            if (last == NULL)   //第一个
            {
                if (NextEntryOffset == 0) //且最后一个
                {
                    //直接返回
                    Data->IoStatus.Information = 0;
                    Data->IoStatus.Status = STATUS_NO_MORE_ENTRIES;
                }
                else//不是最后一个
                {
                    PVOID Next = (PVOID)((PUCHAR)Base + NextEntryOffset);
                    ULONG Next_NextEntryOffset = *(ULONG*)((PUCHAR)Next + ulOffset_NextEntryOffset);
                    RtlMoveMemory(Base, Next, Next_NextEntryOffset);
                    if (Next_NextEntryOffset != 0);
                        *(ULONG*)((PUCHAR)Base + ulOffset_NextEntryOffset) = NextEntryOffset + Next_NextEntryOffset;
                }

            }
            else//不是第一个
            {
                if (NextEntryOffset == 0)    //最后一个
                    *(ULONG*)((PUCHAR)last + ulOffset_NextEntryOffset) = 0;
                else
                    *(ULONG*)((PUCHAR)last + ulOffset_NextEntryOffset) += *(ULONG*)((PUCHAR)Base + ulOffset_NextEntryOffset);
            }
            break;
        }


        if (NextEntryOffset == 0)
            break;
        last = Base;
        Base = (PVOID)((PUCHAR)Base + NextEntryOffset);
    }
	FltReleaseFileNameInformation(pInfo);
    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS nb666(    //预处理，仅放行目录查询相关IRP
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    //_Outptr_result_maybenull_ PVOID* CompletionContext
    PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(FltObjects);

    PFLT_PARAMETERS Params = &Data->Iopb->Parameters;
    if (Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY)
        return FLT_PREOP_SUCCESS_NO_CALLBACK;   //直接放行

    switch (Params->DirectoryControl.QueryDirectory.FileInformationClass)
    {
    case FileDirectoryInformation:
    case FileFullDirectoryInformation:
    case FileBothDirectoryInformation:
    case FileNamesInformation:
    case FileIdBothDirectoryInformation:
    case FileIdFullDirectoryInformation:
    case FileIdExtdDirectoryInformation:

        return FLT_PREOP_SUCCESS_WITH_CALLBACK;
        break;

    default:
        break;
    }

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;

}

const FLT_OPERATION_REGISTRATION Callbacks[] = {    //用于各种IRP的回调，这里只需要处理目录查询

    { IRP_MJ_DIRECTORY_CONTROL, 0, nb666, nb666Post },
    { IRP_MJ_OPERATION_END }
};

FLT_REGISTRATION FilterRegistration = { //过滤器注册信息
    sizeof(FLT_REGISTRATION),         // Size
    FLT_REGISTRATION_VERSION,         // Version
    0,                                // Flags
    NULL,                             // Context
    Callbacks,                             // Operation callbacks
    FilterUnload,                             // FilterUnload
    NULL,//SetupCallback,                             // InstanceSetup
    NULL,                             // InstanceQueryTeardown
    NULL,                             // InstanceTeardownStart
    NULL,                             // InstanceTeardownComplete
    NULL,                             // GenerateFileName
    NULL,                             // NormalizeNameComponent
    NULL,                             // NormalizeContextCleanup
    NULL,                             // TransactionNotification
    NULL                              // NormalizeNameComponentEx
};


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);


    RtlZeroMemory(&ps, sizeof(PathStruct));
    AddPath(&TheFileName);
    AddPath(&TheFileName2);

    status = FltRegisterFilter(DriverObject,
        &FilterRegistration,
        &FilterHandle);

    FLT_ASSERT(NT_SUCCESS(status));

    if (NT_SUCCESS(status)) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering(FilterHandle);

        if (!NT_SUCCESS(status)) 
        {
            FltUnregisterFilter(FilterHandle);
        }


		//Communication Setup
        status = CommunicationSetup();
        if(!NT_SUCCESS(status))
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CreateCommunicationPort failed: 0x%X\n", status));

    }

    return status;
}