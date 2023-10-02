/* vim: set ai et ts=4 sw=4: */
#include "st7735.h"
#include "time.h"
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdarg.h>
#include "rpiInfo.h"

int i2cd;

// Demo for ssd1306 i2c driver for  Raspberry Pi
// Hacked on by Phil Karn, KA9Q Sept 2023

#include <stdio.h>
#include "st7735.h"
#include "time.h"
#include <unistd.h>
#include <stdlib.h>

int printo(uint16_t x, uint16_t y, FontDef const font, uint16_t color, uint16_t bgcolor, char const *fmt, ...);

int main(void) 
{
	if(lcd_begin()){
	  //LCD Screen initialization
	  return 0;
	}
	sleep(1);
	lcd_fill_screen(ST7735_BLACK);
#if 1
	// Host name is more useful than a dynamic IP address
	char hostname[1000];
	gethostname(hostname,sizeof(hostname));
	printo(0,0,Font_8x16,ST7735_WHITE,ST7735_BLACK, "%s",hostname);
#else
	if (IP_SWITCH == IP_DISPLAY_OPEN)
	  {
	    lcd_write_string(0, 0, "IP:", Font_8x16, ST7735_WHITE, ST7735_BLACK);
	    strcpy(iPSource, get_ip_address());                                       // Get the IP address of the device's wireless network card
	    lcd_write_string(24, 0, iPSource, Font_8x16, ST7735_WHITE, ST7735_BLACK); // Send the IP address to the lower machine
	  }
	else
	  {
	    lcd_write_string(0, 0, CUSTOM_DISPLAY, Font_8x16, ST7735_WHITE, ST7735_BLACK); // Send the IP address to the lower machine
	  }
#endif

	lcd_fill_rectangle(0, 20, ST7735_WIDTH, 5, ST7735_BLUE); // Horizontal blue bar
	while(1){
	  lcd_display_cpuLoad();
	  sleep(2);
	  lcd_display_temp();
	  sleep(2);

	  lcd_display_net();
	  sleep(2);

#if 0	  // ram & disk aren't very useful
	  lcd_display_ram();
	  sleep(2);
	  lcd_display_disk();
	  sleep(2);
#endif
	}
	exit(0);
}


/*
 * Set display coordinates
 */
void lcd_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    // col address set
    i2c_write_command(X_COORDINATE_REG, x0 + ST7735_XSTART, x1 + ST7735_XSTART);
    // row address set
    i2c_write_command(Y_COORDINATE_REG, y0 + ST7735_YSTART, y1 + ST7735_YSTART);
    // write to RAM
    i2c_write_command(CHAR_DATA_REG, 0x00, 0x00);

    i2c_write_command(SYNC_REG, 0x00, 0x01);
}

/*
 * Display a single character
 */
void lcd_write_char(uint16_t x, uint16_t y, char ch, FontDef const font, uint16_t color, uint16_t bgcolor)
{
    uint32_t i, b, j;

    lcd_set_address_window(x, y, x + font.width - 1, y + font.height - 1);

    for (i = 0; i < font.height; i++)
    {
        b = font.data[(ch - 32) * font.height + i];
        for (j = 0; j < font.width; j++)
        {
            if ((b << j) & 0x8000)
            {
                i2c_write_data(color >> 8, color & 0xFF);
            }
            else
            {
                i2c_write_data(bgcolor >> 8, bgcolor & 0xFF);
            }
        }
    }
}


/*
 * display string
 */
