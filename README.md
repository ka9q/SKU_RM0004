# KA9Q hacks
I got one of these rack units recently, and I couldn't help but hack on the code that drives the little OLED displays on the front of each unit. I've completely rewritten it. Among other things:
1) A hostname is much more useful than a dynamically assigned IP address.
2) I created a systemd service file so it automatically starts.
3) The disk and RAM utilization readouts aren't very useful  
4) A network transmit rate is useful, especially on a system doing real-time SDR.
5) The code and directory layout was just ugly. It needed a complete rewrite.
6) The display is slow. It needed some speedups.
7) I enlarged all the displays to use the full width of 15 characters, including the bar graphs at the bottom
8) The bar graphs take a threshold parameter. When a value exeeds it, it's shown in a different color (usually red).
9) I jettisoned the Python code. (I admit it, I just can't get into Python. The language, not the comedy troupe it's named after. I'm a diehard C programmer.)


# SKU_RM0004
The project supports running on RaspberryPi, Ubuntu, [HomeAssistant](https://github.com/UCTRONICS/UCTRONICS_RM0004_HA),You can also use Python to call compiled DLLs on these platforms.
# RaspberryPi
## Turn on i2c and set the speed
**Add the following to the /boot/config.txt file**
```bash
dtparam=i2c_arm=on,i2c_arm_baudrate=400000
```

## Turn on the button to control the shutdown function
**Add the following to the /boot/config.txt file**
```bash
dtoverlay=gpio-shutdown,gpio_pin=4,active_low=1,gpio_pull=up
```

**reboot your system**
```bash
sudo reboot
```
**Wait for the system to restart**

##  Clone SKU_RM0004 library 
```bash
git clone https://github.com/UCTRONICS/SKU_RM0004.git
```
## Compile 
```bash
cd SKU_RM0004
make
```
## Run 
```
./display
```




## Add automatic start script
**Open the rc.local file**
```bash
sudo nano /etc/rc.local
```
**Add command to the rc.local file**
```bash
cd /home/pi/SKU_RM0004
make clean 
make 
./display &
```
**reboot your system**






