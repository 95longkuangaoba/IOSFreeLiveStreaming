//
// Created by kly on 16/7/28.
//

#ifndef HUANPENGLIBRARY_H264_PARSER_H
#define HUANPENGLIBRARY_H264_PARSER_H


#include "h264_stream.h"


static int h264_codec_parameter(h264_stream_t* h, unsigned char *data, int size)
{
    int nal_start, nal_end;

    size_t sz = size;
    unsigned char *p = data;
    int nal_size = 0;
    int h264_config_size = 0;

    while ((nal_size = find_nal_unit(p, sz, &nal_start, &nal_end)) > 0)
    {
        p += nal_start;
        read_nal_unit(h, p, nal_end - nal_start);

        switch (h->nal->nal_unit_type)
        {
            case NAL_UNIT_TYPE_UNSPECIFIED:
                break;
            case NAL_UNIT_TYPE_CODED_SLICE_NON_IDR:
                break;
            case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A:
                break;
            case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B:
            case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C:
                break;
            case NAL_UNIT_TYPE_CODED_SLICE_IDR:
                break;
            case NAL_UNIT_TYPE_SEI:
                break;
            case NAL_UNIT_TYPE_SPS:
                break;
            case NAL_UNIT_TYPE_PPS:
                break;
            case NAL_UNIT_TYPE_AUD:
                break;
            case NAL_UNIT_TYPE_END_OF_SEQUENCE:
                break;
            case NAL_UNIT_TYPE_END_OF_STREAM:
                break;
            case NAL_UNIT_TYPE_FILLER:
                break;
            case NAL_UNIT_TYPE_SPS_EXT:
                break;
                // 14..18    // Reserved
            case NAL_UNIT_TYPE_CODED_SLICE_AUX:
                break;
                // 20..23    // Reserved
                // 24..31    // Unspecified
            default:
                break;
        }

        h264_config_size += (nal_start + nal_size);
        p += (nal_end - nal_start);
        sz -= nal_end;
    }

    return h264_config_size;
}




int h264_config_parser(unsigned char *h264_config, int h264_config_size, int *width, int *height, int *frame_per_second)
{
    if (h264_config == NULL || h264_config_size <= 0|| width == NULL || height == NULL || frame_per_second == NULL)
    {
        return -1;
    }

    h264_stream_t* h = h264_new();

    int size = h264_codec_parameter(h, h264_config, h264_config_size);

    *width = ((h->sps->pic_width_in_mbs_minus1 + 1) * 16)
             - (h->sps->frame_crop_right_offset * 2)
             - (h->sps->frame_crop_left_offset * 2);
    *height = ((2 - h->sps->frame_mbs_only_flag)
               * (h->sps->pic_height_in_map_units_minus1 + 1) * 16)
              - h->sps->frame_crop_bottom_offset * 2
              - h->sps->frame_crop_top_offset * 2;


    if (h->sps->vui.timing_info_present_flag)
    {
        *frame_per_second = h->sps->vui.time_scale / (2 * h->sps->vui.num_units_in_tick);
    }
    else
    {
        *frame_per_second = 0;
    }

    h264_free(h);

    return 0;
}



#endif //HUANPENGLIBRARY_H264_PARSER_H
