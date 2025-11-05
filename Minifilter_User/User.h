#pragma once

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

typedef enum _CommandEnum
{
	Start = 1,
	Stop,
	ClearAll
}CommandEnum;
struct CommandStruct
{
	int ZeroSignal;// = 0;
	CommandEnum Command;
	WCHAR buffer[MAX_PATH];
};