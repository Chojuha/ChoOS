#ifndef _CONSOLESHELL_H_
#define _CONSOLESHELL_H_

#include "Types.h"
#include "Colors.h"

#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT 500
#define CONSOLESHELL_PROMPTMESSAGE "~$>"

typedef void(*CommandFunction)(CONSOLECOLOR Color , const char *ParameterBuffer);

#pragma pack(push , 1)

typedef struct ShellCommandEntryStruct {
	char *Command;
	char *Help;
	CommandFunction Function;
}SHELLCOMMANDENTRY;

typedef struct ParameterListStruct {
	const char *Buffer;
	int Length;
	int CurrentPosition;
}PARAMETERLIST;

#pragma pack(pop)

void TimeTask(void);
void StartConsoleShell(CONSOLECOLOR Color);
void ExecuteCommand(CONSOLECOLOR Color , const char *CommandBuffer);
void InitParameter(PARAMETERLIST *List , const char *Parameter);
int GetNextParameter(PARAMETERLIST *List , char *Parameter);
static void PrtStartConsoleShellMessage(CONSOLECOLOR Color);
static void Help(CONSOLECOLOR Color , const char *ParameterBuffer);
static void Clrscr(CONSOLECOLOR Color , const char *ParameterBuffer);
static void ShowSystemInformation(CONSOLECOLOR Color , const char *ParameterBuffer);
static void Wait(CONSOLECOLOR Color , const char *ParameterBuffer);
static void ShutdownChoOS(CONSOLECOLOR Color , const char *ParameterBuffer);
static void ShowDateAndTime(CONSOLECOLOR Color , const char *ParameterBuffer);
static void Echo(CONSOLECOLOR Color , const char *ParameterBuffer);
static void CreateTestTask(CONSOLECOLOR Color , const char *ParameterBuffer);
static void KillTask(CONSOLECOLOR Color , const char *ParameterBuffer);
static void TaskList(CONSOLECOLOR Color , const char *ParameterBuffer);
static void PrintRandomNumber(CONSOLECOLOR Color , const char *ParameterBuffer);
static void ShowCPUInformation(CONSOLECOLOR Color , const char *ParameterBuffer);
static void ShowProcessLoad(CONSOLECOLOR Color , const char *ParameterBuffer);
static void FormatHDDInConsole(CONSOLECOLOR Color , const char *ParameterBuffer);
static void MountHDDInConsole(CONSOLECOLOR Color , const char *ParameterBuffer);
static void DeleteFileInConsole(CONSOLECOLOR Color , const char *ParameterBuffer);
static void ShowRootDirectory(CONSOLECOLOR Color , const char *ParameterBuffer);
static void DisableCommandMessage(CONSOLECOLOR Color , const char *ParameterBuffer);
static void EnableCommandMessage(CONSOLECOLOR Color , const char *ParameterBuffer);
static void CheckCommandMessage(CONSOLECOLOR Color , const char *ParameterBuffer);
static void FallingMarble(CONSOLECOLOR Color , const char *ParameterBuffer);
static void Pause_Console(CONSOLECOLOR Color , const char *ParameterBuffer);
static void Ver(CONSOLECOLOR Color , const char *ParameterBuffer);
static void Calculater(CONSOLECOLOR Color , const char *ParameterBuffer);
static void ASCIIList(CONSOLECOLOR Color , const char *ParameterBuffer);
static void CheckFileSystem(CONSOLECOLOR Color , const char *ParameterBuffer);
static void ReadFileInConsole(CONSOLECOLOR Color , const char *ParameterBuffer);
static void CreateFileInConsole(CONSOLECOLOR Color , const char *ParameterBuffer);
static void ShowMPConfigurationTable(CONSOLECOLOR Color , const char *ParameterBuffer);
static void StartApplicationProcessor(CONSOLECOLOR Color , const char *ParameterBuffer);
static void SaveF(CONSOLECOLOR Color , const char *ParameterBuffer);
static void DownloadFileInSerialPort(CONSOLECOLOR Color , const char *ParameterBuffer);
static void CopyFileInConsole(CONSOLECOLOR Color , const char *ParameterBuffer);
static void ExecuteConsoleShellScript(CONSOLECOLOR Color , const char *ParameterBuffer);
static void Comments(CONSOLECOLOR Color , const char *ParameterBuffer);
static void Edit(CONSOLECOLOR Color , const char *ParameterBuffer);
static void QTST(CONSOLECOLOR Color , const char *ParameterBuffer);

#endif
