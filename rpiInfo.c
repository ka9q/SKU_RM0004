// Completely rewritten from original SKU_RM0004 by Phil Karn, Oct 2023
// Placed back under the GPL or whatever the original code was

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <regex.h>
#include <sys/time.h>
#include <time.h>

#include "st7735.h"
#include "rpiInfo.h"

char const *Info = "/proc/net/dev";
char const *Interface = "eth0";

// Get the IP address of wlan0 or eth0
char const * get_ip_address(char const *interface){
  struct ifreq ifr;
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  // I want to get an IPv4 IP address
  ifr.ifr_addr.sa_family = AF_INET;
  // I want IP address attached to "eth0"
  strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
  int symbol=ioctl(fd, SIOCGIFADDR, &ifr);
  close(fd);
  if(symbol==0)
    return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
  else 
    return  "xxx.xxx.xxx.xxx";
}

// get ram memory
int get_cpu_memory(float *Totalram,float *freeram){
  struct sysinfo s_info;

  if(sysinfo(&s_info)==0){            //Get memory information
    FILE* fp=fopen("/proc/meminfo","r");
    if(fp==NULL)
      return -1;
    char buffer[100];
    while(fgets(buffer,sizeof(buffer),fp)) {
      unsigned int value=0;
      char famer[100];
      if(sscanf(buffer,"%s%u",famer,&value)!=2) 
	continue;

      if(strcmp(famer,"MemTotal:") == 0){
	*Totalram=value/1000.0/1000.0;
      } else if(strcmp(famer,"MemFree:") == 0){
	*freeram=value/1000.0/1000.0;
      }
    }
    fclose(fp);    
  }   
  return 0;
}

// get sd memory
int get_sd_memory(uint32_t *MemSize, uint32_t *freesize){
  struct statfs diskInfo;
  if(statfs("/",&diskInfo) == -1)
    return -1;

  unsigned long long blocksize = diskInfo.f_bsize;// The number of bytes per block
  unsigned long long totalsize = blocksize * diskInfo.f_blocks;//Total number of bytes	
  *MemSize=(unsigned int)(totalsize>>30);
  
  unsigned long long size = blocksize * diskInfo.f_bfree; //Now let's figure out how much space we have left
  *freesize = size>>30;
  *freesize = *MemSize - *freesize;
  return 0;
}


// get hard disk memory
int get_hard_disk_memory(int *diskMemSize, int *useMemSize){

  FILE *fd = popen("df -l | grep /dev/sda | awk '{printf \"%s\", $(2)}'","r"); 
  if(fd == NULL)
    return -1;

  char diskMembuff[10];
  fgets(diskMembuff,sizeof(diskMembuff),fd);
  fclose(fd);

  fd = popen("df -l | grep /dev/sda | awk '{printf \"%s\", $(3)}'","r"); 
  if(fd == NULL)
    return -1;
  char useMembuff[10];
  fgets(useMembuff,sizeof(useMembuff),fd);
  fclose(fd);

  *diskMemSize = atoi(diskMembuff)/1024/1024;
  *useMemSize  = atoi(useMembuff)/1024/1024;
  return 0;
}

// get temperature
float get_temperature(int type){
  FILE *fd = fopen("/sys/class/thermal/thermal_zone0/temp","r");

  char buff[10];
  fgets(buff,sizeof(buff),fd);

  unsigned int temp;
  sscanf(buff, "%d", &temp);
  fclose(fd);
  return type == FAHRENHEIT ? temp/1000*1.8+32 : temp/1000.;    
}

// Get cpu usage
float get_cpu_message(void){
    FILE *fp = popen("top -bn1 | grep %Cpu","r");
    char line[256];
    fgets(line,sizeof(line),fp);
    pclose(fp);

    float user,sys,nice,idle;
    sscanf(line,"%%Cpu(s): %f us, %f sy, %f ni, %f id",&user,&sys,&nice,&idle);
    return 100 - idle;
}


struct counts {
  unsigned long long rxbytes,rxpkts,txbytes,txpkts;
  struct timeval t;
};  

static struct counts prev;

// Get transmit bit rate; borrowed from ka9q's linkspeed.c
float get_tx_rate(char const *interface){
  int c;
  regex_t preg;
  if(0 != (c = regcomp(&preg,interface,REG_ICASE|REG_NOSUB))){
    char errbuf[1024];
    regerror(c,&preg,errbuf,sizeof(errbuf));
    return 0;
  }

  bool rdflag = false;
  struct counts curr;
  memset(&curr,0,sizeof(curr));
  gettimeofday(&curr.t,NULL);
  FILE *st = fopen(Info,"r");
  if(st == NULL)
    return 0;

  char buffer[1024];
  while(fgets(buffer,sizeof(buffer),st) != NULL){
    if(regexec(&preg,buffer,0,NULL,0) == 0){
      char const *cp = strchr(buffer,':');
      if(cp == NULL)
	return 0;

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
    // Return 0 on first interation
    float interval = (curr.t.tv_sec - prev.t.tv_sec) +  (curr.t.tv_usec - prev.t.tv_usec) * 1e-6;
    
    // First one will establish history
    float txbitrate = (curr.txbytes - prev.txbytes) / interval;
#if 0 // In case they're ever needed
    float rxbitrate = (curr.rxbytes - prev.rxbytes) / interval;
    float rxpktrate = (curr.rxpkts - prev.rxpkts) / interval;
    float txpktrate = (curr.txpkts - prev.txpkts) / interval;
#endif
    prev = curr;    

    regfree(&preg);
    return txbitrate;
  }
  prev = curr;    
  return 0;
}
