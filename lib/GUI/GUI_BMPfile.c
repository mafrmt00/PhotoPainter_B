/*****************************************************************************
* | File      	:   GUI_BMPfile.h
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*                   Used to shield the underlying layers of each master
*                   and enhance portability
*----------------
* |	This version:   V1.0
* | Date        :   2022-10-11
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
#include "GUI_BMPfile.h"
#include "GUI_Paint.h"
#include "DEV_Config.h"
#include "Debug.h"

#include <stdlib.h>	//exit()
#include <string.h> //memset()
#include <math.h> //memset()

#include "f_util.h"
#include "ff.h"

//#define CALIBRATION_TARGET  // Uncomment to enable calibration test pattern

UBYTE RGBtoColor(UBYTE r, UBYTE g, UBYTE b) {
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;
    
    float max_val = fmaxf(rf, fmaxf(gf, bf));
    float min_val = fminf(rf, fminf(gf, bf));
    float delta = max_val - min_val;
    
    float L = (max_val + min_val) / 2.0f;
    
    float S = 0.0f;
    float H = 0.0f;
    
    if (delta > 0.0001f) {  // small epsilon for float comparison
        if (L < 0.5f) {
            S = delta / (max_val + min_val);
        } else {
            S = delta / (2.0f - max_val - min_val);
        }
        
        if (max_val == rf) {
            H = (gf - bf) / delta;
            if (gf < bf) H += 6.0f;
        } else if (max_val == gf) {
            H = (bf - rf) / delta + 2.0f;
        } else {
            H = (rf - gf) / delta + 4.0f;
        }
        H *= 60.0f;
        if (H < 0.0f) H += 360.0f;
    }
    
    // Determine color based on HSL with wider margins
    if (L < 0.3f) return 0; // Black
    if (L > 0.7f) return 1; // White
    if (S < 0.2f) return 1; // Low saturation, treat as white
    
    // Colored pixels
    if ((H >= 330.0f || H < 30.0f)) return 3; // Red
    if (H >= 30.0f && H < 90.0f) return 2; // Yellow
    if (H >= 90.0f && H < 150.0f) return 6; // Green
    if (H >= 210.0f && H < 270.0f) return 5; // Blue
    
    // Default to white if no match
    return 1;
}

UBYTE GUI_ReadBmp_RGB_6Color(const char *path, UWORD Xstart, UWORD Ystart)
{
    BMPFILEHEADER bmpFileHeader;  //Define a bmp file header structure
    BMPINFOHEADER bmpInfoHeader;  //Define a bmp info header structure
    
    FRESULT fr;
    FIL fil;
    UINT br;
    printf("open %s", path);
    fr = f_open(&fil, path, FA_READ);
    if (FR_OK != fr && FR_EXIST != fr) {
        panic("f_open(%s) error: %s (%d)\n", path, FRESULT_str(fr), fr);
        // exit(0);
    }
    
    // Set the file pointer from the beginning
    f_lseek(&fil, 0);
    f_read(&fil, &bmpFileHeader, sizeof(BMPFILEHEADER), &br);   // sizeof(BMPFILEHEADER) must be 14
    if (br != sizeof(BMPFILEHEADER)) {
        printf("f_read bmpFileHeader error\r\n");
        // printf("br is %d\n", br);
    }
    f_read(&fil, &bmpInfoHeader, sizeof(BMPINFOHEADER), &br);   // sizeof(BMPFILEHEADER) must be 50
    if (br != sizeof(BMPINFOHEADER)) {
        printf("f_read bmpInfoHeader error\r\n");
        // printf("br is %d\n", br);
    }
    if(bmpInfoHeader.biWidth > bmpInfoHeader.biHeight)
        Paint_SetRotate(0);
    else
        Paint_SetRotate(90);

    printf("pixel = %d * %d\r\n", bmpInfoHeader.biWidth, bmpInfoHeader.biHeight);

    // Determine if it is a monochrome bitmap
    int readbyte = bmpInfoHeader.biBitCount;
    if(readbyte != 24){
        printf("Bmp image is not 24 bitmap!\n");
    }
    // Read image data into the cache
    UWORD x, y;
    UBYTE Rdata[3];
    
    f_lseek(&fil, bmpFileHeader.bOffset);

    UBYTE color;
    printf("read data\n");
    
    for(y = 0; y < bmpInfoHeader.biHeight; y++) {//Total display column
        for(x = 0; x < bmpInfoHeader.biWidth ; x++) {//Show a line in the line
#ifdef CALIBRATION_TARGET
            // Generate vertical stripes for calibration test pattern
            int stripe_width = bmpInfoHeader.biWidth / 6;
            if (stripe_width == 0) stripe_width = 1;
            int stripe_index = x / stripe_width;
            if (stripe_index >= 6) stripe_index = 5;
            UBYTE stripe_colors[6] = {0, 1, 2, 3, 5, 6};
            color = stripe_colors[stripe_index];
#else
            if(f_read(&fil, (char *)Rdata, 1, &br) != FR_OK) {
                perror("get bmpdata:\r\n");
                break;
            }
            if(f_read(&fil, (char *)Rdata+1, 1, &br) != FR_OK) {
                perror("get bmpdata:\r\n");
                break;
            }
            if(f_read(&fil, (char *)Rdata+2, 1, &br) != FR_OK) {
                perror("get bmpdata:\r\n");
                break;
            }

            color = RGBtoColor(Rdata[2], Rdata[1], Rdata[0]);
#endif
            Paint_SetPixel(Xstart + bmpInfoHeader.biWidth-1-x, Ystart + y, color);
        }
        watchdog_update();
    }
    printf("close file\n");
    f_close(&fil);

    return 0;
}


