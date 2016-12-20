//
// Created by kly on 16-10-21.
//

#include "rtmp_io.h"

#include "rtmp.h"
#include "self-reliance-media.h"
#include "self-reliance-media-io.h"
#include "h264_parser.h"
#include "mpeg4audio.h"
#include "vlc_h264.h"


#include <sys/select.h>



typedef struct rtmp_io_t
{
    bool running;
    bool connection;

    int nal_size;

    RTMP *rtmp;
    RTMPPacket packet;

    sr___audio_format___t audio_format;
    sr___video_format___t video_format;

}rtmp_io_t;




//////////////////////////////////////////////////////////////////////////////////



static int parser_sps_pps(char *data, int size, char **sps, int *sps_size, char **pps, int *pps_size){

    int i = 0;
    char *head = NULL;

    if (data == NULL){
        log_error(ERRPARAM);
        return -1;
    }

    if ( data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 1 || ( (data[4] & 0x1f) != 7 ) ){
        log_error(ERRPARAM);
        return -1;
    }

    head = data;
    data += 4;
    *sps = data;


    i = 0;
    while(i < size - 4){
        if ( data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 0 && data[i + 3] == 1){
            break;
        }
        i ++;
    }

    *sps_size = i;
    data += i;

    if ( data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 1 || ((data[4] & 0x1f) != 8)){
        log_error(ERRPARAM);
        return -1;
    }

    head = data;
    *pps = data + 4;
    *pps_size = size - (i + 4 + 4);

    return 0;
}

static int make_tag(rtmp_io_t *rtmp_io, sr___media_frame___t *frame){

    int size = 0;
    char *buf = NULL;

    char codec_flag = 0;
    char frame_flag = 0;
    char h264_config_flag = 0;

    char sound_type_flag = 0;
    char sound_format_flag = 0;
    char sound_rate_flag = 0;
    char sound_size_flag = 0;
    char aac_config_flag = 0;

    int sps_size = 0;
    int pps_size = 0;
    char *sps = NULL;
    char *pps = NULL;

    size = 0;
    buf = rtmp_io->packet.m_body;

    if (frame->slice.type.media == SR___Media_Type___Video){

        codec_flag = 7;

        if (frame->slice.type.payload == SR___Payload_Type___Frame){

            if (frame->slice.type.extend == SR___Video_Extend_Type___KeyFrame){
                frame_flag = 0x01;
                h264_config_flag = 0x01;
            }else{
                frame_flag = 0x02;
                h264_config_flag = 0x01;
            }


        }else if (frame->slice.type.payload == SR___Payload_Type___Format){
            sr___video_format___t *video_format = sr___media_frame___to___video_format(frame);
            rtmp_io->video_format = *video_format;
            frame_flag = 0x01;
            h264_config_flag = 0x00;
            parser_sps_pps(rtmp_io->video_format.extend_data, rtmp_io->video_format.extend_data_size,
                           &sps, &sps_size, &pps, &pps_size);
            srfree(video_format);
        }

        buf[0] = (char)(frame_flag << 4 | codec_flag);
        buf[1] = (char)h264_config_flag;
        buf[2] = 0;
        buf[3] = 0;
        buf[4] = 0;
        size = 5;

        if (h264_config_flag == 0){

            buf[5] = 0x01;
            buf[6] = sps[1];
            buf[7] = sps[2];
            buf[8] = sps[3];
            //0x3F 111111 保留位 全部填充 1
            //0x03 Nal size
            buf[9] = (char)(0x3F << 2 | 0x03);
            size += 5;

            //0x07 111 保留位 全部填充 1
            //0x01 Number of sps
            buf[10] = (char)((0x07 << 5) | 0x01);
            buf[11] = (char)((sps_size >> 8) & 0xFF);
            buf[12] = (char)(sps_size & 0xFF);
            size += 3;

            memcpy(buf + size, sps, (size_t)sps_size);
            size += sps_size;

            //Number of pps
            buf[size] = 0x01;
            size ++;
            buf[size] = (char)((pps_size >> 8) & 0xFF);
            size ++;
            buf[size] = (char)(pps_size & 0xFF);
            size ++;
            memcpy(buf + size, pps, (size_t)pps_size);
            size += pps_size;

        }else{

            int x = frame->size - 4;
            frame->data[0] = (uint8_t)((x >> 24) & 0xFF);
            frame->data[1] = (uint8_t)((x >> 16) & 0xFF);
            frame->data[2] = (uint8_t)((x >> 8) & 0xFF);
            frame->data[3] = (uint8_t)((x) & 0xFF);
            memcpy(buf + size, frame->data, frame->size);
            size += frame->size;
        }

    }else if (frame->slice.type.media == SR___Media_Type___Audio){

        sound_format_flag = 0x0A;
        sound_rate_flag = 0x03;
        sound_size_flag = 0x01;
        sound_type_flag = 0x00;

        if (frame->slice.type.payload == SR___Payload_Type___Frame){

            aac_config_flag = 0x01;

        }else if (frame->slice.type.payload == SR___Payload_Type___Format){

            aac_config_flag = 0x00;

            sr___audio_format___t *audio_format = sr___media_frame___to___audio_format(frame);
            rtmp_io->audio_format = *audio_format;
            srfree(audio_format);
        }

        buf[size ++] = sound_format_flag << 4 | sound_rate_flag << 2 | sound_size_flag << 1 | sound_type_flag;
        buf[size ++] = aac_config_flag;
        if (aac_config_flag == 0x00){
            memcpy(buf + size, rtmp_io->audio_format.extend_data, rtmp_io->audio_format.extend_data_size);
            size += rtmp_io->audio_format.extend_data_size;
        }else{
            memcpy(buf + size, frame->data, frame->size);
            size += frame->size;
        }
    }

    return size;
}


