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
	pthread_mutex_init(&tp.tp_mutex, NULL);
}

#if 1
void write_ts_packet(char *buf)
{
	int tail = tp.tp_tail;
printf("write tail: %d\n", tail);
	
	pthread_mutex_lock(&tp.tp_mutex);
	memcpy(tp.tp_buf[tail].buf, buf, 188);
	tp.tp_buf[tail].used = TS_PACKET_USED;

	tail++;
	if(tail > TS_PACKET_NUM-1){
		tail = 0;
	}
	tp.tp_tail = tail;
	pthread_mutex_unlock(&tp.tp_mutex);
}
#else
void write_ts_packet(char *buf)
{
	FILE *fp = fopen("ts.ts", "a");

	printf("wirte\n");
	fwrite(buf, 188, 1, fp);
	fclose(fp);
}
#endif

/*
	return 0:success
	return -1:fail
*/
int read_ts_packet(char *buf)
{
	pthread_mutex_lock(&tp.tp_mutex);
	int tail = tp.tp_tail;
	int head = tp.tp_head;

	//empty
	if(tail-head == 0 && tp.tp_buf[tail].used == TS_PACKET_NOUSED){
		pthread_mutex_unlock(&tp.tp_mutex);
		return -1;	
	}

#if 0
	//full
	if(tp.tp_buf[tail].used == TS_PACKET_USED){
		pthread_mutex_lock(&tp.tp_mutex);
		printf("full %d\n", tail);
		memcpy(buf, tp.tp_buf[tail].buf, 188);
		tp.tp_buf[tail].used = TS_PACKET_NOUSED;

		tail--;
		if(tail > TS_PACKET_NUM-1){
			tail = 0;
			tp.tp_tail = tail;
			tp.tp_head = TS_PACKET_NUM-1;
			pthread_mutex_unlock(&tp.tp_mutex);
			return 0;
		}
		tp.tp_tail = tail;
		tp.tp_head = tail+2;

		pthread_mutex_unlock(&tp.tp_mutex);
		return 0;
	}
#else
	//full
	if(tp.tp_buf[tail].used == TS_PACKET_USED){
		printf("full %d\n", tail);
		memcpy(buf, tp.tp_buf[tail-1].buf, 188);
		tp.tp_buf[tail-1].used = TS_PACKET_NOUSED;
		tp.tp_head = tail;
		tp.tp_buf[tail].used = TS_PACKET_NOUSED;

		pthread_mutex_unlock(&tp.tp_mutex);
		return 0;
	}
#endif
	
	printf("read head: %d\n", head);
	memcpy(buf, tp.tp_buf[head].buf, 188);
	tp.tp_buf[head].used = TS_PACKET_NOUSED;

	head++;
	if(head > TS_PACKET_NUM-1){
		head = 0;
	}
	tp.tp_head = head;
	pthread_mutex_unlock(&tp.tp_mutex);
	return 0;
}

