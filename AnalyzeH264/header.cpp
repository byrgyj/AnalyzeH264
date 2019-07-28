#include "StdAfx.h"
#include "header.h"


header::header(void)
{
}


header::~header(void)
{
}

#include "slice.h"
#include "parset.h"
#include "frame.h"

extern slice_t *currentSlice;
extern sps_t *active_sps;  
extern pps_t *active_pps; 
#pragma mark - ��������
// ����ǰ�����䷨Ԫ��
void parse_first_three_element(bs_t *b);
void parse_rest_elememt_of_sliceHeader(bs_t *b);
void parse_ref_pic_list_modification(bs_t *b);
void alloc_ref_pic_list_modification_buffer(void);
void parse_pred_weight_table(bs_t *b);
void parse_dec_ref_pic_marking(bs_t *b);
// ����CeilLog2(inputVal)
unsigned calculateCeilLog2(unsigned inputVal);

#pragma mark - ����ʵ��
/**
 ����slice_header������������
 1.�Ƚ���ͷ����Ԫ��
 2.ȥ���������
 3.����ʣ��ľ䷨Ԫ��
 */
void processSliceHeader(bs_t *b)
{
    // 0.����ǰ�����䷨Ԫ��
    parse_first_three_element(b);
    // 1.���������
    activeParameterSet(currentSlice->slice_header.pic_parameter_set_id);
    // 2.����ʣ��ľ䷨Ԫ��
    parse_rest_elememt_of_sliceHeader(b);
}

/**
 ����slice_headerͷ�����䷨Ԫ��
 [h264Э���ĵ�λ��]��7.3.3 Slice header syntax
 */
void parse_first_three_element(bs_t *b)
{
    currentSlice->slice_header.first_mb_in_slice = bs_read_ue(b, "SH: first_mb_in_slice");
    
    // ��Ϊslice_typeֵΪ0~9��0~4��5~9�غ�
    int slice_type = bs_read_ue(b, "SH: slice_type");
    if (slice_type > 4) {slice_type -= 5;}
    currentSlice->slice_header.slice_type = slice_type;
    
    currentSlice->slice_header.pic_parameter_set_id = bs_read_ue(b, "SH: pic_parameter_set_id");
}

/**
 ����slice_headerʣ��䷨Ԫ��
 [h264Э���ĵ�λ��]��7.3.3 Slice header syntax
 */
