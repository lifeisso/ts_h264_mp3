
#include "Conversion.h"

ts_packet tp;
FILE * OpenInputFile(FILE * FinputFile,char * InputFileName) 
{
	FinputFile = fopen(InputFileName,"rb");
	if (NULL == FinputFile)
	{
		printf("打开输入文件失败！\n");
		return NULL;
	}
	return FinputFile;
}

FILE * OpenOutputFile(FILE * FOutputFile,char * OutputFileName) 
{
	FOutputFile = fopen(OutputFileName, "wb");
	if (NULL != FOutputFile)
	{
		fclose(FOutputFile);
		fopen(OutputFileName, "w");//文件清空
		fclose(FOutputFile);
		FOutputFile = NULL;
	}
	else
	{
		printf("打开输出文件失败!\n");
	}
	FOutputFile = fopen(OutputFileName, "wb");
	return FOutputFile;
}

int CloseInputFile(FILE * FInputFile)
{
	fclose(FInputFile);
	return 1;
}
int CloesOutputFile(FILE * FOutputFile)
{ 
	fclose(FOutputFile);
	return 1;
}

TsPacketHeader * CreateTsHeader(TsPacketHeader * ts_header,unsigned int PID,unsigned char play_init,unsigned char ada_field_C)
{
	ts_header->sync_byte = TS_SYNC_BYTE;
	ts_header->tras_error = 0x00;
	ts_header->play_init = play_init;
	ts_header->tras_prio = 0x00;
	ts_header->PID = PID;
	ts_header->tras_scramb = 0x00;
	ts_header->ada_field_C = ada_field_C;

	if (PID == TS_PAT_PID)             //这是pat的包
	{
		ts_header->conti_cter = (continuity_counter.continuity_counter_pat %16);
		continuity_counter.continuity_counter_pat ++;
	}
	else if (PID == TS_PMT_PID)        //这是pmt的包
	{
		ts_header->conti_cter = (continuity_counter.continuity_counter_pmt %16);
		continuity_counter.continuity_counter_pmt ++;
	}
	else if (PID == TS_H264_PID )      //这是H264的包
	{
		ts_header->conti_cter = (continuity_counter.continuity_counter_video %16);
		continuity_counter.continuity_counter_video ++;
	}
	else if (PID == TS_MP3_PID)        //这是MP3的包
	{
		ts_header->conti_cter = (continuity_counter.continuity_counter_audio %16);
		continuity_counter.continuity_counter_audio ++;
	}
	else                               //其他包出错，或可扩展
	{
		printf("continuity_counter error packet\n");
		getchar();
	}
	return ts_header;
}

int TsHeader2buffer( TsPacketHeader * ts_header,unsigned char *buffer)
{
	buffer[0]=ts_header->sync_byte;
	buffer[1]=ts_header->tras_error<<7|ts_header->play_init<<6|ts_header->tras_prio<<5|((ts_header->PID>>8)&0x1f);
	buffer[2]=(ts_header->PID&0x00ff);
	buffer[3]=ts_header->tras_scramb<<6|ts_header->ada_field_C<<4|ts_header->conti_cter;
	return 1;
}

int CreatePAT()
{
	TsPacketHeader * ts_header;
	TsPat * ts_pat;
	unsigned char PatBuf[TS_PACKET_SIZE];
	unsigned char * pointer_pat;
	unsigned char * pat ;
	unsigned long long PAT_CRC = 0xFFFFFFFF;


	ts_header = (TsPacketHeader *)malloc(sizeof(TsPacketHeader) * 1);
	ts_pat = (TsPat * )malloc(sizeof(TsPat) * 1);
	memset(PatBuf,0xFF,TS_PACKET_SIZE);
	pointer_pat = PatBuf; 
	pat = PatBuf;
	ts_header = CreateTsHeader(ts_header,TS_PAT_PID,0x01,0x01);              //PID = 0x00,有效荷载单元起始指示符_play_init = 0x01, ada_field_C,0x01,仅有有效负载 ；
	TsHeader2buffer(ts_header,PatBuf);
	pointer_pat[4] = 0;                                                      //自适应段的长度为0
	pointer_pat += 5;
	ts_pat->table_id = 0x00;
	ts_pat->section_syntax_indicator = 0x01;
	ts_pat->zero = 0x00;
	ts_pat->reserved_1 = 0x03;                                               //设置为11；
	ts_pat->section_length = 0x0d;                                           //pat结构体长度 16个字节减去上面的3个字节
	ts_pat->transport_stream_id = 0x01;
	ts_pat->reserved_2 = 0x03;                                               //设置为11；
	ts_pat->version_number = 0x00;
	ts_pat->current_next_indicator = 0x01;                                   //当前的pat 有效
	ts_pat->section_number = 0x00;
	ts_pat->last_section_number = 0x00;
	ts_pat->program_number = 0x01;
	ts_pat->reserved_3 = 0x07;                                               //设置为111；
	ts_pat->program_map_PID = TS_PMT_PID;                                    //PMT的PID
	ts_pat->CRC_32 = PAT_CRC;                                                //传输过程中检测的一种算法值 先设定一个填充值

	pointer_pat[0] = ts_pat->table_id;
	pointer_pat[1] = ts_pat->section_syntax_indicator << 7 | ts_pat->zero  << 6 | ts_pat->reserved_1 << 4 | ((ts_pat->section_length >> 8) & 0x0F);
	pointer_pat[2] = ts_pat->section_length & 0x00FF;
	pointer_pat[3] = ts_pat->transport_stream_id >> 8;
	pointer_pat[4] = ts_pat->transport_stream_id & 0x00FF;
	pointer_pat[5] = ts_pat->reserved_2 << 6 | ts_pat->version_number << 1 | ts_pat->current_next_indicator;
	pointer_pat[6] = ts_pat->section_number;
	pointer_pat[7] = ts_pat->last_section_number;
	pointer_pat[8] = ts_pat->program_number>>8;
	pointer_pat[9] = ts_pat->program_number & 0x00FF;
	pointer_pat[10]= ts_pat->reserved_3 << 5 | ((ts_pat->program_map_PID >> 8) & 0x0F);
	pointer_pat[11]= ts_pat->program_map_PID & 0x00FF;
	pointer_pat += 12;

	PAT_CRC = Zwg_ntohl(calc_crc32(pat + 5, pointer_pat - pat - 5));
	memcpy(pointer_pat, (unsigned char *)&PAT_CRC, 4);
	write_ts_packet(PatBuf);
	//fwrite(PatBuf,188,1,FOutVideoTs);                                           //将PAT包写入文件
	return 1;
}

