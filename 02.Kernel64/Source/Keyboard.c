#include "Types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"
#include "Synchronization.h"
#include "FileSystem.h"

BOOL IsOutputBufferFull(void) {
	if(InPortByte(0x64) & 0x01) {
		return TRUE;
	}
	return FALSE;
}

BOOL IsInputBufferFull(void) {
	if(InPortByte(0x64) & 0x02) {
		return TRUE;
	}
	return FALSE;
}

BOOL WaitForACKAndPutOtherScanCode(void) {
	int i;
	int j;
	BYTE Data;	
	BOOL Result = FALSE;
	BOOL MouseData;
	for(j = 0; j < 100; j++) {
		for(i = 0; i < 0xFFFF; i++) {
			if(IsOutputBufferFull() == TRUE) {
				break;
			}
		}
		if(IsMouseDataInOutputBuffer() == TRUE) {
			MouseData = TRUE;
		}
		else {
			MouseData = FALSE;
		}
		Data = InPortByte(0x60);
		if(Data == 0xFA) {
			Result = TRUE;
			break;
		}
		else {
			if(MouseData == FALSE) {
				ConvertScanCodeAndPutQueue(Data);
			}
			else {
				AccumulateMouseDataAndPutQueue(Data);
			}
		}
	}
	return Result;
}

BOOL ActivateKeyboard(void) {
	int i;
	int j;
	BOOL PreInterrupt;
	BOOL Result;
	PreInterrupt = SetInterruptFlag(FALSE);
	OutPortByte(0x64 , 0xAE);
	for(i = 0; i < 0xFFFF; i++) {
		if(IsInputBufferFull() == FALSE) {
			break;
		}
	}
	OutPortByte(0x60 , 0xF4);
	Result = WaitForACKAndPutOtherScanCode();
	SetInterruptFlag(PreInterrupt);
	return Result;
}

BYTE GetKeyboardScanCode(void) {
	while(IsOutputBufferFull() == FALSE) {
		;
	}
	return InPortByte(0x60);
}

BYTE ChangeKeyboardLED(BOOL CapsLockOn , BOOL NumLockOn , BOOL ScrollLockOn) {
	int i;
	int j;
	BOOL PreInterrupt;
	BOOL Result;
	BYTE Data;
	PreInterrupt = SetInterruptFlag(FALSE);
	for(i = 0; i < 0xFFFF; i++) {
		if(IsInputBufferFull() == FALSE) {
			break;
		}
	}
	OutPortByte(0x60 , 0xED);
	for(i = 0; i < 0xFFFF; i++) {
		if(IsInputBufferFull() == FALSE) {
			break;
		}
	}
	Result = WaitForACKAndPutOtherScanCode();
	if(Result == FALSE) {
		SetInterruptFlag(PreInterrupt);
		return FALSE;
	}
	OutPortByte(0x60 , (CapsLockOn << 2)|(NumLockOn << 1)|ScrollLockOn);
	for(i = 0; i < 0xFFFF; i++) {
		if(IsInputBufferFull() == FALSE) {
			break;
		}
	}
	Result = WaitForACKAndPutOtherScanCode();
	SetInterruptFlag(PreInterrupt);
	return Result;
}

void EnableA20Gate(void) {
	BYTE OutputPortData;
	int i;
	OutPortByte(0x64 , 0xD0);
	for(i = 0; i < 0xFFFF; i++) {
		if(IsOutputBufferFull() == TRUE) {
			break;
		}
	}
	OutputPortData = InPortByte(0x60);
	OutputPortData |= 0x01;
	for(i = 0; i < 0xFFFF; i++) {
		if(IsInputBufferFull() == FALSE) {
			break;
		}
	}
	OutPortByte(0x64 , 0xD1);
	OutPortByte(0x60 , OutputPortData);
}

void Reboot(void) {
	FlushFileSystemCache();
	int i;
	for(i = 0; i < 0xFFFF; i++) {
		if(IsInputBufferFull() == FALSE) {
			break;
		}
	}
	OutPortByte(0x64 , 0xD1);
	OutPortByte(0x60 , 0x00);
	while(1) {
		;
	}
}

void Reboot_NFlush(void) {
	int i;
	for(i = 0; i < 0xFFFF; i++) {
		if(IsInputBufferFull() == FALSE) {
			break;
		}
	}
	OutPortByte(0x64 , 0xD1);
	OutPortByte(0x60 , 0x00);
	while(1) {
		;
	}
}

static KEYBOARDMANAGER KeyboardManager = {0 , };

static QUEUE KeyQueue;
static KEYDATA KeyQueueBuffer[KEY_MAXQUEUECOUNT];

