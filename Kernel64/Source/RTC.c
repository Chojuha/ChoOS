#include "RTC.h"

void ReadRTCTimer(BYTE *Hour , BYTE *Minute , BYTE *Second) {
	BYTE Data;
	OutPortByte(RTC_CMOSADDRESS , RTC_ADDRESS_HOUR);
	Data = InPortByte(RTC_CMOSDATA);
	*Hour = RTC_BCDTOBINARY(Data);
	OutPortByte(RTC_CMOSADDRESS , RTC_ADDRESS_MINUTE);
	Data = InPortByte(RTC_CMOSDATA);
	*Minute = RTC_BCDTOBINARY(Data);
	OutPortByte(RTC_CMOSADDRESS , RTC_ADDRESS_SECOND);
	Data = InPortByte(RTC_CMOSDATA);
	*Second = RTC_BCDTOBINARY(Data);
}

void ReadRTCDate(WORD *Year , BYTE *Month , BYTE *DayOfMonth , BYTE *DayOfWeek) {
	BYTE Data;
	OutPortByte(RTC_CMOSADDRESS , RTC_ADDRESS_YEAR);
	Data = InPortByte(RTC_CMOSDATA);
	*Year = RTC_BCDTOBINARY(Data)+2000;
	OutPortByte(RTC_CMOSADDRESS , RTC_ADDRESS_MONTH);
	Data = InPortByte(RTC_CMOSDATA);
	*Month = RTC_BCDTOBINARY(Data);
	OutPortByte(RTC_CMOSADDRESS , RTC_ADDRESS_DAYOFMONTH);
	Data = InPortByte(RTC_CMOSDATA);
	*DayOfMonth = RTC_BCDTOBINARY(Data);
	OutPortByte(RTC_CMOSADDRESS , RTC_ADDRESS_DAYOFWEEK);
	Data = InPortByte(RTC_CMOSDATA);
	*DayOfWeek = RTC_BCDTOBINARY(Data);
}

char *ConvertDayOfWeekToString(BYTE DayOfWeek) {
	static char *DayOfWeekString[8] = {"     " , "Sunday" , "Monday" , "Tuesday" , "Wednesday" , "Tursday" , "Friday" , "Saturday"};
	if(DayOfWeek >= 8) {
		return DayOfWeekString[0];
	}
	return DayOfWeekString[DayOfWeek];
}
