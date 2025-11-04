#include"fh.h"



NTSTATUS AddPath(PUNICODE_STRING FilePath)
{
    NTSTATUS status = STATUS_SUCCESS;
    //假设是合法字符串

    USHORT DosSub = FilePath->Buffer[0] - L'A', NTSub;  //留下0表示NULL，如果从C:开始就会占用0，所以从A开始


	if (ps.DosList[DosSub].Head == NULL)    //第一次添加该盘符，建立从\\Device\\HarddiskVolumeX到C:的映射
    {
        PVOID TempBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, 64, 'Temp');
        if (TempBuffer == NULL)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "AllocFail\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlStringCbPrintfW(TempBuffer, 64, L"\\??\\%c:", (WCHAR)FilePath->Buffer[0]);
        UNICODE_STRING Temp = { 12, 64, TempBuffer };
        OBJECT_ATTRIBUTES oa;
        InitializeObjectAttributes(&oa, &Temp, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
        HANDLE hLink;
		status = ZwOpenSymbolicLinkObject(&hLink, SYMBOLIC_LINK_QUERY, &oa);    //查询符号链接
		RtlInitEmptyUnicodeString(&Temp, TempBuffer, 64);
        //RtlZeroMemory(TempBuffer, 64);
        /*
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Open link End\n"));
            return STATUS_SUCCESS;
        }
        
        */

        if (!NT_SUCCESS(status))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Open link fail\n"));
            return status;
        }

        ULONG ulSize = 0;

        status = ZwQuerySymbolicLinkObject(hLink, &Temp, &ulSize);
        if (!NT_SUCCESS(status))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Query link fail\n"));
            return status;
        }
        UNICODE_STRING dhv = RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume");
        if (RtlCompareMemory(Temp.Buffer, dhv.Buffer, dhv.Length) != dhv.Length)
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Invalid NTHeader\n"));
            return STATUS_INVALID_PARAMETER;
        }
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NTSub:  %d\n", Temp.Buffer[dhv.Length / sizeof(WCHAR)]));
        NTSub = (USHORT)(Temp.Buffer[dhv.Length / sizeof(WCHAR)] - L'0');
        ps.SubToDos[NTSub] = DosSub;
        ExFreePoolWithTag(TempBuffer, 'Temp');
    }
    
    PVOID NodeBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(DosNode), 'Node');    //为什么内核没有次方函数
    if (NodeBuffer == NULL)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "AllocFail\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    USHORT i;
    for (i = FilePath->Length / sizeof(WCHAR) - 1; i > 2; i--)
        if (FilePath->Buffer[i] == L'\\')
            break;
    PVOID ParentDirBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, (i - 2) * sizeof(WCHAR), 'DosP');  //具体偏移是在WinDbg手动调出来的所以很丑陋
    if (ParentDirBuffer == NULL)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "AllocFail\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(ParentDirBuffer, &(FilePath->Buffer[2]), (i - 2) * sizeof(WCHAR));
    RtlInitUnicodeString(&(((PDosNode)NodeBuffer)->ParentDir), ParentDirBuffer);
    PVOID NameBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, FilePath->Length - (i + 1) * sizeof(WCHAR), 'DosN');
    if (NameBuffer == NULL)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "AllocFail\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(NameBuffer, &(FilePath->Buffer[i + 1]), FilePath->Length - (i + 1) * sizeof(WCHAR));
    RtlInitUnicodeString(&(((PDosNode)NodeBuffer)->FileName), NameBuffer);
    ((PDosNode)NodeBuffer)->Next = NULL;
    
    if (ps.DosList[DosSub].Head == NULL)
    {
        ps.DosList[DosSub].Head = NodeBuffer;
        ps.DosList[DosSub].Tail = NodeBuffer;
        ps.DosList[DosSub].num = 1;
    }
    else
    {
        ps.DosList[DosSub].Tail->Next = NodeBuffer;
        ps.DosList[DosSub].Tail = NodeBuffer;
        ps.DosList[DosSub].num++;
    }
        
    return STATUS_SUCCESS;

}