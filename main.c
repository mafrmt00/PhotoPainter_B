#include "EPD_Test.h"   // Examples
#include "run_File.h"

#include "led.h"
#include "waveshare_PCF85063.h" // RTC
#include "DEV_Config.h"
#include "minIni.h"

#include <time.h>
#include <string.h>

extern const char *fileList;
extern char pathName[];

#define enChargingRtc 1

// Alarm interval in minutes
#define RTC_ALARM_INTERVAL (1)

/*
Mode 0: Automatically get pic folder names and sort them
Mode 1: Automatically get pic folder names but not sorted
Mode 2: pic folder name is not automatically obtained, users need to create fileList.txt file and write the picture name in TF card by themselves
*/
#define Mode 0

/**
 * @brief Measure the battery voltage via ADC
 * 
 * Performs an ADC read of the VBAT pin and converts the raw value to voltage.
 * The function uses a conversion factor based on 3.3V reference and 12-bit ADC resolution,
 * then multiplies by 3 to account for the voltage divider circuit.
 * 
 * Prints voltage information to console every 20 calls to avoid logging spam.
 * 
 * @return float The measured battery voltage in volts
 * 
 * @note This function maintains internal state for call counting to throttle output
 * @see chargeState_callback()
 */
float measureVBAT(void)
{
    static int call_count = 0;
    float Voltage = 0.0;
    // ADC conversion factor: 3.3V reference divided by 4096 (2^12 for 12-bit ADC)
    const float conversion_factor = 3.3f / (1 << 12);
    
    // Read raw ADC value from battery voltage pin
    uint16_t result = adc_read();
    
    // Convert raw ADC value to actual voltage
    // Multiply by 3 due to voltage divider circuit (real voltage is 3x the scaled voltage)
    Voltage = result * conversion_factor * 3;
    
    // Throttle console output to every 20 calls to reduce log spam
    call_count++;
    if (call_count >= 20) 
    {
        PCF85063_PrintCurrentTime();
        printf("Raw value: 0x%03x, voltage: %f V\n", result, Voltage);
        call_count = 0;
    }
    return Voltage;
}

/**
 * @brief GPIO interrupt callback for charge state changes
 * 
 * Handles GPIO interrupt triggered when charge state pin changes.
 * Reads the current charge state and updates LED indicators accordingly:
 * - If VBUS is present and charging: Activate charging LED animation
 * - If VBUS is present and charge complete: Activate charged LED animation
 * 
 * This callback is triggered on GPIO_IRQ_EDGE_RISE or GPIO_IRQ_EDGE_FALL events
 * from the CHARGE_STATE pin.
 * 
 * @return void
 * 
 * @see ledCharging()
 * @see ledCharged()
 * @see measureVBAT()
 */
void chargeState_callback() 
{
    // Check if USB power (VBUS) is present
    if(DEV_Digital_Read(VBUS)) 
    {
        // Check charge state pin (active low)
        if(!DEV_Digital_Read(CHARGE_STATE)) 
        {  
            // Charging in progress
            ledCharging();
        }
        else 
        {  
            // Charge cycle complete
            ledCharged();
        }
    }
}

/**
 * @brief Display content on the e-Paper display
 * 
 * Updates the e-Paper display with appropriate content based on SD card availability.
 * If SD card is present, displays a BMP image from the file list with date/time overlay.
 * If no SD card, displays a default test pattern.
 * 
 * @param hasCard Flag indicating if SD card is present (1=present, 0=not present)
 * @return void
 * 
 * @see EPD_7in3e_display_BMP()
 * @see EPD_7in3e_display()
 * @see measureVBAT()
 */
void run_display(char hasCard)
{
    if(hasCard) 
    {
        // Prepare the file path for the next image to display
        setFilePath();
        // Display BMP image with date/time overlay
        // overlayId: 3=date-time, 1=month calendar, 2=week calendar
        EPD_7in3e_display_BMP(pathName, measureVBAT(), 3);
    }
    else 
    {
        // No SD card: display default pattern
        printf("led_ON_PWR...\r\n");
        EPD_7in3e_display(measureVBAT());
    }
    // Delay for e-Paper display to complete update
    DEV_Delay_ms(100);
}