int CreatePMT()
{
	TsPacketHeader * ts_header;
	TsPmt * ts_pmt;
	unsigned char PmtBuf[TS_PACKET_SIZE];
	unsigned char * pointer_pmt;
	unsigned char * pmt;
	unsigned long long PMT_CRC = 0xFFFFFFFF; 
	int len = 0;

	ts_header = (TsPacketHeader *)malloc(sizeof(TsPacketHeader) * 1);
	ts_pmt = (TsPmt * )malloc(sizeof(TsPmt) * 1);
	memset(PmtBuf,0xFF,TS_PACKET_SIZE);                                      //将一个包填成0xFF
	pointer_pmt = PmtBuf; 
	pmt = PmtBuf;

	ts_header = CreateTsHeader(ts_header,TS_PMT_PID,0x01,0x01);         //PID = 0x00,有效荷载单元起始指示符_play_init = 0x01, ada_field_C,0x01,仅有有效负载；
	TsHeader2buffer(ts_header,PmtBuf);
	pointer_pmt[4] = 0;                                                      //自适应段的长度为0
	pointer_pmt += 5;
	ts_pmt->table_id = 0x02;
	ts_pmt->section_syntax_indicator = 0x01;
	ts_pmt->zero = 0x00;
	ts_pmt->reserved_1 = 0x03;
	ts_pmt->section_length = 0x17;                                           //PMT结构体长度 16 + 5 + 5个字节减去上面的3个字节
	ts_pmt->program_number = 01;                                             //只有一个节目
	ts_pmt->reserved_2 = 0x03;
	ts_pmt->version_number = 0x00;
	ts_pmt->current_next_indicator = 0x01;                                   //当前的PMT有效
	ts_pmt->section_number = 0x00;
	ts_pmt->last_section_number = 0x00;
	ts_pmt->reserved_3 = 0x07;
	ts_pmt->PCR_PID = TS_H264_PID ;                                          //视频PID                                   
	ts_pmt->reserved_4 = 0x0F;
	ts_pmt->program_info_length = 0x00;                                      //后面无 节目信息描述
	ts_pmt->stream_type_video = PMT_STREAM_TYPE_VIDEO;                       //视频的类型
	ts_pmt->reserved_5_video = 0x07;
	ts_pmt->elementary_PID_video = TS_H264_PID;                              //视频的PID
	ts_pmt->reserved_6_video= 0x0F;
	ts_pmt->ES_info_length_video = 0x00;                                     //视频无跟随的相关信息
	ts_pmt->stream_type_audio = PMT_STREAM_TYPE_AUDIO;                       //音频类型
	ts_pmt->reserved_5_audio = 0x07;
	ts_pmt->elementary_PID_audio = TS_MP3_PID;                               //音频PID 
	ts_pmt->reserved_6_audio = 0x0F;
	ts_pmt->ES_info_length_audio = 0x00;                                     //音频无跟随的相关信息
	ts_pmt->CRC_32 = PMT_CRC; 

	pointer_pmt[0] = ts_pmt->table_id;
	pointer_pmt[1] = ts_pmt->section_syntax_indicator << 7 | ts_pmt->zero  << 6 | ts_pmt->reserved_1 << 4 | ((ts_pmt->section_length >> 8) & 0x0F);
	pointer_pmt[2] = ts_pmt->section_length & 0x00FF;
	pointer_pmt[3] = ts_pmt->program_number >> 8;
	pointer_pmt[4] = ts_pmt->program_number & 0x00FF;
	pointer_pmt[5] = ts_pmt->reserved_2 << 6 | ts_pmt->version_number << 1 | ts_pmt->current_next_indicator;
	pointer_pmt[6] = ts_pmt->section_number;
	pointer_pmt[7] = ts_pmt->last_section_number;
	pointer_pmt[8] = ts_pmt->reserved_3 << 5  | ((ts_pmt->PCR_PID >> 8) & 0x1F);
	pointer_pmt[9] = ts_pmt->PCR_PID & 0x0FF;
	pointer_pmt[10]= ts_pmt->reserved_4 << 4 | ((ts_pmt->program_info_length >> 8) & 0x0F);
	pointer_pmt[11]= ts_pmt->program_info_length & 0xFF;
	pointer_pmt[12]= ts_pmt->stream_type_video;                               //视频流的stream_type
	pointer_pmt[13]= ts_pmt->reserved_5_video << 5 | ((ts_pmt->elementary_PID_video >> 8 ) & 0x1F);
	pointer_pmt[14]= ts_pmt->elementary_PID_video & 0x00FF;
	pointer_pmt[15]= ts_pmt->reserved_6_video<< 4 | ((ts_pmt->ES_info_length_video >> 8) & 0x0F);
	pointer_pmt[16]= ts_pmt->ES_info_length_video & 0x0FF;
	pointer_pmt[17]= ts_pmt->stream_type_audio;                               //音频流的stream_type
	pointer_pmt[18]= ts_pmt->reserved_5_audio<< 5 | ((ts_pmt->elementary_PID_audio >> 8 ) & 0x1F);
	pointer_pmt[19]= ts_pmt->elementary_PID_audio & 0x00FF;
	pointer_pmt[20]= ts_pmt->reserved_6_audio << 4 | ((ts_pmt->ES_info_length_audio >> 8) & 0x0F);
	pointer_pmt[21]= ts_pmt->ES_info_length_audio & 0x0FF;
	pointer_pmt += 22;

	len  = pointer_pmt - pmt - 8 + 4;
	len = len > 0xffff ? 0:len;
	*(pmt + 6) = 0xb0 | (len >> 8);
	*(pmt + 7) = len;

	PMT_CRC = Zwg_ntohl(calc_crc32(pmt + 5, pointer_pmt - pmt - 5));
	memcpy(pointer_pmt, (unsigned char  *)&PMT_CRC, 4);
    
	write_ts_packet(PmtBuf);
	//fwrite(PmtBuf,188,1,FOutVideoTs);                                          //将PAT包写入文件
	return 1;
}

NALU_t *AllocNALU(int buffersize)
{
	NALU_t *n;

	if ((n = (NALU_t*)calloc (1, sizeof(NALU_t))) == NULL)
	{
		printf("AllocNALU Error: Allocate Meory To NALU_t Failed ");
		exit(0);
	}

	n->max_size = buffersize;									//Assign buffer size 

	if ((n->buf = (char*)calloc (buffersize, sizeof (char))) == NULL)
	{
		free (n);
		printf ("AllocNALU Error: Allocate Meory To NALU_t Buffer Failed ");
		exit(0);
	}
	return n;
}

void FreeNALU(NALU_t *n)
{
	if (n)
	{
		if (n->buf)
		{
			free(n->buf);
			n->buf=NULL;
		}
		free (n);
	}
}

static int FindStartCode2 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1)               //Check whether buf is 0x000001
	{
		return 0;
	}
	else 
	{
		return 1;
	}
}

static int FindStartCode3 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1)  //Check whether buf is 0x00000001
	{
		return 0;
	}
	else 
	{
		return 1;
	}
}

int GetAnnexbNALU (NALU_t *nalu)
{
	int pos = 0;                  //一个nal到下一个nal 数据移动的指针
	int StartCodeFound  = 0;      //是否找到下一个nal 的前缀
	int rewind = 0;               //判断 前缀所占字节数 3或 4
	unsigned char * Buf = NULL;
	static int info2 =0 ;
	static int info3 =0 ;

	if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL) 
	{
		printf ("GetAnnexbNALU Error: Could not allocate Buf memory\n");
	}

	nalu->startcodeprefix_len = 3;      //初始化前缀位三个字节

	if (3 != fread (Buf, 1, 3, FinH264))//从文件读取三个字节到buf
	{
		free(Buf);
		return 0;
	}
	info2 = FindStartCode2 (Buf);       //Check whether Buf is 0x000001
	if(info2 != 1) 
	{
		//If Buf is not 0x000001,then read one more byte
		if(1 != fread(Buf+3, 1, 1, FinH264))
		{
			free(Buf);
			return 0;
		}
		info3 = FindStartCode3 (Buf);   //Check whether Buf is 0x00000001
		if (info3 != 1)                 //If not the return -1
		{ 
			free(Buf);
			return -1;
		}
		else 
		{
			//If Buf is 0x00000001,set the prefix length to 4 bytes
			pos = 4;
            PrefixLen_H264 = 4;
			nalu->startcodeprefix_len = 4;
		}
	} 
	else
	{
		//If Buf is 0x000001,set the prefix length to 3 bytes
		pos = 3;
		PrefixLen_H264 = 3;
		nalu->startcodeprefix_len = 3;
	}
	//寻找下一个字符符号位， 即 寻找一个nal 从一个0000001 到下一个00000001
	StartCodeFound = 0;
	info2 = 0;
	info3 = 0;
	while (!StartCodeFound)
	{
		if (feof (FinH264))                                 //如果到了文件结尾
		{
			nalu->len = (pos-1) - nalu->startcodeprefix_len;  //从0 开始
			memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);     
			nalu->forbidden_bit = nalu->buf[0] & 0x80;      // 1 bit--10000000
			nalu->nal_reference_idc = nalu->buf[0] & 0x60;  // 2 bit--01100000
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;    // 5 bit--00011111
			free(Buf);
			return ((info3 == 1)? 4 : 3);
		}
		Buf[pos++] = fgetc (FinH264);                       //Read one char to the Buffer 一个字节一个字节从文件向后找
		info3 = FindStartCode3(&Buf[pos-4]);		        //Check whether Buf is 0x00000001 
		if(info3 != 1)
		{
			info2 = FindStartCode2(&Buf[pos-3]);            //Check whether Buf is 0x000001
		}
		StartCodeFound = (info2 == 1 || info3 == 1);        //如果找到下一个前缀
	}

	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (FinH264, rewind, SEEK_CUR))			    //将文件内部指针移动到 nal 的末尾
	{
		free(Buf);
		printf("GetAnnexbNALU Error: Cannot fseek in the bit stream file");
	}

	nalu->len = (pos + rewind) -  nalu->startcodeprefix_len;       //设置包含nal 头的数据长度
	memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//拷贝一个nal 数据到数组中
	nalu->forbidden_bit = nalu->buf[0] & 0x80;                     //1 bit  设置nal 头
	nalu->nal_reference_idc = nalu->buf[0] & 0x60;                 // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;                   // 5 bit
	free(Buf);
	return ((info3 == 1)? 4 : 3);                                               
}


