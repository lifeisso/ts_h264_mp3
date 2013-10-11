#pragma once

#include "Information.h"
#include "Mybs.h"

FILE * OpenInputFile(FILE * FinputFile,char * InputFileName);                                                                                                              //�������ļ�
FILE * OpenOutputFile(FILE * FOutputFile,char * OutputFileName);                                                                                                           //������ļ�
int CloseInputFile(FILE * FInputFile);                                                                                                                                     //�ر������ļ�
int CloesOutputFile(FILE * FOutputFile);                                                                                                                                   //�ر�����ļ�
TsPacketHeader *  CreateTsHeader(TsPacketHeader * ts_header,unsigned int PID,unsigned char play_init,unsigned char ada_field_C);                                           //����ts ͷ
int TsHeader2buffer( TsPacketHeader * ts_header,unsigned char *buffer);                                                                                                    //��tsͷ��Ϣд��buf                                           
int CreatePAT();                                                                                                                                                           //����PAT,������д���ļ�
int CreatePMT();                                                                                                                                                           //����PMT,������д���ļ�  
////////////////////////////////////////////////
//BEGAN��Ƶ������
int ProcessingVideo();                                                                                                                                                     //��Ƶ������
NALU_t *AllocNALU(int buffersize);                                                                                                                                         //����nal ��Դ
void FreeNALU(NALU_t *n);                                                                                                                                                  //�ͷ�nal ��Դ 
static int FindStartCode2 (unsigned char *Buf);                                                                                                                            //�ж�nal ǰ׺�Ƿ�Ϊ3���ֽ�
static int FindStartCode3 (unsigned char *Buf);                                                                                                                            //�ж�nal ǰ׺�Ƿ�Ϊ4���ֽ�
int GetAnnexbNALU (NALU_t *nalu);                                                                                                                                          //��дnal ���ݺ�ͷ
int H2642PES();                                                                                                                                                            //��һ֡��Ƶ���ݴ����PES��
int GetFrameType(NALU_t * n);                                                                                                                                              //�õ�H264֡����
//END��Ƶ������  
////////////////////////////////////////////////
//BEGAN��Ƶ������
int ProcessingAudio();                                                                                                                                                     //��Ƶ������
int GetFrameHead(Mp3_Header * mp3_header);                                                                                                                                 //��д��Ƶͷ 4���ֽ�  
int MP32PES();                                                                                                                                                             //��һ֡��Ƶ���ݴ����PES�� 
//END��Ƶ������
////////////////////////////////////////////////
//BEGAN-PES-2-TS
int WriteAdaptive_flags_Head(Ts_Adaptation_field  * ts_adaptation_field);                                                                                                   //��д����Ӧ�α�־֡ͷ��
int WriteAdaptive_flags_Tail(Ts_Adaptation_field  * ts_adaptation_field);                                                                                                   //��д����Ӧ�α�־֡β��
int CreateAdaptive_Ts(Ts_Adaptation_field * ts_adaptation_field,unsigned char * pointer_ts,unsigned int AdaptiveLength);                                                    //��������Ӧ��
int PES2TS(TsPes * ts_pes,unsigned int Video_Audio_PID,Ts_Adaptation_field * ts_adaptation_field_Head ,Ts_Adaptation_field * ts_adaptation_field_Tail);                     //��PES���һ��һ����TS��

//END-PES-2-TS
////////////////////////////////////////////////