void lcd_write_string(uint16_t x, uint16_t y, char const *str, FontDef const font, uint16_t color, uint16_t bgcolor)
{

    while (*str)
    {
        if (x + font.width >= ST7735_WIDTH)
        {
            x = 0;
            y += font.height;
            if (y + font.height >= ST7735_HEIGHT)
            {
                break;
            }

            if (*str == ' ')
            {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        lcd_write_char(x, y, *str, font, color, bgcolor);
        i2c_write_command(SYNC_REG, 0x00, 0x01);
        x += font.width;
        str++;
    }
}


// This function was calling out to be written. --KA9Q
int printo(uint16_t x, uint16_t y, FontDef const font, uint16_t color, uint16_t bgcolor, char const *fmt, ...){
  va_list ap;

  // Determine required size
  va_start(ap, fmt);
  int n = vsnprintf(NULL, 0, fmt, ap);
  char *p = alloca(n+1); // one extra for null
  n = vsnprintf(p,n+1,fmt,ap);
  va_end(ap);
  lcd_write_string(x,y,p,font,color,bgcolor);
  return n;
}


/*
 * fill rectangle
 */
void lcd_fill_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint8_t buff[320] = {0};
    uint16_t count = 0;
    // clipping
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT))
        return;
    if ((x + w - 1) >= ST7735_WIDTH)
        w = ST7735_WIDTH - x;
    if ((y + h - 1) >= ST7735_HEIGHT)
        h = ST7735_HEIGHT - y;
    lcd_set_address_window(x, y, x + w - 1, y + h - 1);

    for (count = 0; count < w; count++)
    {
        buff[count * 2] = color >> 8;
        buff[count * 2 + 1] = color & 0xFF;
    }
    for (y = h; y > 0; y--)
    {
        i2c_burst_transfer(buff, sizeof(uint16_t) * w);
    }
}

/*
 * fill screen
 */