void parse_rest_elememt_of_sliceHeader(bs_t *b)
{
    slice_header_t *slice_header = &currentSlice->slice_header;
    
    if (active_sps->separate_colour_plane_flag == 1) {
        slice_header->colour_plane_id = bs_read_u(b, 2, "SH: colour_plane_id");
    }else {
        slice_header->colour_plane_id = COLOR_PLANE_Y;
    }
    
    slice_header->frame_num = bs_read_u(b, active_sps->log2_max_frame_num_minus4 + 4, "SH: frame_num");
    
    // FIXME: frame_num gap processing
    
    if (active_sps->frame_mbs_only_flag) {
        slice_header->field_pic_flag = 0;
    } else {
        slice_header->field_pic_flag = bs_read_u(b, 1, "SH: field_pic_flag");
        if (slice_header->field_pic_flag) {
            slice_header->bottom_field_flag = bs_read_u(b, 1, "SH: bottom_field_flag");
        }else {
            slice_header->bottom_field_flag = 0;
        }
    }
    
    if (currentSlice->idr_flag) {
        slice_header->idr_pic_id = bs_read_ue(b, "SH: idr_pic_id");
    }
    
    if (active_sps->pic_order_cnt_type == 0)
    {
        slice_header->pic_order_cnt_lsb = bs_read_u(b, active_sps->log2_max_pic_order_cnt_lsb_minus4 + 4, "SH: pic_order_cnt_lsb");
        if (active_pps->bottom_field_pic_order_in_frame_present_flag && !slice_header->field_pic_flag) {
            slice_header->delta_pic_order_cnt_bottom = bs_read_se(b, "SH: delta_pic_order_cnt_bottom");
        }else {
            slice_header->delta_pic_order_cnt_bottom = 0;
        }
    }
    
    if (active_sps->pic_order_cnt_type == 1 &&
        !active_sps->delta_pic_order_always_zero_flag)
    {
        slice_header->delta_pic_order_cnt[0] = bs_read_se(b, "SH: delta_pic_order_cnt[0]");
        if (active_pps->bottom_field_pic_order_in_frame_present_flag &&
            !slice_header->field_pic_flag) {
            slice_header->delta_pic_order_cnt[1] = bs_read_se(b, "SH: delta_pic_order_cnt[1]");
        }else {
            slice_header->delta_pic_order_cnt[1] = 0;
        }
    }else if (active_sps->pic_order_cnt_type == 1) {
        slice_header->delta_pic_order_cnt[0] = 0;
        slice_header->delta_pic_order_cnt[1] = 0;
    }
    
    if (active_pps->redundant_pic_cnt_present_flag) {
        slice_header->redundant_pic_cnt = bs_read_ue(b, "SH: redundant_pic_cnt");
    }
    
    if (slice_header->slice_type == Slice_Type_B) {
        slice_header->direct_spatial_mv_pred_flag = bs_read_u(b, 1, "SH: direct_spatial_mv_pred_flag");
    }
    
    if (slice_header->slice_type == Slice_Type_P ||
             slice_header->slice_type == Slice_Type_SP ||
             slice_header->slice_type == Slice_Type_B)
    {
        slice_header->num_ref_idx_active_override_flag = bs_read_u(b, 1, "SH: num_ref_idx_active_override_flag");
        if (slice_header->num_ref_idx_active_override_flag)
        {
            slice_header->num_ref_idx_l0_active_minus1 = bs_read_ue(b, "SH: num_ref_idx_l0_active_minus1");
            if (slice_header->slice_type == Slice_Type_B)
            {
                slice_header->num_ref_idx_l1_active_minus1 = bs_read_ue(b, "SH: num_ref_idx_l1_active_minus1");
            }
        }
    }
    
    // 1.�����ο�ͼ���б������䷨Ԫ��
    parse_ref_pic_list_modification(b);
    
    if ((active_pps->weighted_pred_flag && (slice_header->slice_type == Slice_Type_P || slice_header->slice_type == Slice_Type_SP)) ||
        (active_pps->weighted_bipred_idc == 1 && slice_header->slice_type == Slice_Type_B)) {
        // 2.����Ԥ���Ȩ���䷨Ԫ��
        parse_pred_weight_table(b);
    }
    
    if (currentSlice->nal_ref_idc != 0) {
        // 3.��������ο�ͼ���ʶ�䷨Ԫ��
        parse_dec_ref_pic_marking(b);
    }
    
    if (active_pps->entropy_coding_mode_flag &&
        slice_header->slice_type != Slice_Type_I &&
        slice_header->slice_type != Slice_Type_SI) {
        slice_header->cabac_init_idc = bs_read_ue(b, "SH: cabac_init_idc");
    }else {
        slice_header->cabac_init_idc = 0;
    }
    
    slice_header->slice_qp_delta = bs_read_se(b, "SH: slice_qp_delta");
    if (slice_header->slice_type == Slice_Type_SP ||
        slice_header->slice_type == Slice_Type_SI) {
        if (slice_header->slice_type == Slice_Type_SP) {
            slice_header->sp_for_switch_flag = bs_read_u(b, 1, "SH: sp_for_switch_flag");
        }
        slice_header->slice_qs_delta = bs_read_se(b, "SH: slice_qs_delta");
    }
    
    if (active_pps->deblocking_filter_control_present_flag) {
        slice_header->disable_deblocking_filter_idc = bs_read_ue(b, "SH: disable_deblocking_filter_idc");
        if (slice_header->disable_deblocking_filter_idc != 1) {
            slice_header->slice_alpha_c0_offset_div2 = bs_read_se(b, "SH: slice_alpha_c0_offset_div2");
            slice_header->slice_beta_offset_div2 = bs_read_se(b, "SH: slice_beta_offset_div2");
        }else {
            // ����Ĭ��ֵ
            slice_header->slice_alpha_c0_offset_div2 = 0;
            slice_header->slice_beta_offset_div2 = 0;
        }
    }else {
        // ����Ĭ��ֵ
        slice_header->disable_deblocking_filter_idc = 0;
        slice_header->slice_alpha_c0_offset_div2 = 0;
        slice_header->slice_beta_offset_div2 = 0;
    }
    
    if (active_pps->num_slice_groups_minus1 > 0 &&
        active_pps->slice_group_map_type >= 3 &&
        active_pps->slice_group_map_type <= 5) {
        // ��7.4.3 slice_header����
        // ����ֱ����active_pps->pic_size_in_map_units_minus1����Ϊ������ûֵ
        int bit_len = ((active_sps->pic_width_in_mbs_minus1 + 1) * (active_sps->pic_height_in_map_units_minus1 + 1)) / (active_pps->slice_group_change_rate_minus1 + 1);
        // ����Ceil(bit_len)
        if (((active_sps->pic_width_in_mbs_minus1 + 1) * (active_sps->pic_height_in_map_units_minus1 + 1)) % (active_pps->slice_group_change_rate_minus1 + 1)) {
            bit_len++;
        }
        
        // ȥ����Ceil( Log2( PicSizeInMapUnits �� SliceGroupChangeRate + 1 ) )
        bit_len = calculateCeilLog2(bit_len + 1);
        
        slice_header->slice_group_change_cycle = bs_read_u(b, bit_len, "SH: slice_group_change_cycle");
    }
}

