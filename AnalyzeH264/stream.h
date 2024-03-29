#pragma once
class stream
{
public:
    stream(void);
    ~stream(void);
};

#ifndef stream_h
#define stream_h

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

extern uint8_t *file_buff;

#define MAX_BUFFER_SIZE (50*1024*1024)
//#define TRACE 1

#ifdef TRACE
FILE *trace_fp;
#endif

// 读取h264文件，读取失败返回-1，否则返回文件大小
int readAnnexbBitStreamFile(char *fp);
void freeFilebuffer(void);

void traceInput(char *traceString, uint32_t eleValue);

#endif /* stream_h */