int H2642PES()                              
{
	Ts_Adaptation_field  ts_adaptation_field_Head ; 
	Ts_Adaptation_field  ts_adaptation_field_Tail ; 

	ts_video_pes->packet_start_code_prefix = 0x000001;
	ts_video_pes->stream_id = TS_H264_STREAM_ID;                               //E0~EF表示是视频的,C0~DF是音频,H264-- E0
	ts_video_pes->PES_packet_length = 0 ;// OneFrameLen_H264 + 8;              //一帧数据的长度 不包含 PES包头 ,这个8 是 自适应的长度,填0 可以自动查找
	ts_video_pes->Pes_Packet_Length_Beyond = OneFrameLen_H264;

	if (OneFrameLen_H264 > 0xFFFF)                                             //如果一帧数据的大小超出界限
	{
		ts_video_pes->PES_packet_length = 0x00;
		ts_video_pes->Pes_Packet_Length_Beyond = OneFrameLen_H264;
	}
	ts_video_pes->marker_bit = 0x02;
	ts_video_pes->PES_scrambling_control = 0x00;                               //人选字段 存在，不加扰
	ts_video_pes->PES_priority = 0x00;
	ts_video_pes->data_alignment_indicator = 0x00;
	ts_video_pes->copyright = 0x00;
	ts_video_pes->original_or_copy = 0x00;
	ts_video_pes->PTS_DTS_flags = 0x03;
	ts_video_pes->ESCR_flag = 0x00;
	ts_video_pes->ES_rate_flag = 0x00;
	ts_video_pes->DSM_trick_mode_flag = 0x00;
	ts_video_pes->additional_copy_info_flag = 0x00;
	ts_video_pes->PES_CRC_flag = 0x00;
	ts_video_pes->PES_extension_flag = 0x00;
	ts_video_pes->PES_header_data_length = 0x0a;                                //后面的数据 包括了	PTS和 DTS所占的字节数

	//清 0 
	ts_video_pes->tsptsdts.pts_32_30  = 0;
	ts_video_pes->tsptsdts.pts_29_15 = 0;
	ts_video_pes->tsptsdts.pts_14_0 = 0;
	ts_video_pes->tsptsdts.dts_32_30 = 0;
	ts_video_pes->tsptsdts.dts_29_15 = 0;
	ts_video_pes->tsptsdts.dts_14_0 = 0;

	ts_video_pes->tsptsdts.reserved_1 = 0x0003;                                 //填写 pts信息
	// Videopts大于30bit，使用最高三位 
	if(Videopts > 0x7FFFFFFF)
	{
		ts_video_pes->tsptsdts.pts_32_30 = (Videopts >> 30) & 0x07;                 
		ts_video_pes->tsptsdts.marker_bit1 = 0x01;
	}
	else 
	{
		ts_video_pes->tsptsdts.marker_bit1 = 0;
	}
	// Videopts大于15bit，使用更多的位来存储
	if(Videopts > 0x7FFF)
	{
		ts_video_pes->tsptsdts.pts_29_15 = (Videopts >> 15) & 0x007FFF ;
		ts_video_pes->tsptsdts.marker_bit2 = 0x01;
	}
	else
	{
		ts_video_pes->tsptsdts.marker_bit2 = 0;
	}
	//使用最后15位
	ts_video_pes->tsptsdts.pts_14_0 = Videopts & 0x007FFF;
	ts_video_pes->tsptsdts.marker_bit3 = 0x01;


	ts_video_pes->tsptsdts.reserved_2 = 0x0001;                                 //填写 dts信息
	// Videopts大于30bit，使用最高三位 
	if(Videodts > 0x7FFFFFFF)
	{
		ts_video_pes->tsptsdts.dts_32_30 = (Videodts >> 30) & 0x07;                 
		ts_video_pes->tsptsdts.marker_bit4 = 0x01;
	}
	else 
	{
		ts_video_pes->tsptsdts.marker_bit4 = 0;
	}
	// Videopts大于15bit，使用更多的位来存储
	if(Videodts > 0x7FFF)
	{
		ts_video_pes->tsptsdts.dts_29_15 = (Videodts >> 15) & 0x007FFF ;
		ts_video_pes->tsptsdts.marker_bit5 = 0x01;
	}
	else
	{
		ts_video_pes->tsptsdts.marker_bit5 = 0;
	}
	//使用最后15位
	ts_video_pes->tsptsdts.dts_14_0 = Videodts & 0x007FFF;
	ts_video_pes->tsptsdts.marker_bit6 = 0x01;

	memcpy(ts_video_pes->Es,OneFrameBuf_H264,OneFrameLen_H264);

	//将数据存入新的H264 文件检测 中间步骤是否正确
	fwrite(ts_video_pes->Es,OneFrameLen_H264,1,FOutNEWH264);

	WriteAdaptive_flags_Head(&ts_adaptation_field_Head); //填写自适应段标志帧头
	WriteAdaptive_flags_Tail(&ts_adaptation_field_Tail); //填写自适应段标志帧尾

	//EnterCriticalSection(&Cs);
	PES2TS(ts_video_pes,TS_H264_PID,&ts_adaptation_field_Head,&ts_adaptation_field_Tail); 
	//LeaveCriticalSection(&Cs);
	return 1;
}


int GetFrameType(NALU_t * nal)
{
	bs_t s;
	int frame_type = 0; 
    bs_init( &s,OneFrameBuf_H264 + PrefixLen_H264 +1  , OneFrameLen_H264 - PrefixLen_H264 -1 );


	if (nal->nal_unit_type == NAL_SLICE || nal->nal_unit_type ==  NAL_SLICE_IDR )
	{
		/* i_first_mb */
		bs_read_ue( &s );
		/* picture type */
        frame_type =  bs_read_ue( &s );
		switch(frame_type)
		{
		case 0: case 5: /* P */
			//printf("当前帧是 P 帧！\n");
			VideoFrameCount ++;
			if (VideoFrameCount == 1)
			{
				//Videopts = VideoPtsIni + Frame_Time;
				Videopts = 0;
				Videodts = Videopts;
			}
			else
			{
				//Videopts = Frame_Time * VideoFrameCount;
				Videopts += 3600;
				Videodts = Videopts;
			}
			break;
		case 1: case 6: /* B */
			//printf("当前帧是 B 帧！\n");
			VideoFrameCount ++;
			if (VideoFrameCount == 1)
			{
				//Videopts = VideoPtsIni + Frame_Time;
				Videopts = 0;
				Videodts = Videopts;
			}
			else
			{
				//Videopts = Frame_Time * VideoFrameCount;
				Videopts += 3600;
				Videodts = Videopts;
			}
			break;
		case 3: case 8: /* SP */
	    	//printf("当前帧是 SP 帧！\n");
		    break;
		case 2: case 7: /* I */
		    //printf("当前帧是 I 帧！\n");
			VideoFrameCount ++;
			if (VideoFrameCount == 1)
			{
			//	Videodts = VideoPtsIni;
			//	Videopts = VideoPtsIni + Frame_Time;
				Videopts = 0;
				Videodts = Videopts;
			}
			else
			{
				//Videodts = (VideoFrameCount - 1) * Frame_Time;
				//Videopts = VideoFrameCount * Frame_Time ;
				Videopts += 3600;
				Videodts = Videopts;
			}
			break;
		case 4: case 9: /* SI */
			printf("当前帧是 SI 帧！\n");
			break;
		}
	}
	return 1;
}