static KEYMAPPINGENTRY KeyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] = {
	{KEY_NONE , KEY_NONE} , 
	{KEY_ESC , KEY_ESC} , 
	{'1' , '!'} , 
	{'2' , '@'} , 
	{'3' , '#'} , 
	{'4' , '$'} , 
	{'5' , '%'} , 
	{'6' , '^'} , 
	{'7' , '&'} , 
	{'8' , '*'} , 
	{'9' , '('} , 
	{'0' , ')'} , 
	{'-' , '_'} , 
	{'=' , '+'} , 
	{KEY_BACKSPACE , KEY_BACKSPACE} , 
	{KEY_TAB , KEY_TAB} , 
	{'q' , 'Q'} , 
	{'w' , 'W'} , 
	{'e' , 'E'} , 
	{'r' , 'R'} , 
	{'t' , 'T'} , 
	{'y' , 'Y'} , 
	{'u' , 'U'} , 
	{'i' , 'I'} , 
	{'o' , 'O'} , 
	{'p' , 'P'} , 
	{'[' , '{'} , 
	{']' , '}'} , 
	{'\n' , '\n'} , 
	{KEY_CTRL , KEY_CTRL} , 
	{'a' , 'A'} , 
	{'s' , 'S'} , 
	{'d' , 'D'} , 
	{'f' , 'F'} , 
	{'g' , 'G'} , 
	{'h' , 'H'} , 
	{'j' , 'J'} , 
	{'k' , 'K'} , 
	{'l' , 'L'} , 
	{';' , ':'} , 
	{'\'' , '\"'} , 
	{'`' , '~'} , 
	{KEY_LSHIFT , KEY_LSHIFT} , 
	{'\\' , '|'} , 
	{'z' , 'Z'} , 
	{'x' , 'X'} , 
	{'c' , 'C'} , 
	{'v' , 'V'} , 
	{'b' , 'B'} , 
	{'n' , 'N'} , 
	{'m' , 'M'} , 
	{',' , '<'} , 
	{'.' , '>'} , 
	{'/' , '?'} , 
	{KEY_RSHIFT , KEY_RSHIFT} , 
	{'*' , '*'} , 
	{KEY_LALT , KEY_LALT} ,
	{' ' , ' '} , 
	{KEY_CAPSLOCK , KEY_CAPSLOCK} , 
	{KEY_F1	, KEY_F1} , 
	{KEY_F2 , KEY_F2} , 
	{KEY_F3 , KEY_F3} , 
	{KEY_F4 , KEY_F4} , 
	{KEY_F5 , KEY_F5} , 
	{KEY_F6 , KEY_F6} , 
	{KEY_F7 , KEY_F7} , 
	{KEY_F8 , KEY_F8} , 
	{KEY_F9 , KEY_F9} , 
	{KEY_F10 , KEY_F10} , 
	{KEY_NUMLOCK , KEY_NUMLOCK} , 
	{KEY_SCROLLLOCK , KEY_SCROLLLOCK} ,
	{KEY_HOME , '7'} , 
	{KEY_UP , '8'} ,
	{KEY_PAGEUP , '9'} , 
	{'-' , '-'} ,
	{KEY_LEFT , '4'} , 
	{KEY_CENTER , '5'} , 
	{KEY_RIGHT , '6'} , 
	{'+' , '+'} , 
	{KEY_END , '1'} , 
	{KEY_DOWN , '2'} , 
	{KEY_PAGEDOWN , '3'} , 
	{KEY_INS , '0'} , 
	{KEY_DEL , '.'} , 
	{KEY_NONE , KEY_NONE} , 
	{KEY_NONE , KEY_NONE} , 
	{KEY_NONE , KEY_NONE} , 
	{KEY_F11 , KEY_F11} , 
	{KEY_F12 , KEY_F12}
};

BOOL IsAlphabetScanCode(BYTE ScanCode) {
	if(('a' <= KeyMappingTable[ScanCode].NormalCode) && (KeyMappingTable[ScanCode].NormalCode <= 'z')) {
		return TRUE;
	}
	return FALSE;
}

BOOL IsNumberOrSymbolScanCode(BYTE ScanCode) {
	if((2 <= ScanCode) && (ScanCode <= 53) && (IsAlphabetScanCode(ScanCode) == FALSE)) {
		return TRUE;
	}
	return FALSE;
}

BOOL IsNumberPadScanCode(BYTE ScanCode) {
	if((71 <= ScanCode) && (ScanCode <= 83)) {
		return TRUE;
	}
	return FALSE;
}

