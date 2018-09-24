#include "SerialPort.h"
#include "Utility.h"

static SERIALMANAGER SerialManager;

void InitSerialPort(void) {
	WORD PortBaseAddress;
	InitMutex(&(SerialManager.Lock));
	PortBaseAddress = SERIAL_PORT_COM1;
	OutPortByte(PortBaseAddress+SERIAL_PORT_INDEX_INTERRUPTENABLE , 0);
	OutPortByte(PortBaseAddress+SERIAL_PORT_INDEX_DIVISORLATCHMSB , SERIAL_DIVISORLATCH_115200);
	OutPortByte(PortBaseAddress+SERIAL_PORT_INDEX_DIVISORLATCHMSB , SERIAL_DIVISORLATCH_115200 >> 8);
	OutPortByte(PortBaseAddress+SERIAL_PORT_INDEX_LINECONTROL , SERIAL_LINECONTROL_8BIT|SERIAL_LINECONTROL_NOPARITY|SERIAL_LINECONTROL_1BITSTOP);
	OutPortByte(PortBaseAddress+SERIAL_PORT_INDEX_FIFOCONTROL , SERIAL_FIFOCONTROL_FIFOENABLE|SERIAL_FIFOCONTROL_14BYTEFIFO);
}

static BOOL	IsSerialTransmitterBufferEmpty(void) {
	BYTE Data;
	Data = InPortByte(SERIAL_PORT_COM1+SERIAL_PORT_INDEX_LINESTATUS);
	if((Data & SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) == SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) {
		return TRUE;
	}
	return FALSE;
}

void SendSerialData(BYTE *Buffer , int Size) {
	int SentByte;
	int TempSize;
	int j;
	Lock(&(SerialManager.Lock));
	while(SentByte < Size) {
		while(IsSerialTransmitterBufferEmpty() == FALSE) {
			Sleep(0);
		}
		TempSize = MIN(Size-SentByte , SERIAL_FIFOMAXSIZE);
		for(j = 0; j < TempSize; j++) {
			OutPortByte(SERIAL_PORT_COM1+SERIAL_PORT_INDEX_TRANSMITBUFFER , Buffer[SentByte+j]);
		}
		SentByte += TempSize;
	}
	UnLock(&(SerialManager.Lock));
}

static BOOL IsSerialReceiveBufferFull(void) {
	BYTE Data;
	Data = InPortByte(SERIAL_PORT_COM1+SERIAL_PORT_INDEX_LINESTATUS);
	if((Data & SERIAL_LINESTATUS_RECEIVEDDATAREADY) == SERIAL_LINESTATUS_RECEIVEDDATAREADY) {
		return TRUE;
	}
	return FALSE;
}

int ReceiveSerialData(BYTE *Buffer , int Size) {
	int i;
	Lock(&(SerialManager.Lock));
	for(i = 0; i < Size; i++) {
		if(IsSerialReceiveBufferFull() == FALSE) {
			break;
		}
		Buffer[i] = InPortByte(SERIAL_PORT_COM1+SERIAL_PORT_INDEX_RECEIVEBUFFER);
	}
	UnLock(&(SerialManager.Lock));
	return i;
}

void ClearSerialFIFO(void) {
	Lock(&(SerialManager.Lock));
	OutPortByte(SERIAL_PORT_COM1+SERIAL_PORT_INDEX_FIFOCONTROL , SERIAL_FIFOCONTROL_FIFOENABLE|SERIAL_FIFOCONTROL_14BYTEFIFO|SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO|SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO);
	UnLock(&(SerialManager.Lock));
}