int ProcessingVideo()
{
	unsigned char * OneFrameBuf_H264_Temporary;
	unsigned int OneFrameLen_H264_Temporary ;
	NALU_t * n = NULL;

	OneFrameLen_H264_Temporary = 0;
	OneFrameBuf_H264_Temporary = (unsigned char *)malloc(sizeof(char )* TS_MAX_OUT_BUFF * 3);
	n = AllocNALU(TS_MAX_OUT_BUFF);                                //分配nal 资源

	while(!feof(FinH264))                                          //如果未到文件结尾
	{
		usleep(10000);
		PrefixLen_H264 = 0;
		GetAnnexbNALU(n);                                          //取出一帧数据         
		OneFrameLen_H264 = 0;
		memset(OneFrameBuf_H264,0,TS_MAX_OUT_BUFF);

	    GetFrameType(n);                                           //获取帧类型
		if (n->len < 188)
		{
			if (PrefixLen_H264 == 3)
			{
				OneFrameBuf_H264_Temporary[0 + OneFrameLen_H264_Temporary] = 0x00;
				OneFrameBuf_H264_Temporary[1 + OneFrameLen_H264_Temporary] = 0x00;
				OneFrameBuf_H264_Temporary[2 + OneFrameLen_H264_Temporary] = 0x01;
				OneFrameLen_H264_Temporary += 3;
				memcpy(OneFrameBuf_H264_Temporary + OneFrameLen_H264_Temporary ,n->buf,n->len);
				OneFrameLen_H264_Temporary += n->len;
			}
			else if (PrefixLen_H264 == 4)
			{
				OneFrameBuf_H264_Temporary[0 + OneFrameLen_H264_Temporary] = 0x00;
				OneFrameBuf_H264_Temporary[1 + OneFrameLen_H264_Temporary] = 0x00;
				OneFrameBuf_H264_Temporary[2 + OneFrameLen_H264_Temporary] = 0x00;
				OneFrameBuf_H264_Temporary[3 + OneFrameLen_H264_Temporary] = 0x01;
				OneFrameLen_H264_Temporary += 4;
				memcpy(OneFrameBuf_H264_Temporary + OneFrameLen_H264_Temporary ,n->buf,n->len);
				OneFrameLen_H264_Temporary += n->len;
			}
			else
			{
				printf("H264读取错误！\n");
			}
			continue;
		}
		else
		{
			if (PrefixLen_H264 == 3)
			{
				OneFrameBuf_H264_Temporary[0 + OneFrameLen_H264_Temporary] = 0x00;
				OneFrameBuf_H264_Temporary[1 + OneFrameLen_H264_Temporary] = 0x00;
				OneFrameBuf_H264_Temporary[2 + OneFrameLen_H264_Temporary] = 0x01;
				OneFrameLen_H264_Temporary += 3;
				memcpy(OneFrameBuf_H264_Temporary + OneFrameLen_H264_Temporary ,n->buf,n->len);
				OneFrameLen_H264_Temporary += n->len;
			}
			else if (PrefixLen_H264 == 4)
			{
				OneFrameBuf_H264_Temporary[0 + OneFrameLen_H264_Temporary] = 0x00;
				OneFrameBuf_H264_Temporary[1 + OneFrameLen_H264_Temporary] = 0x00;
				OneFrameBuf_H264_Temporary[2 + OneFrameLen_H264_Temporary] = 0x00;
				OneFrameBuf_H264_Temporary[3 + OneFrameLen_H264_Temporary] = 0x01;
				OneFrameLen_H264_Temporary += 4;
				memcpy(OneFrameBuf_H264_Temporary + OneFrameLen_H264_Temporary ,n->buf,n->len);
				OneFrameLen_H264_Temporary += n->len;
			}
			else
			{
				printf("H264读取错误！\n");
			}
			memcpy(OneFrameBuf_H264,OneFrameBuf_H264_Temporary,OneFrameLen_H264_Temporary);
			OneFrameLen_H264 = OneFrameLen_H264_Temporary;
			H2642PES();                                         //开始将一帧或几帧数据填入 PES结构体
			memset(OneFrameBuf_H264_Temporary ,0,TS_MAX_OUT_BUFF * 3);
	
			OneFrameLen_H264_Temporary = 0;
		}
	}

	if (OneFrameBuf_H264_Temporary)
	{
		free( OneFrameBuf_H264_Temporary);
		OneFrameBuf_H264_Temporary  = NULL;
	}
	FreeNALU(n);                                                //释放nal 资源  
	return 1;
}