/**
 ����ref_pic_list_modification()�䷨Ԫ��
 [h264Э���ĵ�λ��]��7.3.3.1 Reference picture list modification syntax
 */
void parse_ref_pic_list_modification(bs_t *b)
{
    rplm_t *rplm = &currentSlice->slice_header.ref_pic_list_modification;
    int i, val;
    
    // 0.��ʼ���ڴ�ռ�
    alloc_ref_pic_list_modification_buffer();
    
    if (currentSlice->slice_header.slice_type != Slice_Type_I &&
        currentSlice->slice_header.slice_type != Slice_Type_SI) {
        rplm->ref_pic_list_modification_flag_l0 = bs_read_u(b, 1, "SH: ref_pic_list_modification_flag_l0");
        if (rplm->ref_pic_list_modification_flag_l0) {
            i = 0;
            do {
                val = rplm->modification_of_pic_nums_idc_lo[i] = bs_read_ue(b, "SH: modification_of_pic_nums_idc_lo");
                if (val == 0 || val == 1) {
                    rplm->abs_diff_pic_num_minus1_lo[i] = bs_read_ue(b, "SH: abs_diff_pic_num_minus1_lo");
                }else if (val == 2) {
                    rplm->long_term_pic_num_lo[i] = bs_read_ue(b, "SH: long_term_pic_num_lo");
                }
                i++;
            } while (val != 3);
        }
    }
    
    if (currentSlice->slice_header.slice_type == Slice_Type_B) {
        rplm->ref_pic_list_modification_flag_l1 = bs_read_u(b, 1, "SH: ref_pic_list_modification_flag_l1");
        if (rplm->ref_pic_list_modification_flag_l1) {
            i = 0;
            do {
                val = rplm->modification_of_pic_nums_idc_l1[i] = bs_read_ue(b, "SH: modification_of_pic_nums_idc_l1");
                if (val == 0 || val == 1) {
                    rplm->abs_diff_pic_num_minus1_l1[i] = bs_read_ue(b, "SH: abs_diff_pic_num_minus1_l1");
                }else if (val == 2) {
                    rplm->long_term_pic_num_l1[i] = bs_read_ue(b, "SH: long_term_pic_num_l1");
                }
                i++;
            } while (val != 3);
        }
    }
}

/**
 Ϊref_pic_list_modification()��ʼ���ڴ�ռ�
 [h264Э���ĵ�λ��]��7.3.3.1 Reference picture list modification syntax
 */
