#ifndef _ASSEMBLYUTILITY_H_
#define _ASSEMBLYUTILITY_H_

#include "Types.h"
#include "Task.h"

BYTE InPortByte(WORD Port);
void OutPortByte(WORD Port , BYTE Data);
WORD InPortWord(WORD Port);
void OutPortWord(WORD Port , WORD Data);
void LoadGDTR(QWORD GDTRAddress);
void LoadTR(WORD TSSSegmentOffset);
void LoadIDTR(QWORD IDTRAddress);
void EnableInterrupt(void);
void DisableInterrupt(void);
QWORD ReadRFLAGS(void);
QWORD ReadTSC(void);
void SwitchContext(CONTEXT *CurrentContext , CONTEXT *NextContext);
void Hlt(void);
BOOL TestAndSet(volatile BYTE *Destiantion , BYTE Compare , BYTE Source);
void InitFPU(void);
void SaveFPUContext(void *FPUContext);
void LoadFPUContext(void *FPUContext);
void SetTS(void);
void ClearTS(void);
void EnableGlobalLocalAPIC(void);
void Pause(void);

#endif
