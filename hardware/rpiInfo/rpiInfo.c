#include "rpiInfo.h"
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
#include "st7735.h"
#include <stdlib.h>

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
#include "rpiInfo.h"

extern int i2cd;

/******
Demo for ssd1306 i2c driver for  Raspberry Pi 
******/
#include <stdio.h>
#include "st7735.h"
#include "time.h"
#include <unistd.h>



/*
* Get the IP address of wlan0 or eth0
*/

char* get_ip_address(void)
{
    int fd;
    struct ifreq ifr;
    int symbol=0;
    if (IPADDRESS_TYPE == ETH0_ADDRESS)
    {
      fd = socket(AF_INET, SOCK_DGRAM, 0);
      /* I want to get an IPv4 IP address */
      ifr.ifr_addr.sa_family = AF_INET;
      /* I want IP address attached to "eth0" */
      strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
      symbol=ioctl(fd, SIOCGIFADDR, &ifr);
      close(fd);
      if(symbol==0)
      {
        return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
      }
      else
      {
        char* buffer="xxx.xxx.xxx.xxx";
        return buffer;
      }
    }
    else if (IPADDRESS_TYPE == WLAN0_ADDRESS)
    {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        /* I want to get an IPv4 IP address */
        ifr.ifr_addr.sa_family = AF_INET;
        /* I want IP address attached to "wlan0" */
        strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);
        symbol=ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);    
        if(symbol==0)
        {
          return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);   
        }
        else
        {
          char* buffer="xxx.xxx.xxx.xxx";
          return buffer;
        }
    }
    else
    {
      char* buffer="xxx.xxx.xxx.xxx";
      return buffer;
    }
}

/*
* get ram memory
*/
void get_cpu_memory(float *Totalram,float *freeram)
{
  struct sysinfo s_info;

  unsigned int value=0;
  unsigned char buffer[100]={0};
  unsigned char famer[100]={0};
    if(sysinfo(&s_info)==0)            //Get memory information
    {
        FILE* fp=fopen("/proc/meminfo","r");
        if(fp==NULL)
        {
            return ;
        }
        while(fgets(buffer,sizeof(buffer),fp))
        {
            if(sscanf(buffer,"%s%u",famer,&value)!=2)
            {
            continue;
            }
            if(strcmp(famer,"MemTotal:")==0)
            {
             *Totalram=value/1000.0/1000.0;
            }
            else if(strcmp(famer,"MemFree:")==0)
            {
              *freeram=value/1000.0/1000.0;
            }
        }
        fclose(fp);    
    }   
}

/*
* get sd memory
*/
void get_sd_memory(uint32_t *MemSize, uint32_t *freesize)
{
    struct statfs diskInfo;
    statfs("/",&diskInfo);
    unsigned long long blocksize = diskInfo.f_bsize;// The number of bytes per block
    unsigned long long totalsize = blocksize*diskInfo.f_blocks;//Total number of bytes	
    *MemSize=(unsigned int)(totalsize>>30);


    unsigned long long size = blocksize*diskInfo.f_bfree; //Now let's figure out how much space we have left
    *freesize=size>>30;
    *freesize=*MemSize-*freesize;
}


/*
* get hard disk memory
*/
uint8_t get_hard_disk_memory(uint16_t *diskMemSize, uint16_t *useMemSize)
{
  *diskMemSize = 0;
  *useMemSize = 0;
  uint8_t diskMembuff[10] = {0};
  uint8_t useMembuff[10] = {0};
  FILE *fd = NULL;
  fd=popen("df -l | grep /dev/sda | awk '{printf \"%s\", $(2)}'","r"); 
  fgets(diskMembuff,sizeof(diskMembuff),fd);
  fclose(fd);

  fd=popen("df -l | grep /dev/sda | awk '{printf \"%s\", $(3)}'","r"); 
  fgets(useMembuff,sizeof(useMembuff),fd);
  fclose(fd);

  *diskMemSize = atoi(diskMembuff)/1024/1024;
  *useMemSize  = atoi(useMembuff)/1024/1024;
}

