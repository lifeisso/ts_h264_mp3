#pragma once

#include "Information.h"
#include "Mybs.h"

FILE * OpenInputFile(FILE * FinputFile,char * InputFileName);                                                                                                              //打开输入文件
FILE * OpenOutputFile(FILE * FOutputFile,char * OutputFileName);                                                                                                           //打开输出文件
int CloseInputFile(FILE * FInputFile);                                                                                                                                     //关闭输入文件
int CloesOutputFile(FILE * FOutputFile);                                                                                                                                   //关闭输出文件
TsPacketHeader *  CreateTsHeader(TsPacketHeader * ts_header,unsigned int PID,unsigned char play_init,unsigned char ada_field_C);                                           //创建ts 头
int TsHeader2buffer( TsPacketHeader * ts_header,unsigned char *buffer);                                                                                                    //将ts头信息写入buf                                           
int CreatePAT();                                                                                                                                                           //创建PAT,并将其写入文件
int CreatePMT();                                                                                                                                                           //创建PMT,并将其写入文件  
////////////////////////////////////////////////
//BEGAN视频处理函数
int ProcessingVideo();                                                                                                                                                     //视频处理函数
NALU_t *AllocNALU(int buffersize);                                                                                                                                         //分配nal 资源
void FreeNALU(NALU_t *n);                                                                                                                                                  //释放nal 资源 
static int FindStartCode2 (unsigned char *Buf);                                                                                                                            //判断nal 前缀是否为3个字节
static int FindStartCode3 (unsigned char *Buf);                                                                                                                            //判断nal 前缀是否为4个字节
int GetAnnexbNALU (NALU_t *nalu);                                                                                                                                          //填写nal 数据和头
int H2642PES();                                                                                                                                                            //将一帧视频数据打包成PES包
int GetFrameType(NALU_t * n);                                                                                                                                              //得到H264帧类型
//END视频处理函数  
////////////////////////////////////////////////
//BEGAN音频处理函数
int ProcessingAudio();                                                                                                                                                     //音频处理函数
int GetFrameHead(Mp3_Header * mp3_header);                                                                                                                                 //填写音频头 4个字节  
int MP32PES();                                                                                                                                                             //将一帧音频数据打包成PES包 
//END音频处理函数
////////////////////////////////////////////////
//BEGAN-PES-2-TS
int WriteAdaptive_flags_Head(Ts_Adaptation_field  * ts_adaptation_field);                                                                                                   //填写自适应段标志帧头的
int WriteAdaptive_flags_Tail(Ts_Adaptation_field  * ts_adaptation_field);                                                                                                   //填写自适应段标志帧尾的
int CreateAdaptive_Ts(Ts_Adaptation_field * ts_adaptation_field,unsigned char * pointer_ts,unsigned int AdaptiveLength);                                                    //创建自适应段
int PES2TS(TsPes * ts_pes,unsigned int Video_Audio_PID,Ts_Adaptation_field * ts_adaptation_field_Head ,Ts_Adaptation_field * ts_adaptation_field_Tail);                     //将PES打成一个一个的TS包

//END-PES-2-TS
////////////////////////////////////////////////