#ifndef _MODESWITCH_H_
#define _MODESWITCH_H_

#include "Types.h"

void kReadCPUID(DWORD dwEAX , DWORD *pdwEAX , DWORD *pdwEBX , DWORD *pdwECX , DWORD *pdwEDX);
void kSwitchAndExecute64bitKernel(void);

#endif
