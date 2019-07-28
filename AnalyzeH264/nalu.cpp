#include "StdAfx.h"
#include "nalu.h"


nalu::nalu(void)
{
}


nalu::~nalu(void)
{
}

#include <assert.h>
#include "stream.h"
#include "parset.h"
#include "slice.h"

extern slice_t *currentSlice;

/**
 �ҵ�h264�����е�nalu
 [h264Э���ĵ�λ��]��Annex B
 
 @param nalu �����ҵ���nalu->buf
 @param buff_size ������С
 @param curr_nal_start ��ǰ�ҵ���nalu����ʼλ��
 @param curr_find_index ��ǰ��ȡ��ָ��λ��
 @return nalu�Ĵ�С
 */
int find_nal_unit(nalu_t *nalu, int buff_size, int *curr_nal_start, int *curr_find_index) {
    
    int *i = curr_find_index;
    
    //( next_bits( 24 ) != 0x000001 && next_bits( 32 ) != 0x00000001 )
    // Ѱ����ʼ�룬ֻҪ��һλ�����㣬���������Ѱ��
    while (
           (file_buff[*i] != 0x00 || file_buff[*i+1] != 0x00 || file_buff[*i+2] != 0x01) &&
           (file_buff[*i] != 0x00 || file_buff[*i+1] != 0x00 || file_buff[*i+2] != 0x00 || file_buff[*i+3] != 0x01)
           ) {
        
        *i = *i + 1;
        if (*i+3 > buff_size) {return 0;} // û���ҵ����˳�����
    }
    
    // �ҵ���ʼ�룬�ж��������0x000001������0x00000001���򽫶�ȡ������1
    // if( next_bits( 24 ) != 0x000001 )
    if (file_buff[*i] != 0x00 || file_buff[*i+1] != 0x00 || file_buff[*i+2] != 0x01) {
        
        *i = *i + 1; // ��ȡ������1
    }
    
    *i += 3; // ��ȡ������3
    *curr_nal_start = *i;
    
    // ����nalu����
    int j = 0;
    // Ѱ�ҽ�β
    //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
    while (
           (file_buff[*i] != 0x00 || file_buff[*i+1] != 0x00 || file_buff[*i+2] != 0x00) &&
           (file_buff[*i] != 0x00 || file_buff[*i+1] != 0x00 || file_buff[*i+2] != 0x01)
           ) {
        
        nalu->buf[j] = file_buff[*i]; // ����ȡ����nalu�����ȫ�ֱ���nalu����
        j++;
        *i = *i +1;
        if (*i+3 >= buff_size) { // Ѱ�ҵ��ļ���β
            
            nalu->buf[j] = file_buff[*i];
            nalu->buf[j+1] = file_buff[*i+1];
            nalu->buf[j+2] = file_buff[*i+2];
            nalu->buf[j+3] = file_buff[*i+3];
            return buff_size - *curr_nal_start;
        }
    }
    
    return *curr_find_index - *curr_nal_start;
}

/**
 ��ȡһ��nalu
 @see 7.3.1 NAL unit syntax
 @see 7.4.1 NAL unit semantics
 */
void read_nal_unit(nalu_t *nalu)
{
    int nalu_size = nalu->len;
    
    // 1.ȥ��nalu�е�emulation_prevention_three_byte��0x03
    nalu->len = nal_to_rbsp(nalu);
    
    // 2.��ʼ������ض�ȡ���߾��
    bs_t *bs = bs_new(nalu->buf, nalu->len);
    
    // 3. ��ȡnal header 7.3.1
    nalu->forbidden_zero_bit = bs_read_u(bs, 1, NULL);
    nalu->nal_ref_idc = bs_read_u(bs, 2, NULL);
    nalu->nal_unit_type = bs_read_u(bs, 5, NULL);
    
    printf("\tnal->forbidden_zero_bit: %d\n", nalu->forbidden_zero_bit);
    printf("\tnal->nal_ref_idc: %d\n", nalu->nal_ref_idc);
    printf("\tnal->nal_unit_type: %d\n", nalu->nal_unit_type);

#if TRACE
    fprintf (trace_fp, "\n\nAnnex B NALU len %d, forbidden_bit %d, nal_reference_idc %d, nal_unit_type %d\n\n", nalu_size, nalu->forbidden_zero_bit, nalu->nal_ref_idc, nalu->nal_unit_type);
    fflush (trace_fp);
#endif
    
    switch (nalu->nal_unit_type)
    {
        case H264_NAL_SPS:
            nalu->len = rbsp_to_sodb(nalu);
            processSPS(bs);
            break;
            
        case H264_NAL_PPS:
            nalu->len = rbsp_to_sodb(nalu);
            processPPS(bs);
            break;
            
        case H264_NAL_SLICE:
        case H264_NAL_IDR_SLICE:
            currentSlice->idr_flag = (nalu->nal_unit_type == H264_NAL_IDR_SLICE);
            currentSlice->nal_ref_idc = nalu->nal_ref_idc;
            nalu->len = rbsp_to_sodb(nalu);
            processSlice(bs);
            break;
            
        case H264_NAL_DPA:
            nalu->len = rbsp_to_sodb(nalu);
            break;
            
        case H264_NAL_DPB:
            nalu->len = rbsp_to_sodb(nalu);
            break;
            
        case H264_NAL_DPC:
            nalu->len = rbsp_to_sodb(nalu);
            break;
            
        default:
            break;
    }
    
    bs_free(bs);
}

