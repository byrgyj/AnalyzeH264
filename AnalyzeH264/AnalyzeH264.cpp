// AnalyzeH264.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "nalu.h"
#include "stream.h"
#include "slice.h"

extern slice_t *currentSlice ;
int _tmain(int argc, _TCHAR* argv[])
{
    // 0. 读取h264文件
    int buff_size = readAnnexbBitStreamFile("silent_cif_baseline_5_frames.h264");
    printf("totalSize: %d\n", buff_size);

    // 1. 开辟nalu_t保存nalu_header和SODB
    nalu_t *nalu = allocNalu(MAX_NALU_SIZE);

    currentSlice = allocSlice();

    int nalu_i = 0;
    int curr_nal_start = 0;  // 当前找到的nalu起始位置
    int curr_find_index = 0; // 当前查找的位置索引

    // 2.找到h264码流中的各个nalu
    while ((nalu->len = find_nal_unit(nalu, buff_size, &curr_nal_start, &curr_find_index)) > 0) {
        printf("nalu: %d, start: %d, index: %d, size: %d\n", nalu_i, curr_nal_start, curr_find_index, curr_find_index - curr_nal_start);

        // 读取/解析 nalu
        read_nal_unit(nalu);

        nalu_i++;
    }

    freeSlice(currentSlice);
    freeNalu(nalu);
    freeFilebuffer();
	return 0;
}