/**
 * @brief Load RTC settings from frame.ini if update flag is set
 *
 * Reads the [datetime] section from frame.ini containing date, time, and an update flag.
 * If the update flag is set to 1, applies the date/time values to the RTC and resets
 * the flag back to 0.
 *
 * @return void
 *
 * @see PCF85063_SetTime_YMD()
 * @see PCF85063_SetTime_HMS()
 */
void load_rtc_from_ini(void)
{
    char iniFile[] = "frame.ini";
    long update_flag = 0;
    long year, month, day, hour, minute, second;

    // Read the update flag first
    update_flag = ini_getl("datetime", "update", 0, iniFile);

    if (update_flag == 1)
    {
        // Read date/time values from ini file
        year = ini_getl("datetime", "year", 2026, iniFile);
        month = ini_getl("datetime", "month", 1, iniFile);
        day = ini_getl("datetime", "day", 1, iniFile);
        hour = ini_getl("datetime", "hour", 0, iniFile);
        minute = ini_getl("datetime", "minute", 0, iniFile);
        second = ini_getl("datetime", "second", 0, iniFile);

        // Apply the date and time to RTC
        PCF85063_SetTime_YMD((int)year, (int)month, (int)day);
        PCF85063_SetTime_HMS((int)hour, (int)minute, (int)second);

        PCF85063_PrintCurrentTime();
        printf("RTC updated from frame.ini: %04ld-%02ld-%02ld %02ld:%02ld:%02ld\n", 
               year, month, day, hour, minute, second);

        // Reset the update flag to 0
        ini_putl("datetime", "update", 0, iniFile);
        printf("Update flag reset to 0\n");
    }
    else
    {
        printf("No RTC update requested (update flag = %ld)\n", update_flag);
    }
}

/**
 * @brief Main application entry point
 * 
 * Initializes all hardware modules (GPIO, SPI, I2C, ADC), configures the RTC and watchdog,
 * checks system health (battery voltage), and enters the main operational loop.
 * 
 * The application supports two operational modes:
 * 1. Normal Mode: Displays images on e-Paper when not charging
 * 2. Charging Mode: Continuously monitors battery and displays when RTC alarm triggers
 * 
 * Supports three SD card operation modes (configured via Mode define):
 * - Mode 0: Auto-detect and sort image folder names
 * - Mode 1: Auto-detect image folder names (unsorted)
 * - Mode 2: Manual file list (requires fileList.txt on SD card)
 * 
 * @return int Exit code (0 on normal shutdown, -1 on initialization error)
 * 
 * @see DEV_Module_Init()
 * @see measureVBAT()
 * @see PCF85063_clear_alarm_flag()
 * @see run_display()
 */