int GetFrameHead(Mp3_Header * mp3_header )
{
	unsigned char mp3headerbuf[4];

	int error = 0;
	OneFrameLen_Mp3 = 0;
	Bitrate_Mp3 = 0.0;
	Sampling_Frequency_Mp3 = 0;
	
	memset(mp3headerbuf,0,4);
	if (feof(FinMp3))                                            //如果到文件末尾
	{
		return 0;
	}
	else
	{            
		error =  fread (mp3headerbuf, 1, 4, FinMp3);              //从文件读取四个字节到buf
		if (error < 0)             
		{
			printf("读取MP3头数据失败\n");
			return 0;
		}
		if (error = 0 )
		{
			//文件结束
		}
		else
		{
			//文件读取正常
		}
	}
	mp3_header->sync = (mp3headerbuf[0] << 3)| (mp3headerbuf[1] >> 5) & 0x07;
	mp3_header->version = (mp3headerbuf[1] >> 3 ) & 0x03;
	mp3_header->layer = ( mp3headerbuf[1] >> 1) & 0x03;
	mp3_header->error_protection = mp3headerbuf[1] & 0x01;
	mp3_header->bitrate_index = (mp3headerbuf[2] >> 4 ) & 0x0F;
	mp3_header->sampling_frequency = (mp3headerbuf[2] >> 2 ) & 0x03;
	mp3_header->padding = (mp3headerbuf[2] >> 1) & 0x01;
	mp3_header->private_t = mp3headerbuf[2] & 0x01;
	mp3_header->mode = (mp3headerbuf[3] >> 6) & 0x03;
	mp3_header->mode_extension = (mp3headerbuf[3] >> 4) & 0x03;
	mp3_header->copyright = (mp3headerbuf[3] >> 3) & 0x01;
	mp3_header->original = (mp3headerbuf[3] >> 2) & 0x01;
	mp3_header->emphasis = mp3headerbuf[3] & 0x03;


	if (mp3_header->version == 0x02)                 //MPEG 2 
	{
		if (mp3_header->sampling_frequency == 0x00)                                //22.05khz
		{
			Sampling_Frequency_Mp3 = 22.05;
		}
		else if (mp3_header->sampling_frequency == 0x01)                           //24khz
		{
			Sampling_Frequency_Mp3 = 24;
		}
		else if (mp3_header->sampling_frequency == 0x02)                           //16khz
		{
			Sampling_Frequency_Mp3 = 16;
		}
		else 
		{
			printf("mp3_header->Sampling_Frequency_Mp3 == 0x03 未定义\n");            
		}

		if (mp3_header->layer == 0x01)               //layer 3            
		{
			if (mp3_header->bitrate_index == 0x00)
			{
				printf("mp3_header->bitrate_index == 0x00 free\n");
			}
			else if (mp3_header->bitrate_index == 0x01)
			{
				Bitrate_Mp3 = 8;
			}
			else if (mp3_header->bitrate_index == 0x02)
			{
				Bitrate_Mp3 = 16;
			}
			else if (mp3_header->bitrate_index == 0x03)
			{
				Bitrate_Mp3 = 24;
			}
			else if (mp3_header->bitrate_index == 0x04)
			{
				Bitrate_Mp3 = 32;
			}
			else if (mp3_header->bitrate_index == 0x05)
			{
				Bitrate_Mp3 = 40;
			}
			else if (mp3_header->bitrate_index == 0x06)
			{
				Bitrate_Mp3 = 48;
			}
			else if (mp3_header->bitrate_index == 0x07)
			{
				Bitrate_Mp3 = 56;
			}
			else if (mp3_header->bitrate_index == 0x08)
			{
				Bitrate_Mp3 = 64;
			}
			else if (mp3_header->bitrate_index == 0x09)
			{
				Bitrate_Mp3 = 80;
			}
			else if (mp3_header->bitrate_index == 0x0A)
			{
				Bitrate_Mp3 = 96;
			}
			else if (mp3_header->bitrate_index == 0x0B)
			{
				Bitrate_Mp3 = 112;
			}
			else if (mp3_header->bitrate_index == 0x0C)
			{
				Bitrate_Mp3 = 128;
			}
			else if (mp3_header->bitrate_index == 0x0D)
			{
				Bitrate_Mp3 = 144;
			}
			else if (mp3_header->bitrate_index == 0x0E)
			{
				Bitrate_Mp3 = 160;
			}
			else  //0x0F
			{
				printf("mp3_header->bitrate_index == 0x0F 不允许的值\n");
			}

			OneFrameLen_Mp3 = (72 * Bitrate_Mp3)/ Sampling_Frequency_Mp3 + mp3_header->padding ;
		}
		if (mp3_header->layer == 0x02 )               //layer 2            
		{
			if (mp3_header->bitrate_index == 0x00)
			{
				printf("mp3_header->bitrate_index == 0x00 free\n");
			}
			else if (mp3_header->bitrate_index == 0x01)
			{
				Bitrate_Mp3 = 8;
			}
			else if (mp3_header->bitrate_index == 0x02)
			{
				Bitrate_Mp3 = 16;
			}
			else if (mp3_header->bitrate_index == 0x03)
			{
				Bitrate_Mp3 = 24;
			}
			else if (mp3_header->bitrate_index == 0x04)
			{
				Bitrate_Mp3 = 32;
			}
			else if (mp3_header->bitrate_index == 0x05)
			{
				Bitrate_Mp3 = 40;
			}
			else if (mp3_header->bitrate_index == 0x06)
			{
				Bitrate_Mp3 = 48;
			}
			else if (mp3_header->bitrate_index == 0x07)
			{
				Bitrate_Mp3 = 56;
			}
			else if (mp3_header->bitrate_index == 0x08)
			{
				Bitrate_Mp3 = 64;
			}
			else if (mp3_header->bitrate_index == 0x09)
			{
				Bitrate_Mp3 = 80;
			}
			else if (mp3_header->bitrate_index == 0x0A)
			{
				Bitrate_Mp3 = 96;
			}
			else if (mp3_header->bitrate_index == 0x0B)
			{
				Bitrate_Mp3 = 112;
			}
			else if (mp3_header->bitrate_index == 0x0C)
			{
				Bitrate_Mp3 = 128;
			}
			else if (mp3_header->bitrate_index == 0x0D)
			{
				Bitrate_Mp3 = 144;
			}
			else if (mp3_header->bitrate_index == 0x0E)
			{
				Bitrate_Mp3 = 160;
			}
			else  //0x0F
			{
				printf("mp3_header->bitrate_index == 0x0F 不允许的值\n");
			}
			OneFrameLen_Mp3 = (72 * Bitrate_Mp3)/ Sampling_Frequency_Mp3 + mp3_header->padding ;
		}
		else if (mp3_header->layer == 0x03)          //layer 1
		{ 
			if (mp3_header->bitrate_index == 0x00)
			{
				printf("mp3_header->bitrate_index == 0x00 free\n");
			}
			else if (mp3_header->bitrate_index == 0x01)
			{
				Bitrate_Mp3 = 32;
			}
			else if (mp3_header->bitrate_index == 0x02)
			{
				Bitrate_Mp3  = 48;
			}
			else if (mp3_header->bitrate_index == 0x03)
			{
				Bitrate_Mp3 = 56;
			}
			else if (mp3_header->bitrate_index == 0x04)
			{
				Bitrate_Mp3 = 64;
			}
			else if (mp3_header->bitrate_index == 0x05)
			{
				Bitrate_Mp3 = 80;
			}
			else if (mp3_header->bitrate_index == 0x06)
			{
				Bitrate_Mp3 = 96;
			}
			else if (mp3_header->bitrate_index == 0x07)
			{
				Bitrate_Mp3 = 112;
			}
			else if (mp3_header->bitrate_index == 0x08)
			{
				Bitrate_Mp3 = 128;
			}
			else if (mp3_header->bitrate_index == 0x09)
			{
				Bitrate_Mp3 = 144;
			}
			else if (mp3_header->bitrate_index == 0x0A)
			{
				Bitrate_Mp3 = 160;
			}
			else if (mp3_header->bitrate_index == 0x0B)
			{
				Bitrate_Mp3 = 176;
			}
			else if (mp3_header->bitrate_index == 0x0C)
			{
				Bitrate_Mp3 = 192;
			}
			else if (mp3_header->bitrate_index == 0x0D)
			{
				Bitrate_Mp3 = 224;
			}
			else if (mp3_header->bitrate_index == 0x0E)
			{
				Bitrate_Mp3 = 256;
			}
			else  //0x0F
			{
				printf("mp3_header->bitrate_index == 0x0F 不允许的值\n");
			}
			OneFrameLen_Mp3 = (24 * Bitrate_Mp3)/ Sampling_Frequency_Mp3 + mp3_header->padding ;
		}
		else                                         //00 未定义
		{
			printf("MPEG 2 ,00 暂不做处理\n");
		}
	}
	else if (mp3_header->version == 0x03)            //MPEG 1
	{
		if (mp3_header->sampling_frequency == 0x00)                                //44.1khz
		{
			Sampling_Frequency_Mp3 = 44.1;
		}
		else if (mp3_header->sampling_frequency == 0x01)                           //48khz
		{
			Sampling_Frequency_Mp3 = 48;
		}
		else if (mp3_header->sampling_frequency == 0x02)                           //32khz
		{
			Sampling_Frequency_Mp3 = 32;
		}
		else 
		{
			printf("mp3_header->Sampling_Frequency_Mp3 == 0x03 未定义\n");            
		}

		if (mp3_header->layer == 0x01)               //layer 3            
		{
			if (mp3_header->bitrate_index == 0x00)
			{
				printf("mp3_header->bitrate_index == 0x00 free\n");
			}
			else if (mp3_header->bitrate_index == 0x01)
			{
				Bitrate_Mp3 = 32;
			}
			else if (mp3_header->bitrate_index == 0x02)
			{
				Bitrate_Mp3 = 40;
			}
			else if (mp3_header->bitrate_index == 0x03)
			{
				Bitrate_Mp3 = 48;
			}
			else if (mp3_header->bitrate_index == 0x04)
			{
				Bitrate_Mp3 = 56;
			}
			else if (mp3_header->bitrate_index == 0x05)
			{
				Bitrate_Mp3 = 64;
			}
			else if (mp3_header->bitrate_index == 0x06)
			{
				Bitrate_Mp3 = 80;
			}
			else if (mp3_header->bitrate_index == 0x07)
			{
				Bitrate_Mp3 = 96;
			}
			else if (mp3_header->bitrate_index == 0x08)
			{
				Bitrate_Mp3 = 112;
			}
			else if (mp3_header->bitrate_index == 0x09)
			{
				Bitrate_Mp3 = 128;
			}
			else if (mp3_header->bitrate_index == 0x0A)
			{
				Bitrate_Mp3 = 160;
			}
			else if (mp3_header->bitrate_index == 0x0B)
			{
				Bitrate_Mp3 = 192;
			}
			else if (mp3_header->bitrate_index == 0x0C)
			{
				Bitrate_Mp3 = 224;
			}
			else if (mp3_header->bitrate_index == 0x0D)
			{
				Bitrate_Mp3 = 256;
			}
			else if (mp3_header->bitrate_index == 0x0E)
			{
				Bitrate_Mp3 = 320;
			}
			else  //0x0F
			{
				printf("mp3_header->bitrate_index == 0x0F 不允许的值\n");
			}
			OneFrameLen_Mp3 = (144 * Bitrate_Mp3)/ Sampling_Frequency_Mp3 + mp3_header->padding ;
		}

		if (mp3_header->layer == 0x02 )              //layer 2            
		{
			if (mp3_header->bitrate_index == 0x00)
			{
				printf("mp3_header->bitrate_index == 0x00 free\n");
			}
			else if (mp3_header->bitrate_index == 0x01)
			{
				Bitrate_Mp3 = 32;
			}
			else if (mp3_header->bitrate_index == 0x02)
			{
				Bitrate_Mp3 = 48;
			}
			else if (mp3_header->bitrate_index == 0x03)
			{
				Bitrate_Mp3 = 56;
			}
			else if (mp3_header->bitrate_index == 0x04)
			{
				Bitrate_Mp3 = 64;
			}
			else if (mp3_header->bitrate_index == 0x05)
			{
				Bitrate_Mp3 = 80;
			}
			else if (mp3_header->bitrate_index == 0x06)
			{
				Bitrate_Mp3 = 96;
			}
			else if (mp3_header->bitrate_index == 0x07)
			{
				Bitrate_Mp3 = 112;
			}
			else if (mp3_header->bitrate_index == 0x08)
			{
				Bitrate_Mp3 = 128;
			}
			else if (mp3_header->bitrate_index == 0x09)
			{
				Bitrate_Mp3 = 160;
			}
			else if (mp3_header->bitrate_index == 0x0A)
			{
				Bitrate_Mp3 = 192;
			}
			else if (mp3_header->bitrate_index == 0x0B)
			{
				Bitrate_Mp3 = 224;
			}
			else if (mp3_header->bitrate_index == 0x0C)
			{
				Bitrate_Mp3 = 256;
			}
			else if (mp3_header->bitrate_index == 0x0D)
			{
				Bitrate_Mp3 = 320;
			}
			else if (mp3_header->bitrate_index == 0x0E)
			{
				Bitrate_Mp3 = 384;
			}
			else  //0x0F
			{
				printf("mp3_header->bitrate_index == 0x0F 不允许的值\n");
			}
			OneFrameLen_Mp3 = (144 * Bitrate_Mp3)/ Sampling_Frequency_Mp3 + mp3_header->padding ;
		}
		else if (mp3_header->layer == 0x03)          //layer 1
		{ 
			if (mp3_header->bitrate_index == 0x00)
			{
				printf("mp3_header->bitrate_index == 0x00 free\n");
			}
			else if (mp3_header->bitrate_index == 0x01)
			{
				Bitrate_Mp3 = 32;
			}
			else if (mp3_header->bitrate_index == 0x02)
			{
				Bitrate_Mp3 = 64;
			}
			else if (mp3_header->bitrate_index == 0x03)
			{
				Bitrate_Mp3 = 96;
			}
			else if (mp3_header->bitrate_index == 0x04)
			{
				Bitrate_Mp3 = 128;
			}
			else if (mp3_header->bitrate_index == 0x05)
			{
				Bitrate_Mp3 = 160;
			}
			else if (mp3_header->bitrate_index == 0x06)
			{
				Bitrate_Mp3 = 192;
			}
			else if (mp3_header->bitrate_index == 0x07)
			{
				Bitrate_Mp3 = 224;
			}
			else if (mp3_header->bitrate_index == 0x08)
			{
				Bitrate_Mp3 = 256;
			}
			else if (mp3_header->bitrate_index == 0x09)
			{
				Bitrate_Mp3 = 288;
			}
			else if (mp3_header->bitrate_index == 0x0A)
			{
				Bitrate_Mp3 = 320;
			}
			else if (mp3_header->bitrate_index == 0x0B)
			{
				Bitrate_Mp3 = 352;
			}
			else if (mp3_header->bitrate_index == 0x0C)
			{
				Bitrate_Mp3 = 384;
			}
			else if (mp3_header->bitrate_index == 0x0D)
			{
				Bitrate_Mp3 = 416;
			}
			else if (mp3_header->bitrate_index == 0x0E)
			{
				Bitrate_Mp3 = 448;
			}
			else  //0x0F
			{
				printf("mp3_header->bitrate_index == 0x0F 不允许的值\n");
			}
			OneFrameLen_Mp3 = (48 * Bitrate_Mp3)/ Sampling_Frequency_Mp3 + mp3_header->padding ;
		}
		else                                         //00 未定义
		{
			printf("MPEG 1 ,00 暂不做处理\n");
		}
	}
	else if(mp3_header->version == 0x00)             //MPEG 2.5
	{
		if (mp3_header->layer == 0x01 || mp3_header->layer == 0x02 )               //layer 3  //layer 2            
		{
			printf("MPEG2.5 ,layer 3 /layer 2 暂不做处理");
		}
		else if (mp3_header->layer == 0x03)          //layer 1
		{ 
			printf("MPEG2.5 ,layer 1 暂不做处理");
		}
		else                                         //00 未定义
		{
			printf("MPEG 2.5 ,00 暂不做处理\n");
		}
	}
	else                                             //01 未定义 
	{
		printf("暂不处理\n");
	}
	memcpy(OneFrameBuf_MP3,mp3headerbuf,4);          //将头写入数据
	return 1;
}

