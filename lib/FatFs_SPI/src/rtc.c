/* rtc.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use 
this file except in compliance with the License. You may obtain a copy of the 
License at

   http://www.apache.org/licenses/LICENSE-2.0 
Unless required by applicable law or agreed to in writing, software distributed 
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR 
CONDITIONS OF ANY KIND, either express or implied. See the License for the 
specific language governing permissions and limitations under the License.
*/
#include <stdio.h>
#include <time.h>
//
//#include "hardware/rtc.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/types.h"
#include "pico/util/datetime.h"

//
#include "ff.h"
#include "util.h"  // calculate_checksum
//
#include "rtc.h"

#define UWORD WORD

typedef struct{
  UWORD years;
  UWORD months;
  UWORD days;
  UWORD hours;
  UWORD minutes;
  UWORD seconds;
}Time_data;

extern Time_data PCF85063_GetTime(); // Forward declaration for get_fattime() to avoid include mess

// Called by FatFs:
DWORD get_fattime(void) {
    Time_data t = PCF85063_GetTime();
    uint32_t year = (uint32_t)t.years + 2000u;
    if (year < 1980u) {
        return 0;
    }

    DWORD fattime = 0;
    fattime |= ((DWORD)((year - 1980u) & 0x7Fu)) << 25;
    fattime |= ((DWORD)(t.months & 0x0Fu)) << 21;
    fattime |= ((DWORD)(t.days & 0x1Fu)) << 16;
    fattime |= ((DWORD)(t.hours & 0x1Fu)) << 11;
    fattime |= ((DWORD)(t.minutes & 0x3Fu)) << 5;
    fattime |= ((DWORD)((t.seconds / 2u) & 0x1Fu));
    return fattime;
}
