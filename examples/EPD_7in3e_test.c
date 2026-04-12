/*****************************************************************************
* | File      	:   EPD_7in3e_test.c
* | Author      :   Waveshare team
* | Function    :   7.3inch e-Paper (F) Demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2023-03-13
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_Test.h"
#include "ImageData.h"
#include "run_File.h"
#include "EPD_7in3e.h"
#include "GUI_Paint.h"
#include "GUI_BMPfile.h"
#include "waveshare_PCF85063.h"  // RTC time source for overlays

#include <stdlib.h> // malloc() free()
#include <string.h>

static int isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int daysInMonth(int year, int month)
{
    if (month == 2) return isLeapYear(year) ? 29 : 28;
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

static int dayOfWeek(int y, int m, int d)
{
    if (m < 3) {
        m += 12;
        y -= 1;
    }
    int K = y % 100;
    int J = y / 100;
    int h = (d + (13 * (m + 1)) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
    // Zeller: 0=Saturday,1=Sunday,...6=Friday. Convert to 0=Sunday
    return (h + 6) % 7;
}

static void Overlay_DrawDateTime(const Time_data now)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u", now.years + 2000, now.months, now.days, now.hours, now.minutes, now.seconds);
    Paint_DrawString_EN(10, 10, buf, &Font16, EPD_7IN3E_WHITE, EPD_7IN3E_TRANSPARENT);
}

static void Overlay_DrawMonthCalendar(const Time_data now)
{
    Paint_SetScale(6);
    const char* monthNames[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    char title[40];
    snprintf(title, sizeof(title), "%s %04u", monthNames[now.months], now.years + 2000);
    int baseX = 10;
    int baseY = 20;
    Paint_DrawString_EN(baseX, baseY, title, &Font24, EPD_7IN3E_BLACK, WHITE);

    const char* weekNames[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    int colWidth = 800 / 9;  // 88 pixels per column
    int calendarStartX = colWidth;  // Start at column 1 (leave column 0 free)
    for (int i = 0; i < 7; i++) {
        Paint_DrawString_EN(calendarStartX + i * colWidth, baseY + 30, (char *)weekNames[i], &Font24, EPD_7IN3E_BLACK, EPD_7IN3E_TRANSPARENT);
    }

    int firstWeekday = dayOfWeek(now.years + 2000, now.months, 1);
    int dim = daysInMonth(now.years + 2000, now.months);
    int day = 1;
    for (int row = 0; row < 6 && day <= dim; row++) {
        for (int col = 0; col < 7 && day <= dim; col++) {
            if (row == 0 && col < firstWeekday) continue;
            char buf[8];
            snprintf(buf, sizeof(buf), "%2d", day);
            int x = calendarStartX + col * colWidth;
            int y = baseY + 60 + row * 50;  // Increased row spacing for larger font
            if (day == now.days) {
                Paint_DrawRectangle(x - 3, y - 3, x + 50, y + 30, EPD_7IN3E_BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
            }
            Paint_DrawString_EN(x, y, buf, &Font24, EPD_7IN3E_BLACK, WHITE);
            day++;
        }
    }
}

static int daysSinceEpoch(int y, int m, int d)
{
    if (m <= 2) {
        y -= 1;
        m += 12;
    }
    int a = y / 100;
    int b = a / 4;
    int c = 2 - a + b;
    int e = (int)(365.25 * (y + 4716));
    int f = (int)(30.6001 * (m + 1));
    return c + d + e + f - 1524;
}

static void DateFromDays(int days, int *y, int *m, int *d)
{
    int a = days + 32044;
    int b = (4 * a + 3) / 146097;
    int c = a - (146097 * b) / 4;
    int d1 = (4 * c + 3) / 1461;
    int e = c - (1461 * d1) / 4;
    int m1 = (5 * e + 2) / 153;
    *d = e - (153 * m1 + 2) / 5 + 1;
    *m = m1 + 3 - 12 * (m1 / 10);
    *y = 100 * b + d1 - 4800 + (m1 / 10);
}

static void Overlay_DrawWeekCalendar(const Time_data now)
{
    const char* weekNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    int baseX = 10;
    int baseY = 20;
    Paint_DrawString_EN(baseX, baseY, "Current Week", &Font24, EPD_7IN3E_BLACK, WHITE);

    int todaySerial = daysSinceEpoch(now.years + 2000, now.months, now.days);
    int weekday = dayOfWeek(now.years + 2000, now.months, now.days);
    int weekStartSerial = todaySerial - weekday;

    for (int i = 0; i < 7; i++) {
        int yy, mm, dd;
        DateFromDays(weekStartSerial + i, &yy, &mm, &dd);
        char buf[32];
        snprintf(buf, sizeof(buf), "%s %02d/%02d", weekNames[i], mm, dd);
        int y = baseY + 35 + i * 45;
        Paint_DrawString_EN(baseX, y, buf, &Font24, (i == weekday ? EPD_7IN3E_GREEN : EPD_7IN3E_BLACK), EPD_7IN3E_TRANSPARENT);
    }
}

static void Display_Overlay(int overlayId, const Time_data now)
{
    switch (overlayId) {
        case 0:
            break;
        case 1:
            Overlay_DrawMonthCalendar(now);
            break;
        case 2:
            Overlay_DrawWeekCalendar(now);
            break;
        case 3:
             Overlay_DrawDateTime(now);
             break;
        default:
            break;
    }
}

int EPD_7in3e_display_BMP(const char *path, float vol, int overlayId)
{
    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN3E_Init();

    //Create a new image cache
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_7IN3E_WIDTH % 2 == 0)? (EPD_7IN3E_WIDTH / 2 ): (EPD_7IN3E_WIDTH / 2 + 1)) * EPD_7IN3E_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT, 0, EPD_7IN3E_WHITE);
    Paint_SetScale(6);

#if 1
    run_mount();

    printf("Display BMP\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3E_WHITE);
    Paint_SetRotate(0); // Force upright orientation for overlay + bitmap
    
    GUI_ReadBmp_RGB_6Color(path, 0, 0);

    // Keep upright orientation
    Paint_SetRotate(180);

    // Add an overlay after BMP has been loaded
    Time_data now = PCF85063_GetTime();
    Display_Overlay(overlayId, now);

    char strvol[21] = {0};
    sprintf(strvol, "%f V", vol);
    if(vol < 3.3)
    {
        Paint_DrawString_EN(10, 10, "Low voltage, please charge in time.", &Font16, EPD_7IN3E_BLACK, EPD_7IN3E_WHITE);
        Paint_DrawString_EN(10, 26, strvol, &Font16, EPD_7IN3E_BLACK, EPD_7IN3E_WHITE);
    }

    printf("EPD_Display\r\n");
    EPD_7IN3E_Display(BlackImage);

    run_unmount();
#endif
    printf("Update Path Index...\r\n");
    updatePathIndex();

    printf("Display Sleep...\r\n\r\n");
    EPD_7IN3E_Sleep();
    free(BlackImage);
    BlackImage = NULL;

    return 0;
}

int EPD_7in3e_display(float vol)
{
    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN3E_Init();

    //Create a new image cache
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_7IN3E_WIDTH % 2 == 0)? (EPD_7IN3E_WIDTH / 2 ): (EPD_7IN3E_WIDTH / 2 + 1)) * EPD_7IN3E_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT, 0, EPD_7IN3E_WHITE);
    Paint_SetScale(6);

    printf("Display BMP\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3E_WHITE);
    
    Paint_DrawBitMap(Image6color);

    Paint_SetRotate(270);
    char strvol[21] = {0};
    sprintf(strvol, "%f V", vol);
    if(vol < 3.3) {
        Paint_DrawString_EN(10, 10, "Low voltage, please charge in time.", &Font16, EPD_7IN3E_BLACK, EPD_7IN3E_WHITE);
        Paint_DrawString_EN(10, 26, strvol, &Font16, EPD_7IN3E_BLACK, EPD_7IN3E_WHITE);
    }

    printf("EPD_Display\r\n");
    EPD_7IN3E_Display(BlackImage);

    printf("Goto Sleep...\r\n\r\n");
    EPD_7IN3E_Sleep();
    free(BlackImage);
    BlackImage = NULL;

    return 0;
}

int EPD_7in3e_test(void)
{
    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN3E_Init();

    //Create a new image cache
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_7IN3E_WIDTH % 2 == 0)? (EPD_7IN3E_WIDTH / 2 ): (EPD_7IN3E_WIDTH / 2 + 1)) * EPD_7IN3E_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT, 0, EPD_7IN3E_WHITE);
    Paint_SetScale(6);

#if 1   // Drawing on the image
    //1.Select Image
    printf("SelectImage:BlackImage\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3E_WHITE);

    int hNumber, hWidth, vNumber, vWidth;
    hNumber = 20;
	hWidth = EPD_7IN3E_HEIGHT/hNumber; // 800/20
    vNumber = 10;
	vWidth = EPD_7IN3E_WIDTH/vNumber; // 480/10
	
    // 2.Drawing on the image
    printf("Drawing:BlackImage\r\n");
	for(int i=0; i<vNumber; i++) {
		Paint_DrawRectangle(1, 1+i*vWidth, 800, vWidth*(i+1), EPD_7IN3E_GREEN + (i % 5), DOT_PIXEL_1X1, DRAW_FILL_FULL);
	}
	for(int i=0, j=0; i<hNumber; i++) {
		if(i%2) {
			j++;
			Paint_DrawRectangle(1+i*hWidth, 1, hWidth*(1+i), 480, j%2 ? EPD_7IN3E_BLACK : EPD_7IN3E_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		}
	}

    printf("EPD_Display\r\n");
    EPD_7IN3E_Display(BlackImage);
#endif

    printf("Display Sleep...\r\n");
    EPD_7IN3E_Sleep();
    free(BlackImage);
    BlackImage = NULL;

    return 0;
}