/**
 ȥ��rbsp�е�0x03
 @see 7.3.1 NAL unit syntax
 @see 7.4.1.1 Encapsulation of an SODB within an RBSP
 @return ����ȥ��0x03��nalu�Ĵ�С
 */
int nal_to_rbsp(nalu_t *nalu)
{
    
    int nalu_size = nalu->len;
    int j = 0;
    int count = 0;
    
    // ����0x000003���03ȥ����������cabac_zero_word��βʱ��β��Ϊ0x000003�����
    for (int i = 0; i < nalu_size; i++)
    {
        
        if (count == 2 && nalu->buf[i] == 0x03)
        {
            if (i == nalu_size - 1) // ��βΪ0x000003
            {
                break; // ����ѭ��
            }
            else
            {
                i++; // ������һ��
                count = 0;
            }
        }
        
        nalu->buf[j] = nalu->buf[i];
        
        if (nalu->buf[i] == 0x00)
        {
            count++;
        }
        else
        {
            count = 0;
        }
        
        j++;
    }
    
    return j;
}

/**
 ����SODB�ĳ���
 ��ע��RBSP = SODB + trailing_bits
 */
int rbsp_to_sodb(nalu_t *nalu)
{
    int ctr_bit, bitoffset, last_byte_pos;
    bitoffset = 0;
    last_byte_pos = nalu->len - 1;
    
    // 0.��nalu->buf����ĩβ�ı��ؿ�ʼѰ��
    ctr_bit = (nalu->buf[last_byte_pos] & (0x01 << bitoffset));
    
    // 1.ѭ���ҵ�trailing_bits�е�rbsp_stop_one_bit
    while (ctr_bit == 0)
    {
        bitoffset++;
        if(bitoffset == 8)
        {
            // ��nalu->buf�б������nalu_header+RBSP������ҵ����1�ֽڵ�nalu_header������RBSP���ҽ���
            if(last_byte_pos == 1)
                printf(" Panic: All zero data sequence in RBSP \n");
            assert(last_byte_pos != 1);
            last_byte_pos -= 1;
            bitoffset = 0;
        }
        ctr_bit= nalu->buf[last_byte_pos-1] & (0x01 << bitoffset);
    }
    // ��ע��������ʼ�Ѷ�last_byte_pos����1������ʱlast_byte_pos��ʾ�����SODB��λ�ã�Ȼ��ֵ��nalu->len�õ�����SODB�Ĵ�С
    return last_byte_pos;
}

/**
 ��rbsp_trailing_bits()֮ǰ�Ƿ��и�������
 [h264Э���ĵ�λ��]��7.2 Specification of syntax functions, categories, and descriptors
 */
int more_rbsp_data(bs_t *b)
{
    // 0.�Ƿ��Ѷ���ĩβ
    if (bs_eof(b)) {return 0;}
    
    // 1.��һ����ֵ�Ƿ�Ϊ0��Ϊ0˵�����и�������
    if (bs_peek_u1(b) == 0) {return 1;}
    
    // 2.����˵����һ����ֵΪ1�����Ҫ�����Ƿ����1����rbsp_stop_one_bit��Ҳ��1�����Ƿ�ȫΪ0
    bs_t bs_temp;
    bs_clone(&bs_temp, b);
    
    // 3.�����ղŶ�ȡ�����1������ز鿴1��������б��أ�ֱ��������һ��1���������Ϊֹ
    bs_read_u1(&bs_temp);
    while(!bs_eof(&bs_temp))
    {
        if (bs_read_u1(&bs_temp) == 1) { return 1; }
    }
    
    return 0;
}

nalu_t *allocNalu(int buff_size)
{
    // calloc����ʼ����������ڴ�ռ�
    nalu_t *nalu = (nalu_t *)calloc( 1, sizeof(nalu_t));
    if (nalu == NULL) {
        fprintf(stderr, "%s\n", "AllocNALU: nalu");
        exit(-1);
    }
    
    nalu->buf = (uint8_t *)calloc(buff_size, sizeof(uint8_t));
    if (nalu->buf == NULL) {
        fprintf(stderr, "%s\n", "AllocNALU: nalu->buf");
        exit(-1);
    }
    return nalu;
}

void freeNalu(nalu_t *nalu)
{
    free(nalu);
}