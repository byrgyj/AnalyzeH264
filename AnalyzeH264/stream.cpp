#include "StdAfx.h"
#include "stream.h"


stream::stream(void)
{
}


stream::~stream(void)
{
}


uint8_t *file_buff;

// ��ȡh264�ļ�����ȡʧ�ܷ���-1�����򷵻��ļ���С
int readAnnexbBitStreamFile(char *fp)
{
    FILE *fp_h264 = fopen(fp, "rb");
    if (fp_h264 == NULL) {
        printf("��h264�ļ�ʧ��\n");
        return -1;
    }

    file_buff = (uint8_t *)malloc(MAX_BUFFER_SIZE);
    int file_size = (int)fread(file_buff, sizeof(uint8_t), MAX_BUFFER_SIZE, fp_h264);
    fclose(fp_h264);

#if TRACE
    trace_fp = fopen("trace_dec.txt", "w");
    if (trace_fp == NULL) {
        printf("��trace_dec.txt�ļ�ʧ��\n");
        return -1;
    }
#endif

    return file_size;
}

void freeFilebuffer(void)
{
    free(file_buff);
#if TRACE
    fclose(trace_fp);
#endif
}

void traceInput(char *traceString, uint32_t eleValue)
{
   /*int inputCharsCount = 0;

    putc('@', trace_fp);

    // 1.¼��traceString
    inputCharsCount += fprintf(trace_fp, " %s", traceString);
    while(inputCharsCount++ < 55) {
        putc(' ',trace_fp);
    }

    // 2.¼��eleValue
    fprintf(trace_fp, "  (%3d)\n", eleValue);

    // 3.��������������������ļ���
    fflush(trace_fp);*/
}
