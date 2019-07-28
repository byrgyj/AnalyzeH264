#pragma once
class slice
{
public:
    slice(void);
    ~slice(void);
};


#ifndef slice_h
#define slice_h

#include <stdio.h>
#include "header.h"
#include "bs.h"

typedef enum {
    Slice_Type_P = 0,
    Slice_Type_B,
    Slice_Type_I,
    Slice_Type_SP,
    Slice_Type_SI
} Slice_Type;

typedef struct
{
    int idr_flag; // �Ƿ�ΪIDR֡
    int nal_ref_idc; // nalu->nal_ref_idc
    slice_header_t slice_header;
} slice_t;

void processSliceHeader(bs_t *b);
void processSlice(bs_t *b);

slice_t *allocSlice(void); // ��ʼ��slice�ṹ��
void freeSlice(slice_t *slice); // �ͷ�slice

#endif /* slice_h */