/**
 * @file waveshare_PCF85063.c
 * @brief PCF85063 Real-Time Clock driver for Waveshare modules
 * @author Waveshare team
 * @version 1.0
 * @date 2021-02-02
 *
 * This file provides functions to interface with the PCF85063 RTC chip,
 * including time setting, reading, alarm management, and epoch conversion.
 */
#include "DEV_Config.h"
#include "waveshare_PCF85063.h"
#include <string.h>
#include <stdio.h>


/**
 * @brief Read one byte of data from the PCF85063 via I2C
 * @param Addr Register address to read from
 * @return The byte value read from the register
 */
static UBYTE PCF85063_Read_Byte(UBYTE Addr)
{
	return I2C_Read_Byte(Addr);
}

/**
 * @brief Send one byte of data to the PCF85063 via I2C
 * @param Addr Register address to write to
 * @param Value Value to write to the register
 */
static void PCF85063_Write_Byte(UBYTE Addr, UBYTE Value)
{
	I2C_Write_Byte(Addr, Value);
}

/**
 * @brief Convert a decimal value to BCD (Binary Coded Decimal)
 * @param val Decimal value to convert (0-99)
 * @return BCD representation of the input value
 */
int DecToBcd(int val)
{
	return ((val/10)*16 + (val%10)); 
}

/**
 * @brief Convert a BCD (Binary Coded Decimal) value to decimal
 * @param val BCD value to convert
 * @return Decimal representation of the input BCD value
 */
int BcdToDec(int val)
{
	return ((val/16)*10 + (val%16));
}

/**
 * @brief Initialize the PCF85063 RTC chip
 * This function sets up the RTC, stops the clock, clears the stop bit, and ensures clock stability
 * Date and time will be set to: January 1, 2000, 00:00:00.
 */
void PCF85063_init()
{
	int inspect = 0;
	PCF85063_Write_Byte(CONTROL_1_REG,0x58);
	DEV_Delay_ms(500);
	PCF85063_Write_Byte(SECONDS_REG,PCF85063_Read_Byte(SECONDS_REG)|0x80);
	PCF85063_Write_Byte(CONTROL_2_REG,0x80);
	while(1)
	{
		PCF85063_Write_Byte(SECONDS_REG,PCF85063_Read_Byte(SECONDS_REG)&0x7F);

		if((PCF85063_Read_Byte(SECONDS_REG)&0x80) == 0)
		    break;
		DEV_Delay_ms(500);
		inspect  = inspect+1;
		if(inspect>5)
		{
			printf("Clock stability unknown\r\n");
			break;
		}
	}
}

/**
 * @brief Set the date (Year, Month, Day) in the PCF85063 RTC
 * @param Years Year value (0-99, representing 2000-2099)
 * @param Months Month value (1-12)
 * @param Days Day value (1-31)
 */
void PCF85063_SetTime_YMD(int Years,int Months,int Days)
{
	if(Years>99)
		Years = 99;
	if(Months>12)
		Months = 12;
	if(Days>31)
		Days = 31;	
	PCF85063_Write_Byte(YEARS_REG  ,DecToBcd(Years));
	PCF85063_Write_Byte(MONTHS_REG ,DecToBcd(Months)&0x1F);
	PCF85063_Write_Byte(DAYS_REG   ,DecToBcd(Days)&0x3F);
}

/**
 * @brief Set the time (Hour, Minute, Second) in the PCF85063 RTC
 * @param hour Hour value (0-23)
 * @param minute Minute value (0-59)
 * @param second Second value (0-59)
 */
void PCF85063_SetTime_HMS(int hour,int minute,int second)
{
	if(hour>23)
		hour = 23;
	if(minute>59)
		minute = 59;
	if(second>59)
		second = 59;
	PCF85063_Write_Byte(HOURS_REG  ,DecToBcd(hour)&0x3F);
	PCF85063_Write_Byte(MINUTES_REG,DecToBcd(minute)&0x7F);
	PCF85063_Write_Byte(SECONDS_REG,DecToBcd(second)&0x7F);
}

/**
 * @brief Check if a given year is a leap year
 * @param year Year to check (full year, e.g., 2023)
 * @return 1 if leap year, 0 otherwise
 */