int MP32PES()                                                 
{
	Ts_Adaptation_field  ts_adaptation_field_Head ; 
	Ts_Adaptation_field  ts_adaptation_field_Tail ; 

	if (OneFrame_Mp3_Num == 0 )
	{
		OneFrameLen_Mp3_Fixed = OneFrameLen_Mp3;                               //防止后面的一帧长度 读错 所用
	}
	OneFrame_Mp3_Num ++;
	ts_audio_pes->packet_start_code_prefix = 0x000001;
	ts_audio_pes->stream_id = TS_MP3_STREAM_ID;                                //E0~EF表示是视频的,C0~DF是音频,H264-- E0
	ts_audio_pes->PES_packet_length = 0 ; // OneFrameLen_Mp3 + 8 ;             //一帧数据的长度 不包含 PES包头 ,8自适应段的长度
	ts_audio_pes->Pes_Packet_Length_Beyond = OneFrameLen_Mp3_Fixed;  //= OneFrameLen_Mp3;     //这里读错了一帧  
	if (OneFrameLen_Mp3 > 0xFFFF)                                              //如果一帧数据的大小超出界限
	{
		ts_audio_pes->PES_packet_length = 0x00;
		ts_audio_pes->Pes_Packet_Length_Beyond = OneFrameLen_Mp3;  
	}
	ts_audio_pes->marker_bit = 0x02;
	ts_audio_pes->PES_scrambling_control = 0x00;                               //人选字段 存在，不加扰
	ts_audio_pes->PES_priority = 0x00;
	ts_audio_pes->data_alignment_indicator = 0x00;
	ts_audio_pes->copyright = 0x00;
	ts_audio_pes->original_or_copy = 0x00;
	ts_audio_pes->PTS_DTS_flags = 0x03;
	ts_audio_pes->ESCR_flag = 0x00;
	ts_audio_pes->ES_rate_flag = 0x00;
	ts_audio_pes->DSM_trick_mode_flag = 0x00;
	ts_audio_pes->additional_copy_info_flag = 0x00;
	ts_audio_pes->PES_CRC_flag = 0x00;
	ts_audio_pes->PES_extension_flag = 0x00;
	ts_audio_pes->PES_header_data_length = 0x0a;                                //后面的数据 包括了	PTS和 DTS所占的字节数

	//清 0 
	ts_audio_pes->tsptsdts.pts_32_30  = 0;
	ts_video_pes->tsptsdts.pts_29_15 = 0;
	ts_video_pes->tsptsdts.pts_14_0 = 0;
	ts_video_pes->tsptsdts.dts_32_30 = 0;
	ts_video_pes->tsptsdts.dts_29_15 = 0;
	ts_video_pes->tsptsdts.dts_14_0 = 0;

	ts_video_pes->tsptsdts.reserved_1 = 0x03;                                 //填写 pts信息
	// Adudiopts大于30bit，使用最高三位 
	if(Adudiopts > 0x7FFFFFFF)
	{
		ts_audio_pes->tsptsdts.pts_32_30 = (Adudiopts >> 30) & 0x07;                 
		ts_audio_pes->tsptsdts.marker_bit1 = 0x01;
	}
	else 
	{
		ts_audio_pes->tsptsdts.marker_bit1 = 0;
	}
	// Videopts大于15bit，使用更多的位来存储
	if(Adudiopts > 0x7FFF)
	{
		ts_audio_pes->tsptsdts.pts_29_15 = (Adudiopts >> 15) & 0x007FFF ;
		ts_audio_pes->tsptsdts.marker_bit2 = 0x01;
	}
	else
	{
		ts_audio_pes->tsptsdts.marker_bit2 = 0;
	}
	//使用最后15位
	ts_audio_pes->tsptsdts.pts_14_0 = Adudiopts & 0x007FFF;
	ts_audio_pes->tsptsdts.marker_bit3 = 0x01;


	ts_audio_pes->tsptsdts.reserved_2 = 0x0001;                                 //填写 dts信息
	// Adudiodts大于30bit，使用最高三位 
	if(Adudiodts > 0x7FFFFFFF)
	{
		ts_audio_pes->tsptsdts.dts_32_30 = (Adudiodts >> 30) & 0x07;                 
		ts_audio_pes->tsptsdts.marker_bit4 = 0x01;
	}
	else 
	{
		ts_audio_pes->tsptsdts.marker_bit4 = 0;
	}
	// Adudiodts大于15bit，使用更多的位来存储
	if(Adudiodts > 0x7FFF)
	{
		ts_audio_pes->tsptsdts.dts_29_15 = (Adudiodts >> 15) & 0x007FFF ;
		ts_audio_pes->tsptsdts.marker_bit5 = 0x01;
	}
	else
	{
		ts_audio_pes->tsptsdts.marker_bit5 = 0;
	}
	//使用最后15位
	ts_audio_pes->tsptsdts.dts_14_0 = Adudiodts & 0x007FFF;
	ts_audio_pes->tsptsdts.marker_bit6 = 0x01;

	memcpy(ts_audio_pes->Es,OneFrameBuf_MP3,OneFrameLen_Mp3);

	//将数据存入新的H264 文件检测 中间步骤是否正确
	fwrite(ts_audio_pes->Es,OneFrameLen_Mp3,1,FOutNEWMp3);

	//填写自适应段标志
    WriteAdaptive_flags_Tail(&ts_adaptation_field_Head); //填写自适应段标志  ,这里注意 音频不要算pcr 所以都用帧尾代替就行
	WriteAdaptive_flags_Tail(&ts_adaptation_field_Tail); //填写自适应段标志帧尾

	//EnterCriticalSection(&Cs);
	PES2TS(ts_audio_pes,TS_MP3_PID, &ts_adaptation_field_Head,&ts_adaptation_field_Tail); 
	//LeaveCriticalSection(&Cs);
	Adudiopts += 2150;
	Adudiodts = Adudiopts;
	return 1;
}

int ProcessingAudio()
{
	Mp3_Header * mp3_header = NULL ;                             //分配MP3 header指针

	mp3_header = (Mp3_Header *)malloc(sizeof(Mp3_Header) * 1);

	while(!feof(FinMp3))                                         //如果未到文件末尾
	{
		OneFrameLen_Mp3 = 0;
		memset(OneFrameBuf_MP3,0,TS_MAX_OUT_BUFF);
		GetFrameHead(mp3_header); 
		fread(OneFrameBuf_MP3 + 4 ,1,OneFrameLen_Mp3 - 4,FinMp3);//读取一帧数据到buf中

		MP32PES();                                               //开始将一帧数据填入 PES结构体
	}
	if (mp3_header)
	{
		free(mp3_header);
		mp3_header = NULL;
	}
	return 1;
}

int WriteAdaptive_flags_Head(Ts_Adaptation_field  * ts_adaptation_field)
{
	//填写自适应段
	ts_adaptation_field->discontinuty_indicator = 0;
	ts_adaptation_field->random_access_indicator = 0;
	ts_adaptation_field->elementary_stream_priority_indicator = 0;
	ts_adaptation_field->PCR_flag = 1;                                          //只用到这个
	ts_adaptation_field->OPCR_flag = 0;
	ts_adaptation_field->splicing_point_flag = 0;
	ts_adaptation_field->transport_private_data_flag = 0;
	ts_adaptation_field->adaptation_field_extension_flag = 0;

	//需要自己算
	ts_adaptation_field->pcr  = Videopts * 300;
	ts_adaptation_field->adaptation_field_length = 7;                          //占用7位

	ts_adaptation_field->opcr = 0;
	ts_adaptation_field->splice_countdown = 0;
	ts_adaptation_field->private_data_len = 0;
	return 1;
}

