// Completely rewritten from SKU_0004 by Phil Karn, KA9Q Sept 2023
// Placed back under the GPL or whatever the original license was
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "st7735.h"
#include "rpiInfo.h"

// Thresholds for bar displays
#define DISPLAY_CPU_THRESHOLD 50   // 50% used
#define DISPLAY_RAM_THRESHOLD 80   // 80% full
#define DISPLAY_TEMP_THRESHOLD 55  // 55 C
#define DISPLAY_NET_THRESHOLD 50   // 500 Mb/s
#define DISPLAY_DISK_THRESHOLD 90  // 90% full
#define TEMP_TYPE CELSIUS
#define NET_INTERFACE "eth0"
#define SHOW_RAM_AND_DISK 0       // not very useful
#define SLEEP_INTERVAL 2

void lcd_display_cpuLoad(void);
void lcd_display_ram(void);
void lcd_display_temp(int);
void lcd_display_disk(void);
void lcd_display_net(char const *);

int main(void){
  if(lcd_begin("/dev/i2c-1"))
    exit(1);
  
  (void)get_tx_rate("eth0"); // Prime the differentiator
  usleep(100000);
  lcd_fill_screen(ST7735_BLACK);
  // Host name is more useful than a dynamic IP address
  char hostname[1000];
  gethostname(hostname,sizeof(hostname));
  printo(0,0,Font_8x16,ST7735_WHITE,ST7735_BLACK, "%s",hostname);
  lcd_fill_rectangle(0, 20, ST7735_WIDTH, 5, ST7735_BLUE); // Horizontal blue bar
  while(1){
    lcd_display_cpuLoad();
    sleep(SLEEP_INTERVAL);

    lcd_display_temp(TEMP_TYPE);
    sleep(SLEEP_INTERVAL);

    lcd_display_net(NET_INTERFACE);
    sleep(SLEEP_INTERVAL);

#if SHOW_RAM_AND_DISK	  // ram & disk aren't very useful
    lcd_display_ram();
    sleep(SLEEP_INTERVAL);

    lcd_display_disk();
    sleep(SLEEP_INTERVAL);
#endif
  }
  exit(0);
}


// All print0() statements should produce 15 chars to completely overwrite the previous display

void lcd_display_cpuLoad(void){
    float cpuLoad = get_cpu_message();

    printo(0, 35, Font_11x18, ST7735_WHITE, ST7735_BLACK,"CPU:%9.1f%%",cpuLoad);
    lcd_display_percentage(cpuLoad, DISPLAY_CPU_THRESHOLD, ST7735_GREEN,ST7735_RED);
}

void lcd_display_ram(void){
    float Totalram = 0.0;
    float freeram = 0.0;
    get_cpu_memory(&Totalram, &freeram);
    
    float residue = 100.0 * (Totalram - freeram) / Totalram;
    printo(0, 35, Font_11x18, ST7735_WHITE, ST7735_BLACK, "RAM:%9.1f%%",residue);
    lcd_display_percentage(residue, DISPLAY_RAM_THRESHOLD, ST7735_GREEN,ST7735_RED);
}

void lcd_display_temp(int type){
  float temp = get_temperature(type);
  printo(0,35,Font_11x18, ST7735_WHITE, ST7735_BLACK,"Temp:%8.1f%c",temp,type == FAHRENHEIT ? 'F' : 'C');
  
  if (type == FAHRENHEIT){
    temp -= 32;
    temp /= 1.8;
  }
  lcd_display_percentage(temp, DISPLAY_TEMP_THRESHOLD, ST7735_GREEN, ST7735_RED);
}

void lcd_display_net(char const *interface){
  float tx_rate = get_tx_rate(interface);
  
  if(tx_rate < 1000)
    printo(0, 35, Font_11x18, ST7735_WHITE, ST7735_BLACK,"Tx:%7.0f b/s",tx_rate);
  else if(tx_rate < 1000000)
    printo(0, 35, Font_11x18, ST7735_WHITE, ST7735_BLACK,"Tx:%6.1f kb/s",tx_rate/1000.);
  else
    printo(0, 35, Font_11x18, ST7735_WHITE, ST7735_BLACK,"Tx:%6.1f Mb/s",tx_rate/1000000.);
  int percent = tx_rate / 10000000.;
  lcd_display_percentage(percent,DISPLAY_NET_THRESHOLD,ST7735_GREEN,ST7735_RED); // relative to 1 Gb/s
}

void lcd_display_disk(void){
    uint32_t sdMemSize = 0;
    uint32_t sdUseMemSize = 0;
    get_sd_memory(&sdMemSize, &sdUseMemSize);

    int diskMemSize = 0;
    int diskUseMemSize = 0;
    get_hard_disk_memory(&diskMemSize, &diskUseMemSize);

    int memTotal= sdMemSize + diskMemSize;
    int useMemTotal = sdUseMemSize + diskUseMemSize;
    float unused = 100. * useMemTotal / memTotal;

    printo(0, 35, Font_11x18, ST7735_WHITE, ST7735_BLACK,"Disk:%8.1f%%",unused);
    lcd_display_percentage(unused, DISPLAY_DISK_THRESHOLD, ST7735_GREEN, ST7735_RED);
}
