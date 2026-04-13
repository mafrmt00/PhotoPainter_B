#include "led.h"
#include "DEV_Config.h"

void ledPowerOn(void)
{
    for(int i=0; i<3; i++) {
        DEV_Digital_Write(LED_ACT, 1);
        DEV_Delay_ms(200);
        DEV_Digital_Write(LED_ACT, 0);
        DEV_Delay_ms(100);
    }
}

void ledLowPower(void)
{
    for(int i=0; i<5; i++) {
        DEV_Digital_Write(LED_PWR, 1);
        DEV_Delay_ms(200);
        DEV_Digital_Write(LED_PWR, 0);
        DEV_Delay_ms(100);
    }
}

void ledCharging(void)
{
    //DEV_Digital_Write(LED_PWR, 1);
}

void ledCharged(void)
{
    //DEV_Digital_Write(LED_PWR, 0);
}

void powerOff(void)
{
    DEV_Digital_Write(BAT_OFF, 0); // Turn off battery power to the system

    sleep_ms(300); // Allow time for power to fully cut before any further code execution attempts to run (should not happen since power is cut, but added for safety)

    watchdog_reboot(0,0,0); // Force a reset to start clean if we are still running code after power off command (should not happen, but added for safety)
}

void led_OFF_ACT(void)
{
    DEV_Digital_Write(LED_ACT, 0);
}

void led_ON_ACT(void)
{
    DEV_Digital_Write(LED_ACT, 1);
}

void led_ON_PWR(void)
{
    DEV_Digital_Write(LED_PWR, 1);
}   

void led_OFF_PWR(void)
{
    DEV_Digital_Write(LED_PWR, 0);
}



