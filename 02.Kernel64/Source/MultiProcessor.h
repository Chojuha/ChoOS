#ifndef _MULTIPROCESSOR_H_
#define _MULTIPROCESSOR_H_

#include "Types.h"
#include "Colors.h"

#define BOOTSTRAPPROCESSOR_FLAGADDRESS 0x7C09
#define MAXPROCESSORCOUNT 16

BOOL StartUpApplicationProcessor(CONSOLECOLOR Color);
static BOOL WakeUpApplicationProcessor(void);
BYTE GetAPICID(void);

#endif