int WriteAdaptive_flags_Tail(Ts_Adaptation_field  * ts_adaptation_field)
{
	//填写自适应段
	ts_adaptation_field->discontinuty_indicator = 0;
	ts_adaptation_field->random_access_indicator = 0;
	ts_adaptation_field->elementary_stream_priority_indicator = 0;
	ts_adaptation_field->PCR_flag = 0;                                          //只用到这个
	ts_adaptation_field->OPCR_flag = 0;
	ts_adaptation_field->splicing_point_flag = 0;
	ts_adaptation_field->transport_private_data_flag = 0;
	ts_adaptation_field->adaptation_field_extension_flag = 0;

	//需要自己算
	ts_adaptation_field->pcr  = 0;
	ts_adaptation_field->adaptation_field_length = 1;                          ////占用1位标志所用的位

	ts_adaptation_field->opcr = 0;
	ts_adaptation_field->splice_countdown = 0;
	ts_adaptation_field->private_data_len = 0;                    
	return 1;
}

int CreateAdaptive_Ts(Ts_Adaptation_field * ts_adaptation_field,unsigned char * pointer_ts,unsigned int AdaptiveLength)
{
	unsigned int CurrentAdaptiveLength = 1;                                 //当前已经用的自适应段长度  
	unsigned char Adaptiveflags = 0;                                        //自适应段的标志

	//填写自适应字段
	if (ts_adaptation_field->adaptation_field_length > 0)
	{
		pointer_ts += 1;                                                    //自适应段的一些标志所占用的1个字节
		CurrentAdaptiveLength += 1;

		if (ts_adaptation_field->discontinuty_indicator)
		{
			Adaptiveflags |= 0x80;
		}
		if (ts_adaptation_field->random_access_indicator)
		{
			Adaptiveflags |= 0x40;
		}
		if (ts_adaptation_field->elementary_stream_priority_indicator)
		{
			Adaptiveflags |= 0x20;
		}
		if (ts_adaptation_field->PCR_flag)
		{
			unsigned long long pcr_base;
			unsigned int pcr_ext;

			pcr_base = (ts_adaptation_field->pcr / 300);
			pcr_ext = (ts_adaptation_field->pcr % 300);

			Adaptiveflags |= 0x10;

			pointer_ts[0] = (pcr_base >> 25) & 0xff;
			pointer_ts[1] = (pcr_base >> 17) & 0xff;
			pointer_ts[2] = (pcr_base >> 9) & 0xff;
			pointer_ts[3] = (pcr_base >> 1) & 0xff;
			pointer_ts[4] = pcr_base << 7 | pcr_ext >> 8 | 0x7e;
			pointer_ts[5] = (pcr_ext) & 0xff;
			pointer_ts += 6;

			CurrentAdaptiveLength += 6;
		}
		if (ts_adaptation_field->OPCR_flag)
		{
			unsigned long long opcr_base;
			unsigned int opcr_ext;

			opcr_base = (ts_adaptation_field->opcr / 300);
			opcr_ext = (ts_adaptation_field->opcr % 300);

			Adaptiveflags |= 0x08;

			pointer_ts[0] = (opcr_base >> 25) & 0xff;
			pointer_ts[1] = (opcr_base >> 17) & 0xff;
			pointer_ts[2] = (opcr_base >> 9) & 0xff;
			pointer_ts[3] = (opcr_base >> 1) & 0xff;
			pointer_ts[4] = ((opcr_base << 7) & 0x80) | ((opcr_ext >> 8) & 0x01);
			pointer_ts[5] = (opcr_ext) & 0xff;
			pointer_ts += 6;
			CurrentAdaptiveLength += 6;
		}
		if (ts_adaptation_field->splicing_point_flag)
		{
			pointer_ts[0] = ts_adaptation_field->splice_countdown;

			Adaptiveflags |= 0x04;

			pointer_ts += 1;
			CurrentAdaptiveLength += 1;
		}
		if (ts_adaptation_field->private_data_len > 0)
		{
			Adaptiveflags |= 0x02;
			if (1+ ts_adaptation_field->private_data_len > AdaptiveLength - CurrentAdaptiveLength)
			{
				printf("private_data_len error !\n");
				return getchar();
			}
			else
			{
				pointer_ts[0] = ts_adaptation_field->private_data_len;
				pointer_ts += 1;
				memcpy (pointer_ts, ts_adaptation_field->private_data, ts_adaptation_field->private_data_len);
				pointer_ts += ts_adaptation_field->private_data_len;

				CurrentAdaptiveLength += (1 + ts_adaptation_field->private_data_len) ;
			}
		}
		if (ts_adaptation_field->adaptation_field_extension_flag)
		{
			Adaptiveflags |= 0x01;
			pointer_ts[1] = 1;
			pointer_ts[2] = 0;
			CurrentAdaptiveLength += 2;
		}
		TSbuf[5] = Adaptiveflags;                        //将标志放入内存
	}
	return 1;
}