int main(void)
{
    // Flag to track SD card presence
    char isCard = 0;
      
    // ========== HARDWARE INITIALIZATION ==========
    // Initialize all hardware peripherals (GPIO, SPI, I2C, ADC)
    if(DEV_Module_Init() != 0) {
        return -1;
    }
    
    // Turn on power indicator LED
    led_ON_PWR();

    // Print firmware identification and build information
    printf("PhotoPainter (B) - Built: %s %s\r\n", __DATE__, __TIME__);
    printf("Init...\r\n");
    
    // ========== RTC AND WATCHDOG SETUP ==========
    printf("Restarting rtc alarm\r\n");
    // Clear any previous RTC alarm flag to start fresh
    PCF85063_clear_alarm_flag();
    // Configure RTC alarm to wake device periodically
    rtcRunAlarm(RTC_ALARM_INTERVAL * 60);

    //Countdown test for RTC to enable Serial output observation
    PCF85063_test();

    // Enable watchdog timer: 8 seconds timeout, will reset on overflow
    // This prevents system hangs by forcing a reset if main loop stalls
    watchdog_enable(8*1000, 1);
    DEV_Delay_ms(1000);

    // ========== GPIO INTERRUPT SETUP ==========
    // Register callback for charge state pin changes (rising and falling edges)
    // This enables real-time response to USB charging connection/disconnection
    gpio_set_irq_enabled_with_callback(CHARGE_STATE, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, chargeState_callback);

    // ========== BATTERY HEALTH CHECK ==========
    // Check if battery voltage is critically low
    float batteryVoltage = measureVBAT();
    if(batteryVoltage < 3.1) 
    {
        // Battery voltage too low for normal operation
        PCF85063_PrintCurrentTime();
        printf("Low battery detected (%.2fV) - entering low power mode...\r\n", batteryVoltage);
        // Disable RTC alarm to prevent power drain from wakeup cycles
        PCF85063_alarm_Time_Disable();
        // Flash LED to indicate low power condition
        ledLowPower();
        printf("Shutting down...\r\n");
        // Cut power to the entire system
        powerOff();
        return 0;
    }
    else 
    {
        // Battery level is sufficient for normal operation
        PCF85063_PrintCurrentTime();
        printf("All systems nominal (%.2fV)...\r\n", batteryVoltage);
        ledPowerOn();
    }

    // ========== SD CARD DETECTION AND INITIALIZATION ==========
    // Test SD card presence and initialize file list based on selected mode
    if(!sdTest()) 
    {
        // SD card detected successfully
        isCard = 1;
        
        // Execute file list handling based on selected mode
        if(Mode == 0)
        {
            // Mode 0: Auto-scan and sort directory contents
            printf("Mode 0: Auto-scan with sorting\r\n");
            sdScanDir();  // Scan directory and generate file list
            file_sort();  // Sort the file list alphabetically
        }
        else if(Mode == 1)
        {
            // Mode 1: Auto-scan without sorting
            printf("Mode 1: Auto-scan without sorting\r\n");
            sdScanDir();  // Scan directory and generate file list (original order)
        }
        else if(Mode == 2)
        {
            // Mode 2: Manual mode - read fileList.txt from SD card
            printf("Mode 2: Manual file list\r\n");
            file_cat();  // Read and parse existing fileList.txt
        }

        load_rtc_from_ini(); // Load RTC settings from frame.ini if update flag is set
    }
    else 
    {
        // No SD card detected - will use default display mode
        isCard = 0;
    }



    // ========== MAIN OPERATIONAL LOOP ==========
    // Branch into two different operational modes based on USB charging status
    
    if(!DEV_Digital_Read(VBUS)) 
    {
        // ========== NORMAL MODE: NOT CONNECTED TO POWER ==========
        // Battery-powered operation: display image once then prepare for sleep
        run_display(isCard);
    }
    else 
    {
        // ========== CHARGING MODE: USB POWER PRESENT ==========
        // Device is connected to USB power - enter charging loop
        // Update LED to reflect charging state
        chargeState_callback();

        // Remain in charging loop while USB power is present
        while(DEV_Digital_Read(VBUS)) 
        {
            // Continuously monitor battery voltage during charging
            measureVBAT();

            // Refresh watchdog timer to prevent reset during charging loop
            // (Otherwise watchdog would trigger after 8 seconds of continuous loop)
            watchdog_update();

            // Check if RTC alarm or button interrupt occurred
            // BAT_STATE is active low when alarm triggers
            if(!DEV_Digital_Read(BAT_STATE)) 
            {  
                // Alarm or button interrupt detected
                PCF85063_clear_alarm_flag();  // Clear RTC interrupt flag
                   
                PCF85063_PrintCurrentTime();
                printf("RTC/Button alarm interrupt start\r\n");
                
                // If charging with RTC enabled, reschedule next alarm
                #if enChargingRtc
                    rtcRunAlarm(RTC_ALARM_INTERVAL * 60);
                #endif
                
                // Update display while plugged in
                run_display(isCard);
                
                PCF85063_PrintCurrentTime();
                printf("RTC/Button alarm interrupt end\r\n");
            }
            // Poll at 200ms intervals to reduce CPU usage during charging
            DEV_Delay_ms(200);
        }
    }
    
    // ========== SHUTDOWN SEQUENCE ==========
    // Device is no longer powered by USB - prepare for sleep or shutdown
    printf("Powering down...\r\n");
    led_OFF_PWR();  // Turn off power indicator LED
    powerOff();     // Cut power to the system

    printf("This should not have happened\r\n");
    return 0;
}