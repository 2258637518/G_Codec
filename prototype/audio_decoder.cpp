#include "audio_decoder.h"
audio_decoder::audio_decoder(int id,SaveQueue<AVPacket*>&unpack_audio_pkt,SaveQueue<AVFrame*>&decode_audio_pkt,AVFormatContext *fmt)
    :unpack_audio_pkt(unpack_audio_pkt),decode_audio_pkt(decode_audio_pkt)
{
    audio_stream_id = id;
    codec_par = fmt->streams[id]->codecpar;
}

bool audio_decoder::init()
{

    return true;
}
