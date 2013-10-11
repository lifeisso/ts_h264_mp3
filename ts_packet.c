#include <stdio.h>

#include "Information.h"

void ts_packet_init(void)
{
	int i;

	for(i = 0; i < TS_PACKET_NUM; i++){
		memset(tp.tp_buf[i].buf, 0, sizeof(tp.tp_buf[i].buf));
		tp.tp_buf[i].used = TS_PACKET_NOUSED;
	}
	tp.tp_head = tp.tp_tail = 0;
}

void write_ts_packet(char *buf)
{
	int tail = tp.tp_tail;
	tail++;
	if(tail > TS_PACKET_NUM){
		tail = 0;
	}
	memcpy(tp.tp_buf[tail].buf, buf, 188);
	tp.tp_tail = tail;
	tp.tp_buf[tail].used = TS_PACKET_USED;
}

/*
	return 0:success
	return -1:fail
*/
int read_ts_packet(char *buf)
{
	int tail = tp.tp_tail;
	int head = tp.tp_head;

	//empty
	if(tail-head == 0 && tp.tp_buf[tail].used == TS_PACKET_NOUSED){
		return -1;	
	}

	//full
	if(tp.tp_buf[tail].used == TS_PACKET_USED){
		memcpy(buf, tp.tp_buf[tail].buf, 188);
		return 0;
	}

	memcpy(buf, tp.tp_buf[head].buf, 188);
	return 0;
}

