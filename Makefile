TATGET := display
CC     := gcc

OBJ := obj

mkfile_path := $(shell pwd)/$(lastword $(MAKEFILE_LIST))
dir=$(shell dirname $(mkfile_path))
$(shell mkdir -p $(dir)/$(OBJ))

SRCDIRS :=  		hardware/rpiInfo \
			hardware/st7735  

SRCS := $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))
NOT_DIR :=$(notdir $(SRCS))
OBJS := $(patsubst %.c, $(OBJ)/%.o, $(NOT_DIR)) 

INCLUDE := $(patsubst %, -I %, $(SRCDIRS))

VPATH := $(SRCDIRS)

$(TATGET):$(OBJS)
	$(CC) -o $@ $^
$(OBJS) : obj/%.o : %.c
	$(CC) -c $(INCLUDE) -o $@ $<


clean:
	rm -rf $(OBJ)
	rm -rf $(TATGET)

install:
	cp display /usr/local/sbin/display
	cp display.service /etc/systemd/system
	systemctl daemon-reload
	systemctl enable display.service
	systemctl start display.service
