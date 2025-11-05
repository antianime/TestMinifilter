#include"fh.h"

NTSTATUS ConnectCallback(
    PFLT_PORT ClientPort,
    PVOID ServerPortCookie,
    PVOID ConnectionContext,
    ULONG SizeOfContext,
    PVOID* ConnectionPortCookie
)
{
    UNREFERENCED_PARAMETER(ClientPort);
    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);
    UNREFERENCED_PARAMETER(ConnectionPortCookie);

    NTSTATUS status = STATUS_SUCCESS;
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Connected!\n"));
	//单向发送简单字符串不需要连接上下文
    return status;
}

VOID DisconnectCallback(
    PVOID ConnectionCookie
)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Disconnected!\n"));
}

NTSTATUS MessageCallback(
    PVOID PortCookie,
    PVOID InputBuffer,  //约定L'\0'结尾
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG ReturnOutputBufferLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(PortCookie);
    //UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    //UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

    switch (*(int*)InputBuffer) //0代表命令
    {
    case 0:
    {
        struct CommandStruct* command = (struct CommandStruct*)InputBuffer;
        switch (command->Command)
        {
            case Start:
            FilterSwitch = TRUE;
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Filter Started\n"));
			break;

            case Stop:
            FilterSwitch = FALSE;
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Filter Stopped\n"));
            break;
        }
    }
        break;
    default:
    {
        struct MessageStruct* message = (struct MessageStruct*)InputBuffer;
        UNICODE_STRING Input = { (USHORT)message->length, (USHORT)InputBufferLength , message->buffer };
        status = AddPath(&Input);
        OutputBuffer = &status;
		ULONG ulSize = sizeof(NTSTATUS);
        ReturnOutputBufferLength = &ulSize;
    }
        break;
    }

    return status;
}

NTSTATUS CommunicationSetup()
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    PSECURITY_DESCRIPTOR sd;
    status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
    InitializeObjectAttributes(&oa, &PortName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, sd);

    FltCreateCommunicationPort(FilterHandle, &ServerPort, &oa, NULL, ConnectCallback, DisconnectCallback, MessageCallback, 1);

    FltFreeSecurityDescriptor(sd);

    return status;
}