/*
* get temperature
*/

float get_temperature(void)
{
    FILE *fd;
    unsigned int temp;
    char buff[10] = {0};
    fd = fopen("/sys/class/thermal/thermal_zone0/temp","r");
    fgets(buff,sizeof(buff),fd);
    sscanf(buff, "%d", &temp);
    fclose(fd);
    return TEMPERATURE_TYPE == FAHRENHEIT ? temp/1000*1.8+32 : temp/1000.;    
}

/*
* Get cpu usage
*/
uint8_t get_cpu_message(void)
{
    FILE * fp;
    uint8_t usCpuBuff[5] = {0};
    uint8_t syCpubuff[5] = {0};
    int usCpu = 0;
    int syCpu = 0;

    fp=popen("top -bn1 | grep %Cpu | awk '{printf \"%.2f\", $(2)}'","r");    //Gets the load on the CPU
    fgets(usCpuBuff, sizeof(usCpuBuff),fp);                                    //Read the user CPU load
    pclose(fp);    

    fp=popen("top -bn1 | grep %Cpu | awk '{printf \"%.2f\", $(4)}'","r");    //Gets the load on the CPU
    fgets(syCpubuff, sizeof(syCpubuff),fp);                                    //Read the system CPU load
    pclose(fp);   
    usCpu = atoi(usCpuBuff);
    syCpu = atoi(syCpubuff);
    return usCpu+syCpu;
  
}

#include <regex.h>
#include <sys/time.h>

char const *interface = "eth0";
char const *info = "/proc/net/dev";

struct counts {
  unsigned long long rxbytes,rxpkts,txbytes,txpkts;
  struct timeval t;
};  

static struct counts prev;

// Get transmit bit rate; borrowed from ka9q's linkspeed.c
int get_tx_rate(void){
  int c;
  regex_t preg;
  if(0 != (c = regcomp(&preg,interface,REG_ICASE|REG_NOSUB))){
    char errbuf[1024];
    regerror(c,&preg,errbuf,sizeof(errbuf));
    return 0;
  }

  struct counts curr;
  memset(&curr,0,sizeof(curr));
  
  double rxbitrate = 0,rxpktrate = 0,txbitrate = 0,txpktrate = 0;

  bool rdflag = false;
  gettimeofday(&curr.t,NULL);
  FILE *st = fopen(info,"r");
  if(st == NULL){
    return 0;
  }
  char buffer[1024];
  while(fgets(buffer,sizeof(buffer),st) != NULL){
    if(regexec(&preg,buffer,0,NULL,0) == 0){
      char const *cp = strchr(buffer,':');
      if(cp == NULL){
	return 0;
      }
      sscanf(cp,": %llu %llu %*u %*u %*u %*u %*u %*u %llu %llu %*u %*u %*u %*u %*u %*u",
	     &curr.rxbytes,&curr.rxpkts,&curr.txbytes,&curr.txpkts);
      rdflag = true; // Successful read
      break;
    }
  }
  fclose(st);
  st = NULL;
  if(!rdflag){
    regfree(&preg);
    return 0;
  }
#if 0
  printf("rxbytes %lu rxpkts %lu txbytes %lu txpkts %lu\n",curr.rxbytes,curr.rxpkts,curr.txbytes,curr.txpkts);
#endif

  if(prev.t.tv_sec != 0){
    // First will establish history
    long long interval = (curr.t.tv_sec - prev.t.tv_sec) * 1000000 + (curr.t.tv_usec - prev.t.tv_usec);
    
    // First one will establish history
    rxbitrate = (curr.rxbytes - prev.rxbytes) / interval;
    txbitrate = (curr.txbytes - prev.txbytes) / interval;
    
    rxpktrate = (curr.rxpkts - prev.rxpkts) / interval;
    txpktrate = (curr.txpkts - prev.txpkts) / interval;
    
    prev = curr;
    regfree(&preg);
  }
  return txbitrate;
}