static int make_frame(rtmp_io_t *rtmp_io, uint32_t type, uint32_t timestamp,
                      uint32_t size, uint8_t *data, sr___media_frame___t **pp_frame)
{
    int result = 0;
    sr___media_frame___t *frame = NULL;

    if (type == 0x08){

        if (size < 4){
            result = ERRPARAM;
            log_error(result);
            goto ERRPARAM_EXIT;
        }

        uint8_t sound_format_flag = (uint8_t)data[0] >> 4;
        uint8_t aac_config_flag = data[1];

        size -= 2;
        data += 2;

//        log_debug("sound fmt %x\n", sound_format_flag);
        //只支持AAC
        if ((uint8_t)sound_format_flag != 10){
            result = ERRPARAM;
            log_error(result);
            goto ERRPARAM_EXIT;
        }

        if (aac_config_flag == 0){
            MPEG4AudioConfig mpeg4AudioConfig = {0};
            result = avpriv_mpeg4audio_get_config(&mpeg4AudioConfig, data, size, 0);
            if (result != 0
                || mpeg4AudioConfig.channels <= 0
                || mpeg4AudioConfig.channels > 2
                || mpeg4AudioConfig.sample_rate <= 0){
                result = ERRPARAM;
                log_error(result);
                goto ERRPARAM_EXIT;
            }
            rtmp_io->audio_format.channels = mpeg4AudioConfig.channels;
            rtmp_io->audio_format.sample_rate = mpeg4AudioConfig.sample_rate;
            rtmp_io->audio_format.extend_data_size = size;
            memcpy(rtmp_io->audio_format.extend_data, data, size);
            frame = sr___audio_format___to___media_frame(&(rtmp_io->audio_format));
            if (frame == NULL){
                result = ERRMEMORY;
                log_error(result);
                goto ERRPARAM_EXIT;
            }

        }else{

            if (size <= 0){
                result = ERRPARAM;
                log_error(result);
                goto ERRPARAM_EXIT;
            }

            result = sr___media_frame___create(&frame);
            if (frame == NULL){
                log_error(result);
                goto ERRPARAM_EXIT;
            }
            frame->data = (uint8_t*)srmalloc(size);
            if (frame->data == NULL){
                result = ERRMEMORY;
                log_error(result);
                goto ERRPARAM_EXIT;
            }
            frame->size = size;
            memcpy(frame->data, data, size);

            frame->slice.type.media = SR___Media_Type___Audio;
            frame->slice.type.payload = SR___Payload_Type___Frame;
            frame->slice.timestamp = timestamp;
        }

    }else if (type == 0x09){

        if (size < 5){
            result = ERRPARAM;
            log_error(result);
            goto ERRPARAM_EXIT;
        }

        char codec_flag = data[0] & 0x0F;
        char frame_flag = data[0] >> 4;
        char h264_config_flag = data[1];

        size -= 5;
        data += 5;

        //只支持H264
        if (codec_flag != 7){
            result = ERRPARAM;
            log_error(result);
            goto ERRPARAM_EXIT;
        }

        if (h264_config_flag == 0){
            if (convert_sps_pps(data, size, rtmp_io->video_format.extend_data,
                                size, &(rtmp_io->video_format.extend_data_size),
                                &(rtmp_io->nal_size)) != 0){
                result = ERRPARAM;
                log_error(result);
                goto ERRPARAM_EXIT;
            }
            h264_config_parser(rtmp_io->video_format.extend_data,
                               rtmp_io->video_format.extend_data_size,
                               &(rtmp_io->video_format.width),
                               &(rtmp_io->video_format.height),
                               &(rtmp_io->video_format.frame_per_second));
            if (rtmp_io->video_format.width <= 0 || rtmp_io->video_format.height <= 0){
                result = ERRPARAM;
                log_error(result);
                goto ERRPARAM_EXIT;
            }

            frame = sr___video_format___to___media_frame(&(rtmp_io->video_format));
            if (frame == NULL){
                result = ERRMEMORY;
                log_error(result);
                goto ERRPARAM_EXIT;
            }

        }else{

            if (size <= 0){
                result = ERRPARAM;
                log_error(result);
                goto ERRPARAM_EXIT;
            }

            struct H264ConvertState state = {0};
            convert_h264_to_annexb(data, size, rtmp_io->nal_size, &state);
            result = sr___media_frame___create(&frame);
            if (result != 0){
                log_error(result);
                goto ERRPARAM_EXIT;
            }
            frame->data = (uint8_t*)srmalloc(size);
            if (frame->data == NULL){
                result = ERRMEMORY;
                log_error(result);
                goto ERRPARAM_EXIT;
            }
            frame->size = size;
            memcpy(frame->data, data, size);

            frame->slice.type.media = SR___Media_Type___Video;
            frame->slice.type.payload = SR___Payload_Type___Frame;
            if (frame_flag == 1){
                frame->slice.type.extend = SR___Video_Extend_Type___KeyFrame;
            }else{
                frame->slice.type.extend = SR___Video_Extend_Type___None;
            }
            frame->slice.timestamp = timestamp;
        }

    }else{

        result = ERRPARAM;
        log_error(result);
        goto ERRPARAM_EXIT;
    }

    *pp_frame = frame;


    return 0;


    ERRPARAM_EXIT:


    if (frame != NULL){
        sr___media_frame___release(&frame);
    }


    return result;
}


