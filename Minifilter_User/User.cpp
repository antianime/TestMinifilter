#include<Windows.h>
#include <FltUser.h>
#include<iostream>
#pragma comment(lib, "FltLib.lib")

#define MAX_PATH 260

WCHAR PortName[] = L"\\Port_nb666";

struct MessageStruct
{
	int length;
	WCHAR buffer[MAX_PATH];
};

struct CommandStruct
{
	int ZeroSignal = 0;
	enum CommandEnum
	{
		Start = 1,
		Stop = 2,
		ClearAll = 3
	};
	WCHAR buffer[MAX_PATH];
};

int main()
{
	
	HRESULT hResult;
	HANDLE hPort;
	DWORD dwSize, dwResult;
	struct MessageStruct message;
	ZeroMemory(&message, sizeof(MessageStruct));
	std::cout << "Input the full path of the file you want to hide (e.g., C:\\path\\to\\file.txt)" << std::endl;
	std::wstring input;
	std::wcin >> input;
	message.length = input.length() * sizeof(WCHAR);//×Ö½ÚÊý£¬ÊÊÓ¦UNICODE_STRING
	memmove(message.buffer, input.c_str(), message.length);
	
	hResult = FilterConnectCommunicationPort(PortName, FLT_PORT_FLAG_SYNC_HANDLE, NULL, 0, NULL, &hPort);
	if (hResult != S_OK)
		std::cout << "Connect fail: " << std::hex << hResult << std::endl;

	hResult = FilterSendMessage(hPort, &message, sizeof(struct MessageStruct), &dwResult, sizeof(dwResult), &dwSize);
	if (hResult != S_OK)
	{
		std::cout << "Send Message fail: " << std::hex << hResult << std::endl;
		//return -1;
	}

	switch (dwResult)
	{
	case 0:
		std::cout << "Add path success!" << std::endl;
		break;
	case 1:
		std::cout << "Add path FAILED!" << std::endl;
		break;
	}

	
	return 0;
}