static int isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * @brief Get the number of days in a given month of a year
 * @param year Year (full year)
 * @param month Month (1-12)
 * @return Number of days in the month
 */
static int daysInMonth(int year, int month)
{
    static const int dim[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && isLeapYear(year)) return 29;
    return dim[month];
}

/**
 * @brief Convert a Time_data structure to Unix epoch time
 * @param t Pointer to Time_data structure containing date and time
 * @return Unix epoch time (seconds since 1970-01-01 00:00:00 UTC)
 */
uint64_t Time_dataToEpoch(const Time_data *t)
{
    // Year stored as years since 2000 (0..99). Convert to full YYYY.
    int year = t->years + 2000;
    int month = t->months;
    int day = t->days;
    int hour = t->hours;
    int min = t->minutes;
    int sec = t->seconds;

    // Days since 1970-01-01
    uint64_t days = 0;
    for (int y = 1970; y < year; y++) {
        days += isLeapYear(y) ? 366 : 365;
    }
    for (int m = 1; m < month; m++) {
        days += daysInMonth(year, m);
    }
    days += (day - 1);

    return days * 86400ULL + (uint64_t)hour * 3600ULL + (uint64_t)min * 60ULL + (uint64_t)sec;
}

/**
 * @brief Get the current time from the PCF85063 RTC
 * @return Time_data structure containing the current date and time
 */
Time_data PCF85063_GetTime()
{
	Time_data time;
	time.years = BcdToDec(PCF85063_Read_Byte(YEARS_REG));
	time.months = BcdToDec(PCF85063_Read_Byte(MONTHS_REG)&0x1F);
	time.days = BcdToDec(PCF85063_Read_Byte(DAYS_REG)&0x3F);
	time.hours = BcdToDec(PCF85063_Read_Byte(HOURS_REG)&0x3F);
	time.minutes = BcdToDec(PCF85063_Read_Byte(MINUTES_REG)&0x7F);
	time.seconds = BcdToDec(PCF85063_Read_Byte(SECONDS_REG)&0x7F);
	return time;
}

/**
 * @brief Convert Unix epoch time to Time_data structure
 * @param epoch Unix epoch time (seconds since 1970-01-01 00:00:00 UTC)
 * @param t Pointer to Time_data structure to fill with converted date and time
 */
static void EpochToTime_data(uint64_t epoch, Time_data *t)
{
    uint64_t seconds = epoch;

    t->seconds = seconds % 60;
    seconds /= 60;

    t->minutes = seconds % 60;
    seconds /= 60;

    t->hours = seconds % 24;
    uint64_t days = seconds / 24;

    int year = 1970;
    while (1) {
        int yearDays = isLeapYear(year) ? 366 : 365;
        if (days >= (uint64_t)yearDays) {
            days -= yearDays;
            year++;
        } else {
            break;
        }
    }

    int month = 1;
    while (1) {
        int mdays = daysInMonth(year, month);
        if (days >= (uint64_t)mdays) {
            days -= mdays;
            month++;
        } else {
            break;
        }
    }

    t->years = (uint16_t)(year - 2000);
    t->months = (uint16_t)month;
    t->days = (uint16_t)(days + 1);
}

/**
 * @brief Set the time in the PCF85063 RTC using Unix epoch time
 * @param epoch Unix epoch time (seconds since 1970-01-01 00:00:00 UTC)
 */
void PCF85063_SetTime_Epoch(uint64_t epoch)
{
    Time_data t;
    EpochToTime_data(epoch, &t);
    PCF85063_SetTime_YMD(t.years, t.months, t.days);
    PCF85063_SetTime_HMS(t.hours, t.minutes, t.seconds);
}

uint64_t PCF85063_GetEpochTime(void)
{
    Time_data t = PCF85063_GetTime();
    return Time_dataToEpoch(&t);
}