static int rtmp_wirte_frame(sr___media_io___t *io, sr___media_frame___t *frame)
{
    int result = 0;
    int tag_size = 0;

    fd_set wfds;
    struct timeval tv;
    int64_t timeout = 0;

    if (frame == NULL || io == NULL){
        log_error(ERRPARAM);
        return ERRPARAM;
    }

    rtmp_io_t *rtmp_io = (rtmp_io_t *)(io->protocol);


    while(ISTRUE(rtmp_io->running)){

        FD_ZERO(&wfds);
        FD_SET(rtmp_io->rtmp->m_sb.sb_socket, &wfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        if ((result = select(rtmp_io->rtmp->m_sb.sb_socket + 1, NULL, &wfds, NULL, &tv)) == 0){
            if ((timeout += 100000) > 1000000000){
                log_error(ERRTIMEOUT);
                return ERRTIMEOUT;
            }
            continue;
        }

        if (result < 0){
            log_error(ERRSYSCALL);
            return ERRSYSCALL;
        }

        break;
    }

    if (SETTRUE(rtmp_io->connection)){
        if (!RTMP_ConnectStream(rtmp_io->rtmp, 0)){
            log_error(ERRTIMEOUT);
            return ERRTIMEOUT;
        }
    }

    if (frame->slice.type.media == SR___Media_Type___Audio){
        tag_size = make_tag(rtmp_io, frame);
        rtmp_io->packet.m_packetType = 0x08;
    }else if (frame->slice.type.media == SR___Media_Type___Video){
        tag_size = make_tag(rtmp_io, frame);
        rtmp_io->packet.m_packetType = 0x09;
    }

    rtmp_io->packet.m_nChannel = 0x04;
    rtmp_io->packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    rtmp_io->packet.m_nTimeStamp = (uint32_t)frame->slice.timestamp;
    rtmp_io->packet.m_nInfoField2 = rtmp_io->rtmp->m_stream_id;
    rtmp_io->packet.m_nBodySize = (uint32_t)tag_size;

    if (!RTMP_SendPacket(rtmp_io->rtmp, &rtmp_io->packet, 0)){
        log_error(ERRTIMEOUT);
        return ERRTIMEOUT;
    }

    return 0;
}


static int rtmp_read_frame(sr___media_io___t *io, sr___media_frame___t **pp_frame)
{
    int result = 0;

    fd_set rfds;
    struct timeval tv;
    int64_t timeout = 0;

    RTMPPacket packet = { 0 };
    sr___media_frame___t *frame = NULL;

    if (pp_frame == NULL || io == NULL){
        log_error(ERRPARAM);
        return ERRPARAM;
    }

    rtmp_io_t *rtmp_io = (rtmp_io_t *)(io->protocol);


    while (ISTRUE(rtmp_io->running)){

        FD_ZERO(&rfds);
        FD_SET(rtmp_io->rtmp->m_sb.sb_socket, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        if ((result = select(rtmp_io->rtmp->m_sb.sb_socket + 1, &rfds, NULL, NULL, &tv)) == 0){
            if ((timeout += 100000) > 10000000){
                result = ERRTIMEOUT;
                log_error(result);
                return result;
            }
            continue;
        }

        timeout = 0;

        if (result < 0){
            result = ERRSYSCALL;
            log_error(result);
            return result;
        }

        if (SETTRUE(rtmp_io->connection)){
            if (!RTMP_ConnectStream(rtmp_io->rtmp, 0)){
                result = ERRTIMEOUT;
                log_error(result);
                return result;
            }
        }

        if (!RTMP_ReadPacket(rtmp_io->rtmp, &packet) || !RTMPPacket_IsReady(&packet)){
            if (rtmp_io->rtmp->m_sb.sb_socket < 0){
                result = ERRTIMEOUT;
                log_error(result);
                return result;
            }
            RTMPPacket_Reset(&packet);
            continue;
        }


        if (packet.m_packetType == 0x09){

            if (ISTRUE(rtmp_io->running)){
                result = make_frame(rtmp_io, packet.m_packetType, packet.m_nTimeStamp,
                                    packet.m_nBodySize, packet.m_body, &frame);
                if (result != 0){
                    RTMPPacket_Free(&packet);
                    if (result == ERRPARAM){
                        log_error(result);
                        continue;
                    }
                    log_error(result);
                    return result;
                }
            }else{
                result = ERRTERMINATION;
            }
            break;

        }else if (packet.m_packetType == 0x08){

            if (ISTRUE(rtmp_io->running)){
//                log_debug("%x %x %x %x\n", packet.m_body[0],packet.m_body[1],packet.m_body[2],packet.m_body[3]);
                result = make_frame(rtmp_io, packet.m_packetType, packet.m_nTimeStamp,
                                    packet.m_nBodySize, packet.m_body, &frame);
                if (result != 0){
                    RTMPPacket_Free(&packet);
                    if (result == ERRPARAM){
                        log_error(result);
                        continue;
                    }
                    log_error(result);
                    return result;
                }
            } else {
                result = ERRTERMINATION;
            }

            break;

        }else if (packet.m_packetType == 0x12){

            log_debug("rtmp packet type %d\n", packet.m_packetType);
            RTMPPacket_Free(&packet);
            continue;

        }else{

            log_debug("rtmp packet type %d\n", packet.m_packetType);
            RTMPPacket_Free(&packet);
            continue;
        }
    }


    RTMPPacket_Free(&packet);
    RTMPPacket_Reset(&packet);

    *pp_frame = frame;


    return result;
}


static void rtmp_close(sr___media_io___t *io)
{
    if (io && io->protocol){
        log_debug("rtmp_close enter\n");
        rtmp_io_t *rtmp_io = (rtmp_io_t *)(io->protocol);
        io->protocol = NULL;
        RTMPPacket_Free(&(rtmp_io->packet));
        RTMP_Close(rtmp_io->rtmp);
        RTMP_Free(rtmp_io->rtmp);
        srfree(rtmp_io);
        log_debug("rtmp_close exit\n");
    }
}


static void rtmp_stop(sr___media_io___t *io)
{
    log_debug("rtmp_stop enter\n");
    if (io && io->protocol){
        rtmp_io_t *rtmp_io = (rtmp_io_t *)(io->protocol);
        SETFALSE(rtmp_io->running);
    }
    log_debug("rtmp_stop exit\n");
}


int rtmp_open(sr___media_io___t *io, const char *url, int mode)
{
    log_debug("rtmp_open enter\n");

    int result = 0;
    rtmp_io_t *rtmp_io = NULL;

    rtmp_io = (rtmp_io_t *)srmalloc(sizeof(rtmp_io_t));
    if (rtmp_io == NULL){
        log_error(ERRMEMORY);
        return ERRMEMORY;
    }
    io->protocol = rtmp_io;

    SETTRUE(rtmp_io->running);

    rtmp_io->rtmp = RTMP_Alloc();
    if (rtmp_io->rtmp == NULL){
        result = ERRUNKONWN;
        log_error(result);
        goto EXIT_LOOP;
    }

    RTMP_Init(rtmp_io->rtmp);

    if (!RTMP_SetupURL(rtmp_io->rtmp, (char *)url)){
        result = ERRUNKONWN;
        log_error(result);
        goto EXIT_LOOP;
    }

    if (mode == MEDIA_IO_WRONLY){
        RTMP_EnableWrite(rtmp_io->rtmp);
    }

    int64_t start_time = sr___start_timing();
    while (!RTMP_Connect(rtmp_io->rtmp, NULL)){
        if (ISFALSE(rtmp_io->running)){
            goto EXIT_LOOP;
        }
        if (sr___complete_timing(start_time) > 10000000){
            result = ERRTIMEOUT;
            log_error(result);
            goto EXIT_LOOP;
        }
        nanosleep((const struct timespec[]){{0, 100000L}}, NULL);
    }

    SETFALSE(rtmp_io->connection);

    RTMPPacket_Reset(&(rtmp_io->packet));
    if (!RTMPPacket_Alloc(&(rtmp_io->packet), 1024 * 1024)){
        result = ERRUNKONWN;
        log_error(result);
        goto EXIT_LOOP;
    }

    io->write = rtmp_wirte_frame;
    io->read = rtmp_read_frame;
    io->stop = rtmp_stop;
    io->close = rtmp_close;

    log_debug("rtmp_open exit\n");

    return 0;


    EXIT_LOOP:


    rtmp_close(io);

    return result;
}
