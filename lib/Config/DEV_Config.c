/**
 * @file DEV_Config.c
 * @author Waveshare team
 * @version 3.0
 * @date 2019-07-31
 * @brief Hardware configuration and device initialization module
 * 
 * This module provides hardware-level interface functions for:
 * - GPIO operations (digital read/write, mode configuration)
 * - SPI communication (single and multiple byte writes)
 * - I2C communication (byte read/write operations)
 * - System initialization (GPIO, SPI, I2C, ADC setup)
 * - Device timing and delay functions
 * 
 * The module abstracts low-level hardware interactions for the Raspberry Pi Pico
 * microcontroller and connected peripherals including:
 * - E-Paper display
 * - SD Card
 * - Real-Time Clock (RTC)
 * - Battery management
 * - LEDs and power control
 * 
 * @copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "DEV_Config.h"

/**
 * @brief Write a digital value to a GPIO pin
 * 
 * Sets the output level of a GPIO pin to either HIGH (1) or LOW (0).
 * The pin must be configured as an output using DEV_GPIO_Mode() before calling this function.
 * 
 * @param Pin GPIO pin number to write to
 * @param Value The digital value to write (0 for LOW, 1 for HIGH)
 * @return void
 * 
 * @see DEV_GPIO_Mode()
 * @see DEV_Digital_Read()
 */
void DEV_Digital_Write(UWORD Pin, UBYTE Value)
{
	gpio_put(Pin, Value);
}

/**
 * @brief Read the digital value from a GPIO pin
 * 
 * Reads the current input level of a GPIO pin.
 * The pin must be configured as an input using DEV_GPIO_Mode() before calling this function.
 * 
 * @param Pin GPIO pin number to read from
 * @return UBYTE The digital value read from the pin (0 for LOW, 1 for HIGH)
 * 
 * @see DEV_GPIO_Mode()
 * @see DEV_Digital_Write()
 */
UBYTE DEV_Digital_Read(UWORD Pin)
{
	return gpio_get(Pin);
}

/**
 * @brief Write a single byte to the SPI bus
 * 
 * Transmits a single byte to the e-Paper display via SPI.
 * This function performs a blocking SPI write operation.
 * 
 * @param Value The byte value to transmit
 * @return void
 * 
 * @note This function blocks until the SPI transmission is complete
 * @see DEV_SPI_Write_nByte()
 */
void DEV_SPI_WriteByte(UBYTE Value)
{
    spi_write_blocking(EPD_SPI_PORT, &Value, 1);
}

/**
 * @brief Write multiple bytes to the SPI bus
 * 
 * Transmits multiple bytes to the e-Paper display via SPI.
 * This function performs a blocking SPI write operation for bulk data transfer.
 * 
 * @param pData Pointer to the data buffer containing bytes to transmit
 * @param Len Number of bytes to transmit
 * @return void
 * 
 * @note This function blocks until all bytes have been transmitted
 * @see DEV_SPI_WriteByte()
 */
void DEV_SPI_Write_nByte(UBYTE *pData, uint32_t Len)
{
    spi_write_blocking(EPD_SPI_PORT, pData, Len);
}

/**
 * @brief Write a byte to the RTC via I2C
 * 
 * Writes a single byte value to a specified register in the Real-Time Clock (RTC)
 * module using I2C communication. The function sends both the register address
 * and the value to be written.
 * 
 * @param Reg The register address in the RTC to write to
 * @param Value The byte value to write to the register
 * @return void
 * 
 * @see I2C_Read_Byte()
 */
void I2C_Write_Byte(UBYTE Reg, UBYTE Value)
{
	UBYTE wbuff[2] = {Reg, Value};
	i2c_write_blocking(RTC_I2C_PORT, RTC_I2C_Address, wbuff, 2, false);
}

/**
 * @brief Read a byte from the RTC via I2C
 * 
 * Reads a single byte value from a specified register in the Real-Time Clock (RTC)
 * module using I2C communication. The function first writes the register address,
 * then reads the value from that register.
 * 
 * @param Reg The register address in the RTC to read from
 * @return UBYTE The byte value read from the register
 * 
 * @see I2C_Write_Byte()
 */
UBYTE I2C_Read_Byte(UBYTE Reg)
{
	UBYTE Value;
	i2c_write_blocking(RTC_I2C_PORT, RTC_I2C_Address, &Reg, 1, false);
	i2c_read_blocking(RTC_I2C_PORT, RTC_I2C_Address, &Value, 1, false);
	return Value;
}

/**
 * @brief Configure a GPIO pin as input or output
 * 
 * Initializes a GPIO pin and sets its direction (input or output).
 * Input pins can optionally have pull-up resistors enabled separately.
 * Output pins will drive the GPIO state when DEV_Digital_Write() is called.
 * 
 * @param Pin GPIO pin number to configure
 * @param Mode The pin direction mode
 *             - 0 or GPIO_IN: Configure as input
 *             - 1 or GPIO_OUT: Configure as output
 * @return void
 * 
 * @see DEV_Digital_Write()
 * @see DEV_Digital_Read()
 */
