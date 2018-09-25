#ifndef _RTC_H_
#define _RTC_H_

#include "Types.h"

#define RTC_CMOSADDRESS 0x70
#define RTC_CMOSDATA 0x71
#define RTC_ADDRESS_SECOND 0x00
#define RTC_ADDRESS_MINUTE 0x02
#define RTC_ADDRESS_HOUR 0x04
#define RTC_ADDRESS_DAYOFWEEK 0x06
#define RTC_ADDRESS_DAYOFMONTH 0x07
#define RTC_ADDRESS_MONTH 0x08
#define RTC_ADDRESS_YEAR 0x09
#define RTC_BCDTOBINARY(x) ((((x) >> 4)*10) + ((x) & 0x0F))

void ReadRTCTimer(BYTE *Hour , BYTE *Minute , BYTE *Second);
void ReadRTCDate(WORD *Year , BYTE *Month , BYTE *DayOfMonth , BYTE *DayOfWeek);
char *ConvertDayOfWeekToString(BYTE DayOfWeek);

#endif
