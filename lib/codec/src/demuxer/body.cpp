/********************************************************
Copyright (c) <2019> <copyright ErosZy>

"Anti 996" License Version 1.0 (Draft)

Permission is hereby granted to any individual or legal entity
obtaining a copy of this licensed work (including the source code,
documentation and/or related items, hereinafter collectively referred
to as the "licensed work"), free of charge, to deal with the licensed
work for any purpose, including without limitation, the rights to use,
reproduce, modify, prepare derivative works of, distribute, publish
and sublicense the licensed work, subject to the following conditions:

1. The individual or the legal entity must conspicuously display,
without modification, this License and the notice on each redistributed
or derivative copy of the Licensed Work.

2. The individual or the legal entity must strictly comply with all
applicable laws, regulations, rules and standards of the jurisdiction
relating to labor and employment where the individual is physically
located or where the individual was born or naturalized; or where the
legal entity is registered or is operating (whichever is stricter). In
case that the jurisdiction has no such laws, regulations, rules and
standards or its laws, regulations, rules and standards are
unenforceable, the individual or the legal entity are required to
comply with Core International Labor Standards.

3. The individual or the legal entity shall not induce, suggest or force
its employee(s), whether full-time or part-time, or its independent
contractor(s), in any methods, to agree in oral or written form, to
directly or indirectly restrict, weaken or relinquish his or her
rights or remedies under such laws, regulations, rules and standards
relating to labor and employment as mentioned above, no matter whether
such written or oral agreements are enforceable under the laws of the
said jurisdiction, nor shall such individual or the legal entity
limit, in any methods, the rights of its employee(s) or independent
contractor(s) from reporting or complaining to the copyright holder or
relevant authorities monitoring the compliance of the license about
its violation(s) of the said license.

THE LICENSED WORK IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN ANY WAY CONNECTION WITH THE
LICENSED WORK OR THE USE OR OTHER DEALINGS IN THE LICENSED WORK.
*********************************************************/

#include "body.h"
#include "stream/buffer.h"

//  x86 nvr 帧头格式
typedef struct _DataHeard
{
	unsigned int headSig; //帧开始标记
	unsigned short codect;//编码类型	PAYLOAD_TYPE_E
	unsigned short framet;//帧类型  h264:H264E_NALU_TYPE_E, h265:H265_NALU_TYPE
	unsigned short gapms;	//帧间距 默认 40
	unsigned short framerate;//平均帧率 默认25
	unsigned int framelen; //帧长度 不包括 格式封装的结束符的长度
}DataHeard;
struct KeyFrameparam
{
	unsigned int vwidth;//视频宽 
	unsigned int vheight;//视频高
	uint64_t     timestamp;//时间戳 utc 时间，关键帧才有
};
typedef enum
{
	H264E_NALU_PSLICE = 1, /*PSLICE types*/
	H264E_NALU_ISLICE = 5, /*ISLICE types*/
	H264E_NALU_SEI = 6, /*SEI types*/
	H264E_NALU_SPS = 7, /*SPS types*/
	H264E_NALU_PPS = 8, /*PPS types*/
	H264E_NALU_BUTT = 9,
	H264E_NALU_UNKNOW = 0x00
} H264E_NALU_TYPE_E;
typedef enum
{
	H265_NALU_PSLICE = 0,       /*PSLICE types*/
	H265_NALU_PSLICE_MAX = 18,  /*PSLICE types*/
	H265_NALU_ISLICE = 19,      /*ISLICE types*/
	H265_NALU_ISLICE_MAX = 20,  /*ISLICE types*/
	H265_NALU_VPS = 32, /*SPS types*/
	H265_NALU_SPS = 33, /*SPS types*/
	H265_NALU_PPS = 34, /*PPS types*/
	H265_NALU_MAX
} H265_NALU_TYPE;

