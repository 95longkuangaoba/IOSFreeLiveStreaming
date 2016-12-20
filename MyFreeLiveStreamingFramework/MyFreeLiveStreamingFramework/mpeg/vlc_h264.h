//
// Created by kly on 16-10-27.
//

#ifndef FREELIVESTREAMING_VLC_H264_H
#define FREELIVESTREAMING_VLC_H264_H


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


/* Convert H.264 NAL format to annex b in-place */
struct H264ConvertState
{
    uint32_t nal_len;
    uint32_t nal_pos;
};


/* Parse the SPS/PPS Metadata and convert it to annex b format */
int convert_sps_pps(const uint8_t *p_buf, uint32_t i_buf_size, uint8_t *p_out_buf, uint32_t i_out_buf_size, uint32_t *p_sps_pps_size, uint32_t *p_nal_size)
{
    int i, j;
    int i_profile;
    uint32_t i_data_size = i_buf_size, i_nal_size, i_sps_pps_size = 0;
    unsigned int i_loop_end;

    /* */
    if (i_data_size < 7)
    {
        return -1;
    }

    /* Read infos in first 6 bytes */
    i_profile = (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
    if (p_nal_size)
        *p_nal_size = (p_buf[4] & 0x03) + 1;
    p_buf += 5;
    i_data_size -= 5;

    for (j = 0; j < 2; j++)
    {
        /* First time is SPS, Second is PPS */
        if (i_data_size < 1)
        {
            return -1;
        }
        i_loop_end = p_buf[0] & (j == 0 ? 0x1f : 0xff);
        p_buf++;
        i_data_size--;

        for (i = 0; i < i_loop_end; i++)
        {
            if (i_data_size < 2)
            {
                return -1;
            }

            i_nal_size = (p_buf[0] << 8) | p_buf[1];
            p_buf += 2;
            i_data_size -= 2;

            if (i_data_size < i_nal_size)
            {
                return -1;
            }
            if (i_sps_pps_size + 4 + i_nal_size > i_out_buf_size)
            {
                return -1;
            }

            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 1;

            memcpy(p_out_buf + i_sps_pps_size, p_buf, i_nal_size);
            i_sps_pps_size += i_nal_size;

            p_buf += i_nal_size;
            i_data_size -= i_nal_size;
        }
    }

    *p_sps_pps_size = i_sps_pps_size;

    return 0;
}

void convert_h264_to_annexb(uint8_t *p_buf, uint32_t i_len, size_t i_nal_size, struct H264ConvertState *state)
{
    if (i_nal_size < 3 || i_nal_size > 4)
        return;

    /* This only works for NAL sizes 3-4 */
    while (i_len > 0)
    {
        if (state->nal_pos < i_nal_size)
        {
            unsigned int i;
            for (i = 0; state->nal_pos < i_nal_size && i < i_len; i++, state->nal_pos++)
            {
                state->nal_len = (state->nal_len << 8) | p_buf[i];
                p_buf[i] = 0;
            }
            if (state->nal_pos < i_nal_size)
                return;
            p_buf[i - 1] = 1;
            p_buf += i;
            i_len -= i;
        }
        if (state->nal_len > INT_MAX)
            return;
        if (state->nal_len > i_len)
        {
            state->nal_len -= i_len;
            return;
        }
        else
        {
            p_buf += state->nal_len;
            i_len -= state->nal_len;
            state->nal_len = 0;
            state->nal_pos = 0;
        }
    }
}


#endif //FREELIVESTREAMING_VLC_H264_H
