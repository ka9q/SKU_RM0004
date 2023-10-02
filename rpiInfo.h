#ifndef  __RPIINFO_H
#define  __RPIINFO_H

#include <stdint.h>
// temp display units
#define CELSIUS       0
#define FAHRENHEIT    1

char const * get_ip_address(char const *interface);
int get_sd_memory(uint32_t *MemSize, uint32_t *freesize);
int get_cpu_memory(float *Totalram, float *freeram);
float get_temperature(int);
float get_cpu_message(void);
int get_hard_disk_memory(int *diskMemSize, int *useMemSize);
float get_tx_rate(char const *interface); // Added by KA9Q


#endif /*__RPIINFO_H*/