int PES2TS(TsPes * ts_pes,unsigned int Video_Audio_PID ,Ts_Adaptation_field * ts_adaptation_field_Head ,Ts_Adaptation_field * ts_adaptation_field_Tail)
{
	TsPacketHeader * ts_header;
	unsigned char * pointer_ts = NULL;                                         //一个188包的指针
	unsigned int AdaptiveLength = 0;                                           //要填写0XFF的长度
	unsigned int NeafPacketCount = 0;                                          //分片包的个数
	unsigned int FirstPacketLoadLength = 0 ;                                   //分片包的第一个包的负载长度
	unsigned char * NeafBuf = NULL;                                            //分片包 总负载的指针
	unsigned int NeafPacketCount_Now = 0;                                      //当前分片包计数          
	unsigned int i = 0;                                                        //测试包正确用的循环参数，可去掉

	ts_header = (TsPacketHeader *)malloc(sizeof(TsPacketHeader) * 1);
	FirstPacketLoadLength = 188 -4 - 1 - ts_adaptation_field_Head->adaptation_field_length - 19; //计算分片包的第一个包的负载长度
	NeafPacketCount += 1;                                                                   //第一个分片包
	NeafPacketCount += (ts_pes->Pes_Packet_Length_Beyond - FirstPacketLoadLength)/ 184;     
	NeafPacketCount += 1;                                                                   //最后一个分片包
	AdaptiveLength = 188 - 4 - 1 - ((ts_pes->Pes_Packet_Length_Beyond - FirstPacketLoadLength)% 184)  ;  //要填写0XFF的长度
	if ((WritePacketNum % 40) == 0)                                                         //每40个包打一个 pat,一个pmt
	{
		CreatePAT();                                                                        //创建PAT
		CreatePMT();                                                                        //创建PMT
	}
	//开始处理第一个包,分片包的个数最少也会是两个
	memset(TSbuf,0,TS_PACKET_SIZE);   
	pointer_ts = TSbuf;
	ts_header = CreateTsHeader(ts_header,Video_Audio_PID,0x01,0x03);                         //PID = TS_H264_PID,有效荷载单元起始指示符_play_init = 0x01, ada_field_C,0x03,含有调整字段和有效负载 ；
	TsHeader2buffer(ts_header,TSbuf);
	pointer_ts += 4;
	pointer_ts[0] = ts_adaptation_field_Head->adaptation_field_length;                            //自适应字段的长度，自己填写的
	pointer_ts += 1;

	CreateAdaptive_Ts(ts_adaptation_field_Head,pointer_ts,(188 - 4 - 1 - 19));                    //填写自适应字段
	pointer_ts += ts_adaptation_field_Head->adaptation_field_length;                              //填写自适应段所需要的长度

	pointer_ts[0] = (ts_pes->packet_start_code_prefix >> 16) & 0xFF;
	pointer_ts[1] = (ts_pes->packet_start_code_prefix >> 8) & 0xFF; 
	pointer_ts[2] = ts_pes->packet_start_code_prefix & 0xFF;
	pointer_ts[3] = ts_pes->stream_id;
	pointer_ts[4] = (ts_pes->PES_packet_length >> 8) & 0xFF;
	pointer_ts[5] = ts_pes->PES_packet_length & 0xFF;
	pointer_ts[6] = ts_pes->marker_bit << 6 | ts_pes->PES_scrambling_control << 4 | ts_pes->PES_priority << 3 |
		ts_pes->data_alignment_indicator << 2 | ts_pes->copyright << 1 |ts_pes->original_or_copy;
	pointer_ts[7] = ts_pes->PTS_DTS_flags << 6 |ts_pes->ESCR_flag << 5 | ts_pes->ES_rate_flag << 4 |
		ts_pes->DSM_trick_mode_flag << 3 | ts_pes->additional_copy_info_flag << 2 | ts_pes->PES_CRC_flag << 1 | ts_pes->PES_extension_flag;
	pointer_ts[8] = ts_pes->PES_header_data_length;

	//pointer_ts[9] = (ts_pes->tsptsdts.reserved_1 << 4) | (ts_pes->tsptsdts.pts_32_30 << 1) | (ts_pes->tsptsdts.marker_bit1);    //这里不知道有什么问题，填不进去
	//pointer_ts[10]= (ts_pes->tsptsdts.pts_29_15 >> 7 ) & 0xFF;
	//pointer_ts[11]= ((((ts_pes->tsptsdts.pts_29_15 & 0x7F) << 1) | ts_pes->tsptsdts.marker_bit2)& 0xFF);  
	//pointer_ts[12]= (ts_pes->tsptsdts.pts_14_0 >> 7) & 0xFF;
	//pointer_ts[13]= (ts_pes->tsptsdts.pts_14_0 & 0x7F) | ts_pes->tsptsdts.marker_bit3;

	//pointer_ts[14]= ts_pes->tsptsdts.reserved_2 << 4 | ts_pes->tsptsdts.dts_32_30 << 1 | ts_pes->tsptsdts.marker_bit4;
	//pointer_ts[15]= (ts_pes->tsptsdts.dts_29_15 >> 7) & 0xFF;
	//pointer_ts[16]= ((((ts_pes->tsptsdts.dts_29_15 &0x7F) << 1) | ts_pes->tsptsdts.marker_bit5) & 0xFF);  
	//pointer_ts[17]= (ts_pes->tsptsdts.dts_14_0 >> 7)  &0xFF;
	//pointer_ts[18]= ts_pes->tsptsdts.dts_14_0 & 0x7F | ts_pes->tsptsdts.marker_bit6;

	if (ts_pes->stream_id == TS_H264_STREAM_ID)
	{
		pointer_ts[9] = (((0x3 << 4) | ((Videopts>> 29) & 0x0E) | 0x01) & 0xff);
		pointer_ts[10]= (((((Videopts >> 14) & 0xfffe) | 0x01) >> 8) & 0xff);
		pointer_ts[11]= ((((Videopts >> 14) & 0xfffe) | 0x01) & 0xff);
		pointer_ts[12]= (((((Videopts << 1) & 0xfffe) | 0x01) >> 8) & 0xff);
		pointer_ts[13]= ((((Videopts << 1) & 0xfffe) | 0x01) & 0xff);


		pointer_ts[14]= (((0x1<< 4) | ((Videodts >> 29) & 0x0E) | 0x01) & 0xff);
		pointer_ts[15]= (((((Videodts >> 14) & 0xfffe) | 0x01) >> 8) & 0xff);
		pointer_ts[16]= ((((Videodts >> 14) & 0xfffe) | 0x01) & 0xff);
		pointer_ts[17]= ((((Videodts << 1) & 0xfffe) | 0x01) >> 8) & 0xff;
		pointer_ts[18]= (((Videodts << 1) & 0xfffe) | 0x01) & 0xff;
	}
	else if (ts_pes->stream_id == TS_MP3_STREAM_ID)
	{
		pointer_ts[9] = (((0x3 << 4) | ((Adudiopts>> 29) & 0x0E) | 0x01) & 0xff);
		pointer_ts[10]= (((((Adudiopts >> 14) & 0xfffe) | 0x01) >> 8) & 0xff);
		pointer_ts[11]= ((((Adudiopts >> 14) & 0xfffe) | 0x01) & 0xff);
		pointer_ts[12]= (((((Adudiopts << 1) & 0xfffe) | 0x01) >> 8) & 0xff);
		pointer_ts[13]= ((((Adudiopts << 1) & 0xfffe) | 0x01) & 0xff);


		pointer_ts[14]= (((0x1<< 4) | ((Adudiodts >> 29) & 0x0E) | 0x01) & 0xff);
		pointer_ts[15]= (((((Adudiodts >> 14) & 0xfffe) | 0x01) >> 8) & 0xff);
		pointer_ts[16]= ((((Adudiodts >> 14) & 0xfffe) | 0x01) & 0xff);
		pointer_ts[17]= ((((Adudiodts << 1) & 0xfffe) | 0x01) >> 8) & 0xff;
		pointer_ts[18]= (((Adudiodts << 1) & 0xfffe) | 0x01) & 0xff;
	}
	else
	{
		printf("ts_pes->stream_id  error 0x%x \n",ts_pes->stream_id);
		return getchar();
	}
	pointer_ts += 19;
	NeafBuf = ts_pes->Es ;
	memcpy(pointer_ts,NeafBuf,FirstPacketLoadLength);  
	NeafPacketCount_Now ++;
	NeafBuf += FirstPacketLoadLength;
	ts_pes->Pes_Packet_Length_Beyond -=FirstPacketLoadLength;
	//将包写入文件
	write_ts_packet(TSbuf);
	//fwrite(TSbuf,188,1,FOutVideoTs);                                        //将一包数据写入文件
	WritePacketNum ++;                                                      //已经写入文件的包个数++


	while(ts_pes->Pes_Packet_Length_Beyond)
	{
		if ((WritePacketNum % 40) == 0)                                      //每40个包打一个 pat,一个pmt
		{
			CreatePAT();                                                     //创建PAT
			CreatePMT();                                                     //创建PMT
		}
		if(ts_pes->Pes_Packet_Length_Beyond >=184)
		{
			//处理中间包
			memset(TSbuf,0,TS_PACKET_SIZE);    
			pointer_ts = TSbuf;
			ts_header = CreateTsHeader(ts_header,Video_Audio_PID,0x00,0x01);   //PID = TS_H264_PID,不是有效荷载单元起始指示符_play_init = 0x00, ada_field_C,0x01,仅有有效负载；
			TsHeader2buffer(ts_header,TSbuf);
			pointer_ts += 4;
			memcpy(pointer_ts,NeafBuf,184); 
			NeafBuf += 184;
			ts_pes->Pes_Packet_Length_Beyond -=184;
			write_ts_packet(TSbuf);
			//fwrite(TSbuf,188,1,FOutVideoTs); 
		}
		else
		{
			if(ts_pes->Pes_Packet_Length_Beyond == 183||ts_pes->Pes_Packet_Length_Beyond == 182)
			{
				if ((WritePacketNum % 40) == 0)                                                         //每40个包打一个 pat,一个pmt
				{
					CreatePAT();                                                                        //创建PAT
					CreatePMT();                                                                        //创建PMT
				}
				memset(TSbuf,0,TS_PACKET_SIZE);  
				pointer_ts = TSbuf;
				ts_header = CreateTsHeader(ts_header,Video_Audio_PID,0x00,0x03);  //PID = TS_H264_PID,不是有效荷载单元起始指示符_play_init = 0x00, ada_field_C,0x03,含有调整字段和有效负载；
				TsHeader2buffer(ts_header,TSbuf);
				pointer_ts += 4;
				pointer_ts[0] =1;
				pointer_ts[1] =0x00 ;
				pointer_ts += 2;
				memcpy(pointer_ts,NeafBuf,182);
				NeafBuf += 182;
				ts_pes->Pes_Packet_Length_Beyond -=182;
				write_ts_packet(TSbuf);
				//fwrite(TSbuf,188,1,FOutVideoTs); 
			}
			else
			{
				if ((WritePacketNum % 40) == 0)                                                         //每40个包打一个 pat,一个pmt
				{
					CreatePAT();                                                                        //创建PAT
					CreatePMT();                                                                        //创建PMT
				}
				memset(TSbuf,0,TS_PACKET_SIZE);  
				pointer_ts = TSbuf;
				ts_header = CreateTsHeader(ts_header,Video_Audio_PID,0x00,0x03);  //PID = TS_H264_PID,不是有效荷载单元起始指示符_play_init = 0x00, ada_field_C,0x03,含有调整字段和有效负载；
				TsHeader2buffer(ts_header,TSbuf);
				pointer_ts += 4;
				pointer_ts[0] =184-ts_pes->Pes_Packet_Length_Beyond-1 ;
				pointer_ts[1] =0x00 ;
				pointer_ts += 2;
				memset(pointer_ts,0xFF,(184-ts_pes->Pes_Packet_Length_Beyond-2));
				pointer_ts+=(184-ts_pes->Pes_Packet_Length_Beyond-2);
				memcpy(pointer_ts,NeafBuf,ts_pes->Pes_Packet_Length_Beyond);
				ts_pes->Pes_Packet_Length_Beyond = 0;
				write_ts_packet(TSbuf);
				//fwrite(TSbuf,188,1,FOutVideoTs);   //将一包数据写入文件
				WritePacketNum ++;  
				return 1;
			}
		}	
		WritePacketNum ++;  
	}
	return 1;
}