void lcd_fill_screen(uint16_t color)
{
    lcd_fill_rectangle(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
    i2c_write_command(SYNC_REG, 0x00, 0x01);
}

uint8_t lcd_begin(void)
{
    uint8_t count = 0;
    uint8_t buffer[20] = {0};
    uint8_t i2c[20] = "/dev/i2c-1";
    // I2C Init
    i2cd = open(i2c, O_RDWR); //"/dev/i2c-1"
    if (i2cd < 0)
    {
        fprintf(stderr, "Device I2C-1 failed to initialize\n");
        return 1;
    }
    if (ioctl(i2cd, I2C_SLAVE_FORCE, I2C_ADDRESS) < 0)
    {
        return 1;
    }
    return 0;
}

void i2c_write_data(uint8_t high, uint8_t low)
{
    uint8_t msg[3] = {WRITE_DATA_REG, high, low};
    write(i2cd, msg, 3);
    usleep(10);
}

void i2c_write_command(uint8_t command, uint8_t high, uint8_t low)
{
    uint8_t msg[3] = {command, high, low};
    write(i2cd, msg, 3);
    usleep(10);
}

void i2c_burst_transfer(uint8_t const *buff, uint32_t length)
{
  useconds_t delay = 400;

  uint32_t count = 0;
    i2c_write_command(BURST_WRITE_REG, 0x00, 0x01);
    while (length > count)
    {
        if ((length - count) > BURST_MAX_LENGTH)
        {
            write(i2cd, buff + count, BURST_MAX_LENGTH);
            count += BURST_MAX_LENGTH;
        }
        else
        {
            write(i2cd, buff + count, length - count);
            count += (length - count);
        }
	usleep(delay);
    }
    i2c_write_command(BURST_WRITE_REG, 0x00, 0x00);
    i2c_write_command(SYNC_REG, 0x00, 0x01);
}


void lcd_display_percentage(uint8_t val, uint16_t color)
{
    uint8_t count = 0;
    uint8_t xCoordinate = 0;
    val += 10;
    if (val >= 100)
    {
        val = 100;
    }
    val /= 10;
    for (count = 0; count < val; count++)
    {
        lcd_fill_rectangle(xCoordinate, 60, 6, 10, color);
        xCoordinate += 10;
    }
    for (count = 0; count < 10 - val; count++)
    {
        lcd_fill_rectangle(xCoordinate, 60, 6, 10, ST7735_GRAY);
        xCoordinate += 10;
    }
}

void lcd_display_cpuLoad(void)
{
    char iPSource[20] = {0};
    uint8_t cpuLoad = 0;

    cpuLoad = get_cpu_message();

    printo(0, 35, Font_11x18, ST7735_WHITE, ST7735_BLACK,"CPU: %d%%     ",cpuLoad);
    lcd_display_percentage(cpuLoad, ST7735_GREEN);
}

void lcd_display_ram(void)
{
    float Totalram = 0.0;
    float freeram = 0.0;
    uint8_t residue = 0;
    uint8_t Total[10] = {0};
    uint8_t free[10] = {0};
    uint8_t residueStr[10] = {0};
    get_cpu_memory(&Totalram, &freeram);
    residue = (Totalram - freeram) / Totalram * 100;
    printo(0, 35, Font_11x18, ST7735_WHITE, ST7735_BLACK, "RAM: %d%%   ",residue);
    lcd_display_percentage(residue, ST7735_YELLOW);
}

void lcd_display_temp(void)
{
    float temp = get_temperature();
    printo(0,35,Font_11x18, ST7735_WHITE, ST7735_BLACK,"Temp: %.1f%c   ",temp,TEMPERATURE_TYPE == FAHRENHEIT ? 'F' : 'C');

    if (TEMPERATURE_TYPE == FAHRENHEIT)
    {
        temp -= 32;
        temp /= 1.8;
    }
    lcd_display_percentage((uint16_t)temp, ST7735_RED);
}

void lcd_display_net(void){
  int tx_rate = get_tx_rate();
  
    printo(0, 35, Font_11x18, ST7735_WHITE, ST7735_BLACK,"Net: %3d Mb/s",tx_rate/1000000);
}


void lcd_display_disk(void)
{

    uint16_t diskMemSize = 0;
    uint16_t diskUseMemSize = 0;
    uint32_t sdMemSize = 0;
    uint32_t sdUseMemSize = 0;

    uint16_t memTotal = 0;
    uint16_t useMemTotal = 0;
    uint16_t residue = 0;

    get_sd_memory(&sdMemSize, &sdUseMemSize);
    get_hard_disk_memory(&diskMemSize, &diskUseMemSize);

    memTotal = sdMemSize + diskMemSize;
    useMemTotal = sdUseMemSize + diskUseMemSize;
    residue = useMemTotal * 1.0 / memTotal * 100;

    printo(0, 35, Font_11x18, ST7735_WHITE, ST7735_BLACK,"Disk: %d%%   ",residue);

    lcd_display_percentage(residue, ST7735_BLUE);
}
void lcd_write_ch(uint16_t x, uint16_t y, char ch, FontType const font, uint16_t color, uint16_t bgcolor)
{
    switch (font)
    {
    case FontType_7x10:
        lcd_write_char(x, y, ch, Font_7x10, color, bgcolor);
        break;
    case FontType_8x16:
        lcd_write_char(x, y, ch, Font_8x16, color, bgcolor);
        break;
    case FontType_11x18:
        lcd_write_char(x, y, ch, Font_11x18, color, bgcolor);
        break;
    case FontType_16x26:
        lcd_write_char(x, y, ch, Font_16x26, color, bgcolor);
        break;
    }
}
void lcd_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t const *data)
{
    uint16_t col = h - y;
    uint16_t row = w - x;
    lcd_set_address_window(x, y, x + w - 1, y + h - 1);
    i2c_burst_transfer(data, sizeof(uint16_t) * col * row);
}

void lcd_write_str(uint16_t x, uint16_t y, char const *str, FontType const font, uint16_t color, uint16_t bgcolor)
{
    switch (font)
    {
    case FontType_7x10:
        lcd_write_string(x, y, str, Font_7x10, color, bgcolor);
        break;
    case FontType_8x16:
        lcd_write_string(x, y, str, Font_8x16, color, bgcolor);
        break;
    case FontType_11x18:
        lcd_write_string(x, y, str, Font_11x18, color, bgcolor);
        break;
    case FontType_16x26:
        lcd_write_string(x, y, str, Font_16x26, color, bgcolor);
        break;
    }
}
