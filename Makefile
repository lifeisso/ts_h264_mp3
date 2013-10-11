
####################################################################################### 
#                                   Variable Define                                   #           
####################################################################################### 
#export CROSS_COMPILE=/usr/arm/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-linux-gnueabi-

#export CROSS_COMPILE=

TOPDIR= `pwd`

LDFLAGS+=-lpthread 
CFLAGS=-Wall -g2 

TARGET ?= ts

OBJS = $(patsubst %.c, %.o, $(wildcard *.c))

############################################################################


all: $(TARGET)

$(TARGET):  $(OBJS) 
	$(CROSS_COMPILE)gcc -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CROSS_COMPILE)gcc -c -o $@ $^ $(CFLAGS)

#提取wzq
wzq_config:
	echo $(@:_config=)

.PHONY:clean 
clean: 
	-@$(RM) *.o *~ $(OBJ_NAME)




