#include "EPD_Test.h"   // Examples
#include "run_File.h"

#include "led.h"
#include "waveshare_PCF85063.h" // RTC
#include "DEV_Config.h"

#include <time.h>

extern const char *fileList;
extern char pathName[];

#define enChargingRtc 1

/*
Mode 0: Automatically get pic folder names and sort them
Mode 1: Automatically get pic folder names but not sorted
Mode 2: pic folder name is not automatically obtained, users need to create fileList.txt file and write the picture name in TF card by themselves
*/
#define Mode 0


float measureVBAT(void)
{
    float Voltage=0.0;
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t result = adc_read();
    Voltage = result * conversion_factor * 3;
    printf("Raw value: 0x%03x, voltage: %f V\n", result, Voltage);
    return Voltage;
}

void chargeState_callback() 
{
    if(DEV_Digital_Read(VBUS)) {
        if(!DEV_Digital_Read(CHARGE_STATE)) {  // is charging
            ledCharging();
        }
        else {  // charge complete
            ledCharged();
        }
    }
}

void run_display(Time_data Time, Time_data alarmTime, char hasCard)
{
    if(hasCard) {
        setFilePath();
        EPD_7in3e_display_BMP(pathName, measureVBAT());   // display bmp
    }
    else {
        printf("led_ON_PWR...\r\n");
        EPD_7in3e_display(measureVBAT());
    }
    DEV_Delay_ms(100);
    PCF85063_clear_alarm_flag();    // clear RTC alarm flag
    #if enChargingRtc
        rtcRunAlarm(Time, alarmTime);  // RTC run alarm
    #endif
}

int main(void)
{
    // Print firmware identification and build info
    printf("PhotoPainter (B) - Built: %s %s\r\n", __DATE__, __TIME__);
    
    // Initialize time structures for current time and alarm time
    Time_data Time = {2024-2000, 3, 31, 0, 0, 0};
    Time_data alarmTime = Time;
    // alarmTime.seconds += 10;
    alarmTime.minutes += 15; 
    //alarmTime.hours +=12;
    char isCard = 0;  // Flag to indicate if SD card is present
  
    printf("Init...\r\n");
    
    // Initialize hardware modules
    if(DEV_Module_Init() != 0) {  // DEV init
        return -1;
    }
    
    // Enable watchdog timer for 8 seconds to prevent hangs
    watchdog_enable(8*1000, 1);   // 8s
    DEV_Delay_ms(1000);
    
    // Initialize Real-Time Clock (RTC)
    PCF85063_init();    // RTC init
    rtcRunAlarm(Time, alarmTime);  // RTC run alarm
    
    // Set up GPIO interrupt for charge state changes
    gpio_set_irq_enabled_with_callback(CHARGE_STATE, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, chargeState_callback);

    // Check battery voltage - if too low, enter low power mode
    if(measureVBAT() < 3.1) {   // battery power is low
        printf("low power ...\r\n");
        PCF85063_alarm_Time_Disable();
        ledLowPower();  // LED flash for Low power
        powerOff(); // BAT off
        return 0;
    }
    else {
        printf("work ...\r\n");
        ledPowerOn();
    }

    // Check SD card presence and handle different modes
    if(!sdTest()) 
    {
        isCard = 1;
        if(Mode == 0)
        {
            printf("Mode 0\r\n");
            sdScanDir();  // Scan directory and save file list
            file_sort();  // Sort the file list
        }
        if(Mode == 1)
        {
            printf("Mode 1\r\n");
            sdScanDir();  // Scan directory and save file list (no sorting)
        }
        if(Mode == 2)
        {
            printf("Mode 2\r\n");
            file_cat();  // Read existing file list from SD card
        }
        
    }
    else 
    {
        isCard = 0;  // No SD card detected
    }

    // Main operation loop - handle charging vs normal display modes
    if(!DEV_Digital_Read(VBUS)) {    // no charge state - normal operation
        run_display(Time, alarmTime, isCard);
    }
    else {  // charge state - enter charging loop
        chargeState_callback();
        while(DEV_Digital_Read(VBUS)) {
            measureVBAT();  // Monitor battery voltage during charging
            #if enChargingRtc
                if(!DEV_Digital_Read(RTC_INT)) {    // RTC interrupt trigger
                    printf("rtc interrupt\r\n");
                    run_display(Time, alarmTime, isCard);
                }
            #endif

            if(!DEV_Digital_Read(BAT_STATE)) {  // KEY pressed
                printf("key interrupt\r\n");
                run_display(Time, alarmTime, isCard);
            }
            DEV_Delay_ms(200);
        }
    }
    
    printf("power off ...\r\n");
    powerOff(); // BAT off

    return 0;
}