BOOL IsUseCombinedCode(BOOL ScanCode) {
	BYTE DownScanCode;
	BOOL UseCombinedKey;
	DownScanCode = ScanCode & 0x7F;
	if(IsAlphabetScanCode(DownScanCode) == TRUE) {
		if(KeyboardManager.ShiftDown ^ KeyboardManager.CapsLockOn) {
			UseCombinedKey = TRUE;
		}
		else {
			UseCombinedKey = FALSE;
		}
	}
	else if(IsNumberOrSymbolScanCode(DownScanCode) == TRUE) {
		if(KeyboardManager.ShiftDown == TRUE) {
			UseCombinedKey = TRUE;
		}
		else {
			UseCombinedKey = FALSE;
		}
	}
	else if((IsNumberPadScanCode(DownScanCode) == TRUE) && (KeyboardManager.ExtendedCodeIn == FALSE)) {
		if(KeyboardManager.NumLockOn == TRUE) {
			UseCombinedKey = TRUE;
		}
		else {
			UseCombinedKey = FALSE;
		}
	}
	return UseCombinedKey;
}

void UpdateCombiationKeyStatusAndLED(BYTE ScanCode) {
	BOOL Down;
	BYTE DownScanCode;
	BOOL LEDStatusChanged = FALSE;
	if(ScanCode & 0x80) {
		Down = FALSE;
		DownScanCode = ScanCode & 0x7F;
	}
	else {
		Down = TRUE;
		DownScanCode = ScanCode;
	}
	if((DownScanCode == 42)||(DownScanCode == 54)) {
		KeyboardManager.ShiftDown = Down;
	}
	else if((DownScanCode == 58) && (Down == TRUE)) {
		KeyboardManager.CapsLockOn ^= TRUE;
		LEDStatusChanged = TRUE;
	}
	else if((DownScanCode == 69) && (Down == TRUE)) {
		KeyboardManager.NumLockOn ^= TRUE;
		LEDStatusChanged = TRUE;
	}
	else if((DownScanCode == 70) && (Down == TRUE)) {
		KeyboardManager.ScrollLockOn ^= TRUE;
		LEDStatusChanged = TRUE;
	}
	if(LEDStatusChanged == TRUE) {
		ChangeKeyboardLED(KeyboardManager.CapsLockOn , KeyboardManager.NumLockOn , KeyboardManager.ScrollLockOn);
	}
}

BOOL ConvertScanCodeToASCIICode(BYTE ScanCode , BYTE *ASCIICode , BOOL *Flags) {
	BOOL UseCombinedKey;
	if(KeyboardManager.SkipCountForPause > 0) {
		KeyboardManager.SkipCountForPause--;
		return FALSE;
	}
	if(ScanCode == 0xE1) {
		*ASCIICode = KEY_PAUSE;
		*Flags = KEY_FLAGS_DOWN;
		KeyboardManager.SkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
		return TRUE;
	}
	else if(ScanCode == 0xE0) {
		KeyboardManager.ExtendedCodeIn = TRUE;
		return FALSE;
	}
	UseCombinedKey = IsUseCombinedCode(ScanCode);
	if(UseCombinedKey == TRUE) {
		*ASCIICode = KeyMappingTable[ScanCode & 0x7F].CombinedCode;
	}
	else {
		*ASCIICode = KeyMappingTable[ScanCode & 0x7F].NormalCode;
	}
	if(KeyboardManager.ExtendedCodeIn == TRUE) {
		*Flags = KEY_FLAGS_EXTENDEDKEY;
		KeyboardManager.ExtendedCodeIn = FALSE;
	}
	else {
		*Flags = 0;
	}
	if((ScanCode & 0x80) == 0) {
		*Flags |= KEY_FLAGS_DOWN;
	}
	UpdateCombiationKeyStatusAndLED(ScanCode);
	return TRUE;
}

BOOL InitKeyboard(void) {
	InitQueue(&KeyQueue , KeyQueueBuffer , KEY_MAXQUEUECOUNT , sizeof(KEYDATA));
	InitSpinLock(&(KeyboardManager.SpinLock));
	return ActivateKeyboard();
}

BOOL ConvertScanCodeAndPutQueue(BYTE ScanCode) {
	KEYDATA Data;
	BOOL Result = FALSE;
	Data.ScanCode = ScanCode;
	if((ConvertScanCodeToASCIICode(ScanCode , &(Data.ASCIICode) , &(Data.Flags))) == TRUE) {
		LockForSpinLock(&(KeyboardManager.SpinLock));
		Result = PutQueue(&KeyQueue , &Data);
		UnLockForSpinLock(&(KeyboardManager.SpinLock));
	}
	return Result;
}

BOOL GetKeyFromKeyQueue(KEYDATA *Data) {
	BOOL Result;
	LockForSpinLock(&(KeyboardManager.SpinLock));
	Result = GetQueue(&KeyQueue , Data);
	UnLockForSpinLock(&(KeyboardManager.SpinLock));
	return Result;
}
