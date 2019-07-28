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

// ��ȡh264�ļ�����ȡʧ�ܷ���-1�����򷵻��ļ���С
int readAnnexbBitStreamFile(char *fp);
void freeFilebuffer(void);

void traceInput(char *traceString, uint32_t eleValue);

#endif /* stream_h */
