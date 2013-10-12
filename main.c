#include <pthread.h>

#include "Information.h"
#include "Conversion.h"

static void *ThreadFun_Video(void *arg)
{
	ProcessingVideo();
	return NULL;
}

static void *ThreadFun_Audio(void *arg)
{
	ProcessingAudio();
	return NULL;
}

static void *threadfun_ts(void *arg)
{
	processingts();
	return NULL;
}

int main(void)
{
	pthread_t handle_ts;

	FinH264 =  OpenInputFile(FinH264,OUTPUTFILENAME_H264);
	FinMp3 =   OpenInputFile(FinMp3,OUTPUTFILENAME_MP3);
	FOutNEWH264 = OpenOutputFile(FOutNEWH264,OUTPUTFILENAME_NEWH264);
	FOutNEWMp3  = OpenOutputFile(FOutNEWMp3,OUTPUTFILENAME_NEWMP3);
	FOutVideoTs = OpenOutputFile(FOutVideoTs,INPUTFILENAME_TS);
	OneFrameBuf_H264 = (char *)malloc(sizeof(char) * TS_MAX_OUT_BUFF);  //Ϊȫ�ֱ��������ڴ�
	OneFrameBuf_MP3 = (char *)malloc(sizeof(char) * TS_MAX_OUT_BUFF);   //Ϊȫ�ֱ��������ڴ�
	ts_video_pes = (TsPes *)malloc(sizeof(TsPes) *1);                   //Ϊȫ����ƵPES�����ڴ�
	memset(ts_video_pes->Es,0,TS_MAX_OUT_BUFF);                         //Ϊȫ����ƵES�����ڴ�
	ts_audio_pes = (TsPes *)malloc(sizeof(TsPes) *1);                   //Ϊȫ����ƵPES�����ڴ�
	memset(ts_audio_pes->Es,0,TS_MAX_OUT_BUFF);                         //Ϊȫ����ƵES�����ڴ�
	continuity_counter.continuity_counter_pat = 0;                      //��ʼ��������
	continuity_counter.continuity_counter_pmt = 0;
	continuity_counter.continuity_counter_video = 0;
	continuity_counter.continuity_counter_video = 0;
//////////////////////////////////////////////////////////////////////////
//BEGAN PROCESSING
	//InitializeCriticalSection(&Cs);                                     //���������ٽ���
	VideoPtsIni = 0;
	Videodts = 0;
	Videopts = 0;
	VideoFrameCount = 0;
	Frame_Time = (1.0/(FPS_Video)) * 90000;
	//handle_H264 = CreateThread(NULL,0,ThreadFun_Video,NULL,0,NULL);     //������Ƶ�����߳�
    //handle_Mp3 = CreateThread(NULL,0,ThreadFun_Audio,NULL,0,NULL);      //������Ƶ�����߳�
	ts_packet_init();
	pthread_create(&handle_H264, NULL,ThreadFun_Video, NULL);     //������Ƶ�����߳�
    	//pthread_create(&handle_Mp3, NULL, ThreadFun_Audio, NULL);      //������Ƶ�����߳�
	pthread_create(&handle_ts, NULL, threadfun_ts, NULL);
	//WaitForMultipleObjects(1,&handle_H264,TRUE,INFINITE);               //һֱ�ȴ��߳̽���
printf("sstart.............\n");
	pause();
	//WaitForMultipleObjects(1,&handle_Mp3,TRUE,INFINITE);                //һֱ�ȴ��߳̽���
	//CloseHandle(handle_H264);        
	//CloseHandle(handle_Mp3);
	//DeleteCriticalSection(&Cs);                                         //ɾ���ٽ���                          
//END ORICESSING
//////////////////////////////////////////////////////////////////////////
	if (OneFrameBuf_H264)
	{
		free(OneFrameBuf_H264);
		OneFrameBuf_H264 = NULL;
	}
	if (OneFrameBuf_MP3)
	{
		free(OneFrameBuf_MP3);
		OneFrameBuf_MP3 = NULL;
	}
	if (ts_video_pes)
	{
		free(ts_video_pes);
		ts_video_pes = NULL;
	}
	if (ts_audio_pes)
	{
		free(ts_audio_pes);
		ts_audio_pes = NULL;
	}
	CloseInputFile(FinH264);
	CloseInputFile(FinMp3);
	CloesOutputFile(FOutNEWH264);
	CloesOutputFile(FOutNEWMp3);
	CloesOutputFile(FOutVideoTs);

	printf("H264 �� MP3 ת TS �ļ� �������!");
	return getchar();
}