void alloc_ref_pic_list_modification_buffer()
{
    rplm_t *rplm = &currentSlice->slice_header.ref_pic_list_modification;
    
    // �ȶԲο�ͼ���б�0��������������������ͼ��������Ϊref_idx_l0�ĳ��ȣ�Ҳ��num_ref_idx_l0_active_minus1+1
    int size = currentSlice->slice_header.num_ref_idx_l0_active_minus1 + 1;
    
    if (currentSlice->slice_header.slice_type != Slice_Type_I &&
        currentSlice->slice_header.slice_type != Slice_Type_SI)
    {
        rplm->modification_of_pic_nums_idc_lo = (int*)calloc(size, sizeof(int));
        rplm->abs_diff_pic_num_minus1_lo = (int*)calloc(size, sizeof(int));
        rplm->long_term_pic_num_lo = (int*)calloc(size, sizeof(int));
        
        if (rplm->modification_of_pic_nums_idc_lo == NULL ||
            rplm->abs_diff_pic_num_minus1_lo == NULL ||
            rplm->long_term_pic_num_lo == NULL) {
            fprintf(stderr, "%s\n", "Alloc ref_pic_list_modification_lo Error");
            exit(-1);
        }
    }else {
        rplm->modification_of_pic_nums_idc_lo = NULL;
        rplm->abs_diff_pic_num_minus1_lo = NULL;
        rplm->long_term_pic_num_lo = NULL;
    }
    
    size = currentSlice->slice_header.num_ref_idx_l1_active_minus1 + 1;
    
    if (currentSlice->slice_header.slice_type == Slice_Type_B)
    {
        rplm->modification_of_pic_nums_idc_l1 = (int*)calloc(size, sizeof(int));
        rplm->abs_diff_pic_num_minus1_l1 = (int*)calloc(size, sizeof(int));
        rplm->long_term_pic_num_l1 = (int*)calloc(size, sizeof(int));
        
        if (rplm->modification_of_pic_nums_idc_l1 == NULL ||
            rplm->abs_diff_pic_num_minus1_l1 == NULL ||
            rplm->long_term_pic_num_l1 == NULL) {
            fprintf(stderr, "%s\n", "Alloc ref_pic_list_modification_l1 Error");
            exit(-1);
        }
    }else {
        rplm->modification_of_pic_nums_idc_l1 = NULL;
        rplm->abs_diff_pic_num_minus1_l1 = NULL;
        rplm->long_term_pic_num_l1 = NULL;
    }
}

/**
 ����pred_weight_table()�䷨Ԫ��
 [h264Э���ĵ�λ��]��7.3.3.2 Prediction weight table syntax
 */