void DEV_GPIO_Mode(UWORD Pin, UWORD Mode)
{
    gpio_init(Pin);
	if(Mode == 0 || Mode == GPIO_IN) {
		gpio_set_dir(Pin, GPIO_IN);
	} else {
		gpio_set_dir(Pin, GPIO_OUT);
	}
}

/**
 * @brief Delay execution for a specified number of milliseconds
 * 
 * Pauses program execution for the specified duration in milliseconds.
 * The function also updates the watchdog timer to prevent watchdog reset
 * during long delays.
 * 
 * @param xms The delay duration in milliseconds
 * @return void
 * 
 * @note The watchdog timer is automatically updated to prevent resets
 */
void DEV_Delay_ms(UDOUBLE xms)
{
	sleep_ms(xms);
	watchdog_update();
}

/**
 * @brief Initialize all GPIO pins for connected peripherals
 * 
 * Configures GPIO pins for all connected hardware modules:
 * - e-Paper display (RST, DC, CS, BUSY)
 * - LEDs (Activity and Power)
 * - SD card (CS)
 * - Real-Time Clock (INT)
 * - Battery management (OFF, STATE, CHARGE_STATE)
 * - Power control (EPD_POWER_EN, VBUS)
 * 
 * Sets initial pin states:
 * - Both LEDs are turned OFF
 * - Chip Select pin is set HIGH
 * - Battery power is enabled (BAT_OFF = 1)
 * - e-Paper power is enabled (EPD_POWER_EN = 1)
 * 
 * @return void
 * 
 * @see DEV_GPIO_Mode()
 * @see DEV_Digital_Write()
 * @see DEV_Module_Init()
 */
void DEV_GPIO_Init(void)
{
	// EPD
	DEV_GPIO_Mode(EPD_RST_PIN, 1);
	DEV_GPIO_Mode(EPD_DC_PIN, 1);
	DEV_GPIO_Mode(EPD_CS_PIN, 1);
	DEV_GPIO_Mode(EPD_BUSY_PIN, 0);

	// LED
	DEV_GPIO_Mode(LED_ACT, 1);
	DEV_GPIO_Mode(LED_PWR, 1);
	
	// SDCARD
	DEV_GPIO_Mode(SD_CS_PIN, 1);
	
	// RTC
	DEV_GPIO_Mode(RTC_INT, 0);
	gpio_pull_up(RTC_INT);
	
	// BAT
	DEV_GPIO_Mode(BAT_OFF, 1);
	DEV_GPIO_Mode(BAT_STATE, 0);
	gpio_pull_up(BAT_STATE);
	DEV_GPIO_Mode(CHARGE_STATE, 0);
	gpio_pull_up(CHARGE_STATE);
	
	// POWER
	DEV_GPIO_Mode(EPD_POWER_EN, 1);
	DEV_GPIO_Mode(VBUS, 0);
	

	DEV_Digital_Write(LED_ACT, 0);	// LED off
	DEV_Digital_Write(LED_PWR, 0);	// LED off
	DEV_Digital_Write(EPD_CS_PIN, 1);
	DEV_Digital_Write(BAT_OFF, 1);	// BAT on
	DEV_Digital_Write(EPD_POWER_EN, 1);	// EPD power on
}

/**
 * @brief Initialize all hardware modules and peripherals
 * 
 * Performs complete hardware initialization including:
 * - Standard I/O initialization
 * - e-Paper SPI configuration (8 MHz)
 * - SD card SPI configuration (12.5 MHz)
 * - Real-Time Clock I2C configuration (100 kHz)
 * - ADC initialization for battery voltage measurement
 * - All GPIO pin configuration
 * 
 * This function must be called once during system startup before using
 * any hardware peripherals. Output includes a confirmation message to stdout.
 * 
 * @return UBYTE Returns 0 on successful initialization
 * 
 * @see DEV_GPIO_Init()
 * @see DEV_Module_Exit()
 */
UBYTE DEV_Module_Init(void)
{
    stdio_init_all();
	
    spi_init(EPD_SPI_PORT, 8000 * 1000);
    gpio_set_function(EPD_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(EPD_MOSI_PIN, GPIO_FUNC_SPI);
	
	spi_init(SD_SPI_PORT, 12500 * 1000);
	gpio_set_function(SD_CLK_PIN, GPIO_FUNC_SPI);
	gpio_set_function(SD_MOSI_PIN, GPIO_FUNC_SPI);
	gpio_set_function(SD_MISO_PIN, GPIO_FUNC_SPI);

	i2c_init(RTC_I2C_PORT, 100 * 1000); 
	gpio_set_function(RTC_SDA, GPIO_FUNC_I2C);
    gpio_set_function(RTC_SCL, GPIO_FUNC_I2C);

	adc_init();
	adc_gpio_init(VBAT);
	adc_select_input(3);

	// GPIO Config
	DEV_GPIO_Init();

    printf("DEV_Module_Init OK \r\n");
	return 0;
}

/**
 * @brief Deinitialize hardware modules and cleanup resources
 * 
 * Performs cleanup and shutdown operations for hardware peripherals.
 * This function should be called when closing the application or before
 * powering down the system.
 * 
 * @return void
 * 
 * @see DEV_Module_Init()
 */
void DEV_Module_Exit(void)
{
	printf("Module exit \r\n");
}
