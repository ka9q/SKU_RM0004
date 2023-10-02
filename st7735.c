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

int I2cd;

// Demo for ssd1306 i2c driver for  Raspberry Pi
// Hacked on by Phil Karn, KA9Q Sept 2023

#include <stdio.h>
#include "st7735.h"
#include "time.h"
#include <unistd.h>
#include <stdlib.h>

// Set display coordinates
void lcd_set_address_window(int x0, int y0, int x1, int y1){
  // col address set
  i2c_write_command(X_COORDINATE_REG, x0 + ST7735_XSTART, x1 + ST7735_XSTART);
  // row address set
  i2c_write_command(Y_COORDINATE_REG, y0 + ST7735_YSTART, y1 + ST7735_YSTART);
  // write to RAM
  i2c_write_command_word(CHAR_DATA_REG, 0);
  
  i2c_write_command_word(SYNC_REG, 0);
}


// Display a single character
void lcd_write_char(int x, int y, char ch, FontDef const font, uint16_t color, uint16_t bgcolor){
  lcd_set_address_window(x, y, x + font.width - 1, y + font.height - 1);
  
  for (int i = 0; i < font.height; i++){
    int b = font.data[(ch - 32) * font.height + i];
    for (int j = 0; j < font.width; j++){
      if ((b << j) & 0x8000){
	i2c_write_word(color);
      } else {
	i2c_write_word(bgcolor);
      }
    }
  }
}


// display string
void lcd_write_string(int x, int y, char const *str, FontDef const font, uint16_t color, uint16_t bgcolor){
  while (*str){
    if (x + font.width >= ST7735_WIDTH){
      x = 0;
      y += font.height;
      if (y + font.height >= ST7735_HEIGHT){
	break;
      }
      if (*str == ' '){
	// skip spaces in the beginning of the new line
	str++;
	continue;
      }
    }
    lcd_write_char(x, y, *str, font, color, bgcolor);
    i2c_write_command_word(SYNC_REG, 0);
    x += font.width;
    str++;
  }
}


// This function was calling out to be written. --KA9Q
// Printf function specifying start coordinates, font, foreground and background colors
int printo(int x, int y, FontDef const font, uint16_t color, uint16_t bgcolor, char const *fmt, ...){
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


// fill rectangle
void lcd_fill_rectangle(int x, int y, int w, int h, uint16_t color){
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
  
  for (count = 0; count < w; count++){
    buff[count * 2] = color >> 8;
    buff[count * 2 + 1] = color & 0xFF;
  }
  for (y = h; y > 0; y--) 
    i2c_burst_transfer(buff, sizeof(uint16_t) * w);
}

// fill screen
void lcd_fill_screen(uint16_t color){
  lcd_fill_rectangle(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
  i2c_write_command_word(SYNC_REG, 1);
}

int lcd_begin(const char *dev){
  // I2C Init
  I2cd = open(dev, O_RDWR); // usually "/dev/i2c-1"
  if (I2cd < 0){
    fprintf(stderr, "Device %s failed to initialize\n",dev);
    return 1;
  }
  if (ioctl(I2cd, I2C_SLAVE_FORCE, I2C_ADDRESS) < 0)
    return 1;

  return 0;
}

void i2c_write_data(uint8_t high, uint8_t low){
    uint8_t msg[3] = {WRITE_DATA_REG, high, low};
    write(I2cd, msg, 3);
    usleep(10);
}
void i2c_write_word(uint16_t word){
    uint8_t msg[3] = {WRITE_DATA_REG, word >> 8, word & 0xff};
    write(I2cd, msg, 3);
    usleep(10);
}

void i2c_write_command(uint8_t command, uint8_t high, uint8_t low){
    uint8_t msg[3] = {command, high, low};
    write(I2cd, msg, 3);
    usleep(10);
}
void i2c_write_command_word(uint8_t command, uint16_t word){
    uint8_t msg[3] = {command, word >> 8, word & 0xff};
    write(I2cd, msg, 3);
    usleep(10);
}

void i2c_burst_transfer(uint8_t const *buff, uint32_t length){
  useconds_t delay = 500;

  uint32_t count = 0;
  i2c_write_command_word(BURST_WRITE_REG, 1);
  while (length > count){
    if ((length - count) > BURST_MAX_LENGTH){
      write(I2cd, buff + count, BURST_MAX_LENGTH);
      count += BURST_MAX_LENGTH;
    } else {
      write(I2cd, buff + count, length - count);
      count += (length - count);
    }
    usleep(delay);
  }
  i2c_write_command_word(BURST_WRITE_REG, 0);
  i2c_write_command_word(SYNC_REG, 1);
}


// Display pseudo analog scale, with any part exceeding threshold displayed in different color
void lcd_display_percentage(int val, int threshold_val, uint16_t color,uint16_t threshold_color){

  val = (val < 0) ? 0 : val;
  val = (val > 100) ? 100 : val;
  val = val * 150 / 100;

  threshold_val = (threshold_val < 0) ? 0 : threshold_val;
  threshold_val = (threshold_val > 100) ? 100 : threshold_val;
  threshold_val = threshold_val * 150 / 100;

  int xCoordinate = 0;
  while(xCoordinate < val && xCoordinate < threshold_val){
    lcd_fill_rectangle(xCoordinate, 60, 6, 10, color);
    xCoordinate += 10;
  }
  while(xCoordinate < val){
    lcd_fill_rectangle(xCoordinate, 60, 6, 10, threshold_color);
    xCoordinate += 10;
  }
  while(xCoordinate < 150){
    lcd_fill_rectangle(xCoordinate, 60, 6, 10, ST7735_GRAY);
    xCoordinate += 10;
  }
}

void lcd_write_ch(int x, int y, char ch, FontType const font, uint16_t color, uint16_t bgcolor){
  switch (font) {
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

void lcd_draw_image(int x, int y, int w, int h, uint8_t const *data){
  int col = h - y;
  int row = w - x;
  lcd_set_address_window(x, y, x + w - 1, y + h - 1);
  i2c_burst_transfer(data, sizeof(uint16_t) * col * row);
}

void lcd_write_str(int x, int y, char const *str, FontType const font, uint16_t color, uint16_t bgcolor){
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
