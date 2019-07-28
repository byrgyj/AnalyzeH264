#include "StdAfx.h"
#include "slice.h"


slice::slice(void)
{
}


slice::~slice(void)
{
}
 slice_t *currentSlice = NULL;

void processSlice(bs_t *b)
{
    // 0.�Ƚ���slice_header
    processSliceHeader(b);
}

// ��ʼ��slice�ṹ��
slice_t *allocSlice(void)
{
    slice_t *slice = (slice_t *)calloc(1, sizeof(slice_t));
    if (slice == NULL) {
        fprintf(stderr, "%s\n", "Alloc PPS Error");
        exit(-1);
    }
    return slice;
}

// �ͷ�slice
void freeSlice(slice_t *slice)
{
    free(slice);
}