shared_ptr<BodyValue> Body::decode(shared_ptr<Buffer> &buffer) {
  Tag tag;
  shared_ptr<BodyValue> value = make_shared<BodyValue>();

#if FLV_DEFAULT_FILE_STREAM
  for (;;) {
    if (buffer->get_length() < Body::MIN_LENGTH) {
      break;
    }

    uint32_t size = buffer->read_uint32_be(0);
    shared_ptr<Buffer> body = make_shared<Buffer>(buffer->slice(4));
    if (body->get_length() < Tag::MIN_LENGTH) {
      break;
    }

    TagValue retValue = tag.decode(body);
    if (retValue.unvalidate) {
      break;
    }

    buffer = retValue.buffer;
    retValue.buffer = make_shared<Buffer>();
    value->tags->push_back(retValue);
  }
#else
  // nvr录像文件解析
  //unsigned int nSig = buffer->read_uint32_le(0);
  DataHeard* pHead = (DataHeard *)(buffer->get_buf_ptr());
  if (pHead->headSig != 0xFE010000)
  {
	  printf("read nvr header failed sig:0x%x\r\n", pHead->headSig);
	  value->buffer = make_shared<Buffer>(buffer->slice(1));	// 读取1B
	  return value;
  }

  int nBaseHeadSize = sizeof(DataHeard);	// 16B
  bool bVideo = false;
  bool bBigFrameSize = false;
  int nHeaderSize = nBaseHeadSize;
  if (pHead->codect == 26)	// PT_H264
  {
	  bVideo = true;
	  if (pHead->framet == H264E_NALU_ISLICE || pHead->framet == H264E_NALU_SPS)
	  {
		  nHeaderSize += 16;	// sizeof KeyFrameparam
		  bBigFrameSize = true;	
	  }
  } else if (pHead->codect == 35)	// h265
  {
	  bVideo = true;
	  if (pHead->framet == H265_NALU_ISLICE || pHead->framet == H265_NALU_ISLICE_MAX || pHead->framet == H265_NALU_VPS || pHead->framet == H265_NALU_SPS)
	  {
		  nHeaderSize += 16;	// sizeof KeyFrameparam
		  bBigFrameSize = true;
	  }
  }
  else
  {
	  // 认为是audio
	  nHeaderSize += 5;
  }

  do 
  {
	  int nOneFrameLen = nHeaderSize + pHead->framelen + 4; // tail = 4
	  if (buffer->get_length() < nOneFrameLen) {
		  //printf("decode nvr header failed bufLen:%d < nOneFrameLen:%d, frameLen:%d\r\n", buffer->get_length(), nOneFrameLen, pHead->framelen);
		  value->unvalidate = true;	// 缓存不足
		  break;
	  }

	  //  read frame
	  if (!bVideo)
	  {
		  buffer = make_shared<Buffer>(buffer->slice(nOneFrameLen));
		  break; // 暂不处理音频
	  }
	 
	  // 这里暂时只处理视频
	  TagValue retValue;
	  retValue.type = VideoTag::TYPE;	// 9

	  static unsigned int nStartTimeMs = 0;
	  retValue.videoTag.frameType = 2;
	  if (pHead->codect == 26 && pHead->framet == H264E_NALU_ISLICE)
	  {
		  retValue.videoTag.frameType = 1;
	  }
	  retValue.videoTag.codecId = 7;		// h264
	  retValue.videoTag.AVCPacketType = 1;	// nalu
	  retValue.videoTag.compositionTime = nStartTimeMs + pHead->gapms;
	  retValue.videoTag.data = make_shared<Buffer>(buffer->slice(nHeaderSize, nHeaderSize + pHead->framelen)); // nalu数据部分
	  retValue.videoTag.buffer = make_shared<Buffer>();
	  retValue.timestamp = retValue.videoTag.compositionTime;

	  nStartTimeMs += pHead->gapms;
	  buffer = make_shared<Buffer>(buffer->slice(nOneFrameLen));;
	  retValue.buffer = make_shared<Buffer>();
	  value->tags->push_back(retValue);

	  // read nvr frame sig:0xfe010000, codec:26, frameset:7,gapms:1,frameLen:30, time:1,headerSize:32
	  PrintFrameParse("read nvr frame sig:0x%x, codec:%d, frameset:%d,gapms:%d,frameLen:%d, time:%d,headerSize:%d, NaluSize:%d\r\n", pHead->headSig,
		  pHead->codect, pHead->framet, pHead->gapms, pHead->framelen, retValue.timestamp, nHeaderSize, retValue.videoTag.data->get_length());
  } while (0);

#endif

  value->buffer = buffer;
  return value;
}