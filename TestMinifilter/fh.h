#pragma once

#include <fltKernel.h>
#include <ntddk.h>
#include<ntstrsafe.h>

#define MAX_PATH 260

extern PFLT_FILTER FilterHandle;

extern UNICODE_STRING PortName;
extern PFLT_PORT ServerPort;

extern BOOLEAN FilterSwitch;

NTSTATUS AddPath(PUNICODE_STRING FilePath);

NTSTATUS CommunicationSetup();

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

struct MessageStruct
{
	int length;
	WCHAR buffer[MAX_PATH];
};

typedef struct _DosNode {	//从C:开始
	//UCHAR tag;//卷标
	UNICODE_STRING ParentDir;
	UNICODE_STRING FileName;
	struct _DosNode* Next;
}DosNode, * PDosNode;

typedef struct _DosList {	//从C:开始，为每个盼复单独建立一个列表（因为其对应符号链接不同）
	DosNode* Head;
	DosNode* Tail;
	int num;
}DosList, * PDosList;


typedef struct _PathStruct {
	
	struct _DosList DosList[26];
	USHORT SubToDos[64];	//From NTSub to DosSub
}PathStruct, * PPathStruct;

PathStruct ps;