void PCF85063_alarm_Time_Enabled(Time_data time)
{
    if(time.seconds>59)
    {
        time.seconds = time.seconds - 60;
        time.minutes = time.minutes + 1;
    }
    if(time.minutes>59)
    {
        time.minutes = time.minutes - 60;
        time.hours = time.hours + 1;
    }
    if(time.hours>23)
    {
        time.hours = time.hours - 24;
        time.days = time.days + 1;
    }
    if(time.months == 1 || time.months == 3 || time.months == 5 || time.months == 7 || time.months == 8 || time.months == 10 || time.months == 12)
    {
        if(time.days>31)
        {
            time.days = time.days - 31;
        }
    }
    else if(time.months == 2)
    {
        if(time.years%4==0)
        {
            if(time.days>29)
            {
                time.days = time.days - 29;
            }
        }
        else
        {
            if(time.days>28)
            {
                time.days = time.days - 28;
            }
        }
    }
    else
    {
        if(time.days>30)
        {
            time.days = time.days - 30;
        }
    }
    // printf("%d-%d-%d %d:%d:%d\r\n",time.years,time.months,time.days,time.hours,time.minutes,time.seconds);
	PCF85063_Write_Byte(CONTROL_2_REG, PCF85063_Read_Byte(CONTROL_2_REG)|0x80);	// Alarm on
	PCF85063_Write_Byte(DAY_ALARM_REG, DecToBcd(time.days) & 0x7F);
    PCF85063_Write_Byte(HOUR_ARARM_REG, DecToBcd(time.hours) & 0x7F);
	PCF85063_Write_Byte(MINUTES_ALARM_REG, DecToBcd(time.minutes) & 0x7F);
	PCF85063_Write_Byte(SECOND_ALARM_REG, DecToBcd(time.seconds) & 0x7F);
}

/**
 * @brief Enable alarm at a specific Unix epoch time
 * @param epoch Unix epoch time (seconds since 1970-01-01 00:00:00 UTC) for the alarm
 */
void PCF85063_alarm_Epoch_Enabled(uint64_t epoch)
{
    Time_data time;
    EpochToTime_data(epoch, &time);

    if(time.seconds>59)
    {
        time.seconds = time.seconds - 60;
        time.minutes = time.minutes + 1;
    }
    if(time.minutes>59)
    {
        time.minutes = time.minutes - 60;
        time.hours = time.hours + 1;
    }
    if(time.hours>23)
    {
        time.hours = time.hours - 24;
        time.days = time.days + 1;
    }
    if(time.months == 1 || time.months == 3 || time.months == 5 || time.months == 7 || time.months == 8 || time.months == 10 || time.months == 12)
    {
        if(time.days>31)
        {
            time.days = time.days - 31;
        }
    }
    else if(time.months == 2)
    {
        if(time.years%4==0)
        {
            if(time.days>29)
            {
                time.days = time.days - 29;
            }
        }
        else
        {
            if(time.days>28)
            {
                time.days = time.days - 28;
            }
        }
    }
    else
    {
        if(time.days>30)
        {
            time.days = time.days - 30;
        }
    }
    // printf("%d-%d-%d %d:%d:%d\r\n",time.years,time.months,time.days,time.hours,time.minutes,time.seconds);
	PCF85063_Write_Byte(CONTROL_2_REG, PCF85063_Read_Byte(CONTROL_2_REG)|0x80);	// Alarm on
	PCF85063_Write_Byte(DAY_ALARM_REG, DecToBcd(time.days) & 0x7F);
    PCF85063_Write_Byte(HOUR_ARARM_REG, DecToBcd(time.hours) & 0x7F);
	PCF85063_Write_Byte(MINUTES_ALARM_REG, DecToBcd(time.minutes) & 0x7F);
	PCF85063_Write_Byte(SECOND_ALARM_REG, DecToBcd(time.seconds) & 0x7F);
}

void PCF85063_alarm_Time_Disable() 
{
	PCF85063_Write_Byte(HOUR_ARARM_REG   ,PCF85063_Read_Byte(HOUR_ARARM_REG)|0x80);
	PCF85063_Write_Byte(MINUTES_ALARM_REG,PCF85063_Read_Byte(MINUTES_ALARM_REG)|0x80);
	PCF85063_Write_Byte(SECOND_ALARM_REG ,PCF85063_Read_Byte(SECOND_ALARM_REG)|0x80);
	PCF85063_Write_Byte(DAY_ALARM_REG, PCF85063_Read_Byte(DAY_ALARM_REG)|0x80);
    PCF85063_Write_Byte(CONTROL_2_REG   ,PCF85063_Read_Byte(CONTROL_2_REG)&0x7F);	// Alarm OFF
}

