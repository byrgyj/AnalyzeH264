#pragma once
class nalu
{
public:
    nalu(void);
    ~nalu(void);
};


#ifndef nalu_h
#define nalu_h

#include <stdio.h>
#include <stdlib.h>
#include "bs.h"

#define MAX_NALU_SIZE 8000000         // ���nalu

/**
Network Abstraction Layer (NAL) unit
@see 7.3.1 NAL unit syntax
*/
typedef struct
{
    // nal header
    int forbidden_zero_bit;                     // f(1)
    int nal_ref_idc;                            // u(2)
    int nal_unit_type;                          // u(5)
    int len;                // �ʼ����nalu_size, Ȼ�󱣴�rbsp_size,���ձ���SODB�ĳ���
    uint8_t *buf;
} nalu_t;

/* 7.4.1 Table 7-1 NAL unit types */
enum nal_unit_type {
    H264_NAL_UNKNOWN         = 0,
    H264_NAL_SLICE           = 1,
    H264_NAL_DPA             = 2,
    H264_NAL_DPB             = 3,
    H264_NAL_DPC             = 4,
    H264_NAL_IDR_SLICE       = 5,
    H264_NAL_SEI             = 6,
    H264_NAL_SPS             = 7,
    H264_NAL_PPS             = 8,
    H264_NAL_AUD             = 9,
    H264_NAL_END_SEQUENCE    = 10,
    H264_NAL_END_STREAM      = 11,
    H264_NAL_FILLER_DATA     = 12,
    H264_NAL_SPS_EXT         = 13,
    H264_NAL_AUXILIARY_SLICE = 19,
};

int find_nal_unit(nalu_t *nalu, int buff_size, int *curr_nal_start, int *curr_find_index);
void read_nal_unit(nalu_t *nalu);

int nal_to_rbsp(nalu_t *nalu);
int rbsp_to_sodb(nalu_t *nalu);

// ��rbsp_trailing_bits()֮ǰ�Ƿ��и�������
int more_rbsp_data(bs_t *b);

nalu_t *allocNalu(int buff_size);
void freeNalu(nalu_t *nalu);

#endif /* nalu_h */