void parse_pred_weight_table(bs_t *b)
{
    pred_weight_table_t *pw_table = &currentSlice->slice_header.pred_weight_table;
    
    pw_table->luma_log2_weight_denom = bs_read_ue(b, "SH: luma_log2_weight_denom");
    if (active_sps->chroma_format_idc != 0) {
        pw_table->chroma_log2_weight_denom = bs_read_ue(b, "SH: chroma_log2_weight_denom");
    }
    
    for (int i = 0; i <= currentSlice->slice_header.num_ref_idx_l0_active_minus1; i++) {
        pw_table->luma_weight_l0_flag = bs_read_u(b, 1, "SH: luma_weight_l0_flag");
        
        // �ο�7.4.3.2���壬��pw_table->luma_weight_l0_flag����0ʱ�������޸�
        if (pw_table->luma_weight_l0_flag) {
            pw_table->luma_weight_l0[i] = bs_read_se(b, "SH: luma_weight_l0");
            pw_table->luma_offset_l0[i] = bs_read_se(b, "SH: luma_offset_l0");
        }else {
            pw_table->luma_weight_l0[i] = 1 << pw_table->luma_log2_weight_denom;
            pw_table->luma_offset_l0[i] = 0;
        }
        
        if (active_sps->chroma_format_idc != 0) {
            pw_table->chroma_weight_l0_flag = bs_read_u(b, 1, "SH: chroma_weight_l0_flag");
            // �ο�7.4.3.2���壬��pw_table->chroma_weight_l0_flag����0ʱ�������޸�
            for (int j = 0; j < 2; j++) {
                if (pw_table->chroma_weight_l0_flag) {
                    pw_table->chroma_weight_l0[i][j] = bs_read_se(b, "SH: chroma_weight_l0");
                    pw_table->chroma_offset_l0[i][j] = bs_read_se(b, "SH: chroma_offset_l0");
                }else {
                    pw_table->chroma_weight_l0[i][j] = 1 << pw_table->chroma_log2_weight_denom;
                    pw_table->chroma_offset_l0[i][j] = 0;
                }
            }
        }
    }
    
    if (currentSlice->slice_header.slice_type == Slice_Type_B) {
        for (int i = 0; i <= currentSlice->slice_header.num_ref_idx_l1_active_minus1; i++) {
            pw_table->luma_weight_l1_flag = bs_read_u(b, 1, "SH: luma_weight_l1_flag");
            
            // �ο�7.4.3.2���壬��pw_table->luma_weight_l1_flag����0ʱ�������޸�
            if (pw_table->luma_weight_l1_flag) {
                pw_table->luma_weight_l1[i] = bs_read_se(b, "SH: luma_weight_l1");
                pw_table->luma_offset_l1[i] = bs_read_se(b, "SH: luma_offset_l1");
            }else {
                pw_table->luma_weight_l1[i] = 1 << pw_table->luma_log2_weight_denom;
                pw_table->luma_offset_l1[i] = 0;
            }
            
            if (active_sps->chroma_format_idc != 0) {
                pw_table->chroma_weight_l1_flag = bs_read_u(b, 1, "SH: chroma_weight_l1_flag");
                // �ο�7.4.3.2���壬��pw_table->chroma_weight_l1_flag����0ʱ�������޸�
                for (int j = 0; j < 2; j++) {
                    if (pw_table->chroma_weight_l1_flag) {
                        pw_table->chroma_weight_l1[i][j] = bs_read_se(b, "SH:   chroma_weight_l1");
                        pw_table->chroma_offset_l1[i][j] = bs_read_se(b, "SH: chroma_offset_l1");
                    }else {
                        pw_table->chroma_weight_l1[i][j] = 1 << pw_table->chroma_log2_weight_denom;
                        pw_table->chroma_offset_l1[i][j] = 0;
                    }
                }
            }
        }
    }
}

/**
 ����dec_ref_pic_marking()�䷨Ԫ��
 [h264Э���ĵ�λ��]��7.3.3.3 Decoded reference picture marking syntax
 */
void parse_dec_ref_pic_marking(bs_t *b)
{
    dec_ref_pic_marking_t *drp_marking = &currentSlice->slice_header.dec_ref_pic_marking;
    int i, val;

    if (currentSlice->idr_flag) {
        drp_marking->no_output_of_prior_pics_flag = bs_read_u(b, 1, "SH: no_output_of_prior_pics_flag");
        drp_marking->long_term_reference_flag = bs_read_u(b, 1, "SH: long_term_reference_flag");
    }else {
        drp_marking->adaptive_ref_pic_marking_mode_flag = bs_read_u(b, 1, "SH: adaptive_ref_pic_marking_mode_flag");
        if (drp_marking->adaptive_ref_pic_marking_mode_flag) {
            i = 0;
            do {
                val = drp_marking->memory_management_control_operation[i] = bs_read_ue(b, "SH: memory_management_control_operation");
                if (val == 1 || val == 3) {
                    drp_marking->difference_of_pic_nums_minus1[i] = bs_read_ue(b, "SH: difference_of_pic_nums_minus1");
                }
                if (val == 2) {
                    drp_marking->long_term_pic_num[i] = bs_read_ue(b, "SH: long_term_pic_num");
                }
                if (val == 3 || val == 6) {
                    drp_marking->long_term_frame_idx[i] = bs_read_ue(b, "SH: long_term_frame_idx");
                }
                if (val == 4) {
                    drp_marking->max_long_term_frame_idx_plus1[i] = bs_read_ue(b, "SH: max_long_term_frame_idx_plus1");
                }
                i++;
            } while (val != 0);
        }
    }
}

#pragma mark - ���߷���
/**
 ����CeilLog2(inputVal)
 */
unsigned calculateCeilLog2(unsigned inputVal)
{
    unsigned tmpVal = inputVal - 1;
    unsigned ret = 0;
    
    while( tmpVal != 0 ) {
        tmpVal >>= 1;
        ret++;
    }
    return ret;
}