int PCF85063_get_alarm_flag()
{
	if(PCF85063_Read_Byte(CONTROL_2_REG)&0x40 == 0x40)
		return 1;
	else 
		return 0;
}

void PCF85063_clear_alarm_flag()
{
	PCF85063_Write_Byte(CONTROL_2_REG   ,PCF85063_Read_Byte(CONTROL_2_REG)&0xBF);
}

void PCF85063_test()
{
    int count = 0;
	
	// PCF85063_SetTime_YMD(21,2,28);
	// PCF85063_SetTime_HMS(23,59,58);
	while(1)
	{
		Time_data T;
		T = PCF85063_GetTime();
		printf("%d-%d-%d %d:%d:%d\r\n",T.years,T.months,T.days,T.hours,T.minutes,T.seconds);
		count+=1;
		DEV_Delay_ms(1000);
		if(count>20)
		    break;
	}
}

/**
 * @brief Print the current RTC time in both human-readable and epoch formats
 * Outputs the current date/time as YYYY-MM-DD HH:MM:SS and as Unix epoch time
 */
void PCF85063_PrintCurrentTime(void)
{
    Time_data t = PCF85063_GetTime();
    uint64_t epoch = PCF85063_GetEpochTime();
    
    printf("Current Time: %04d-%02d-%02d %02d:%02d:%02d (Epoch: %llu)\r\n",
           t.years + 2000, t.months, t.days, t.hours, t.minutes, t.seconds, epoch);
}

void rtcRunAlarm(uint64_t uiAlarmInterval)
{
    uint64_t currentEpoch = PCF85063_GetEpochTime();

    printf("Current RTC epoch time: %llu\r\n", currentEpoch);

    if (parseBuildDateTimeEpoch() > currentEpoch)
    {
        PCF85063_init();    // RTC init

        // If the current RTC time is before the build time, set it to the build time
        PCF85063_SetTime_Epoch(parseBuildDateTimeEpoch());

        printf("RTC time was before build time. Init the RTC and set it to build epoch time: %llu\r\n", parseBuildDateTimeEpoch());
    }   

    currentEpoch = PCF85063_GetEpochTime();
    uint64_t alarmEpoch = currentEpoch + uiAlarmInterval;  // Set alarm for uiAlarmInterval seconds in the future

    printf("Setting RTC alarm for epoch time: %llu (in %llu seconds)\r\n", alarmEpoch, uiAlarmInterval);
    PCF85063_alarm_Epoch_Enabled(alarmEpoch);
}

/**
 * @brief Parse the build date and time from compiler macros to initialize RTC
 * @return Time_data structure containing the parsed build date and time
 */
Time_data parseBuildDateTime(void)
{
    Time_data time = {0};
    const char* date = __DATE__;  // Format: "Mmm dd yyyy"
    const char* time_str = __TIME__;  // Format: "hh:mm:ss"
    
    // Parse month from date string
    char month_str[4] = {0};
    memcpy(month_str, date, 3);
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < 12; i++) {
        if (strcmp(month_str, months[i]) == 0) {
            time.months = i + 1;
            break;
        }
    }
    
    // Parse day and year from date string
    sscanf(date + 4, "%hhu %hu", &time.days, &time.years);
    time.years -= 2000;  // Convert to years since 2000
    
    // Parse time string
    sscanf(time_str, "%hhu:%hhu:%hhu", &time.hours, &time.minutes, &time.seconds);
    
    return time;
}

/**
 * @brief Parse the build date and time from compiler macros and return as Unix epoch time
 * @return Unix epoch time (seconds since 1970-01-01 00:00:00 UTC) for the build date and time
 */
uint64_t parseBuildDateTimeEpoch(void)
{
    Time_data t = parseBuildDateTime();
    return Time_dataToEpoch(&t);
}

