CFLAGS=-Wall
SRC=rpiInfo.c st7735.c main.c

all: depend display

clean:
	rm -rf *.o display .depend

install:
	cp -f display /usr/local/sbin/display
	cp display.service /etc/systemd/system
	systemctl daemon-reload
	systemctl enable display.service
	systemctl restart display.service

depend: .depend

.depend: $(SRC)
	rm -f .depend
	$(CC) $(CFLAGS) -MM $^ > .depend

-include .depend

.PHONY:	clean all install depend

display: main.o st7735.o rpiInfo.o fonts.o
	$(CC) $(LDOPTS) -o $@ $^

