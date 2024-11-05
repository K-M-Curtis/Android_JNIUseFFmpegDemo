#include <jni.h>
#include <string>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

// Android打印Log
#define LOG_TAG "SimplePlayer"
#define LOG(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,FORMAT,##__VA_ARGS__)

// print版本信息
extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_jniuseffmpegdemo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "SimplePlayer";
    char strBuffer[1024 * 4] = {0};
    strcat(strBuffer, "libavcodec : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVCODEC_VERSION));
    strcat(strBuffer, "\nlibavformat : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVFORMAT_VERSION));
    strcat(strBuffer, "\nlibavutil : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVUTIL_VERSION));
    strcat(strBuffer, "\nlibavfilter : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVFILTER_VERSION));
    strcat(strBuffer, "\nlibswresample : ");
    strcat(strBuffer, AV_STRINGIFY(LIBSWRESAMPLE_VERSION));
    strcat(strBuffer, "\nlibswscale : ");
    strcat(strBuffer, AV_STRINGIFY(LIBSWSCALE_VERSION));
    strcat(strBuffer, "\navcodec_configure : \n");
    strcat(strBuffer, avcodec_configuration());
    strcat(strBuffer, "\navcodec_license : ");
    strcat(strBuffer, avcodec_license());
    return env->NewStringUTF(hello.c_str());
}

// 播放视频流
extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniuseffmpegdemo_SimplePlayer_playVideo(JNIEnv *env, jobject thiz, jstring path,
                                                         jobject surface) {
    int ret;
    const char *video_path = env->GetStringUTFChars(path, 0);

    AVFormatContext *format_context = avformat_alloc_context();
    if (avformat_open_input(&format_context, video_path, NULL, NULL) != 0) {
        LOG("failed to open video file...");
        return;
    }
    if (avformat_find_stream_info(format_context, NULL) < 0) {
        LOG("failed to find stream information...");
        return;
    }

    int video_index = -1;
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
            video_index < 0) {
            video_index = i;
        }
    }
    if (video_index < 0 || video_index > format_context->nb_streams) {
        LOG("Can not find video stream...");
        return;
    }

    AVCodecContext *video_ctx = NULL;
    const AVCodec *video_codec = NULL;

    video_codec = avcodec_find_decoder(format_context->streams[video_index]->codecpar->codec_id);
    if (video_codec == NULL) {
        LOG("Cannot supported the video codec...");
        return;
    }
    video_ctx = avcodec_alloc_context3(video_codec);
    if (avcodec_parameters_to_context(video_ctx, format_context->streams[video_index]->codecpar) <
        0) {
        LOG("failed to copy the audio stream codecpar to codec context...");
        return;
    }

    if (avcodec_open2(video_ctx, video_codec, NULL) < 0) {
        LOG("failed to open decoder...");
        return;
    }

    int video_width = video_ctx->width;
    int video_height = video_ctx->height;
    ANativeWindow *native_window = ANativeWindow_fromSurface(env, surface);
    if (native_window == NULL) {
        LOG("failed to Create native window...");
        return;
    }

    ret = ANativeWindow_setBuffersGeometry(native_window, video_width, video_height,
                                           WINDOW_FORMAT_RGBA_8888);
    if (ret < 0) {
        LOG("Can not set native window buffer...");
        ANativeWindow_release(native_window);
        return;
    }

    ANativeWindow_Buffer window_buffer;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgba_frame = av_frame_alloc();

    int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, video_width, video_height, 1);
    uint8_t *out_buffer = (uint8_t *) av_malloc(buffer_size * sizeof(uint8_t));
    av_image_fill_arrays(rgba_frame->data, rgba_frame->linesize, out_buffer, AV_PIX_FMT_RGBA,
                         video_width, video_height, 1);
    struct SwsContext *sws_ctx = sws_getContext(video_width, video_height, video_ctx->pix_fmt,
                                                video_width, video_height, AV_PIX_FMT_RGBA,
                                                SWS_BICUBIC, NULL, NULL, NULL);

    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index == video_index) {
            ret = avcodec_send_packet(video_ctx, packet);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                LOG("Error sending the packet to the decoder...");
                return;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(video_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    LOG("Error during decoding...");
                    return;
                }
                fflush(stdout);

                ret = sws_scale(sws_ctx, (const uint8_t *const *) frame->data, frame->linesize, 0,
                                video_height, rgba_frame->data, rgba_frame->linesize);
                if (ret <= 0) {
                    LOG("data convert failed...");
                    return;
                }

                ret = ANativeWindow_lock(native_window, &window_buffer, NULL);
                if (ret < 0) {
                    LOG("Can not lock native window...");
                } else {
                    uint8_t *bits = (uint8_t *) window_buffer.bits;
                    for (int h = 0; h < video_height; h++) {
                        memcpy(bits + h * window_buffer.stride * 4,
                               out_buffer + h * rgba_frame->linesize[0], rgba_frame->linesize[0]);
                    }
                    ANativeWindow_unlockAndPost(native_window);
                }
            }
        }
        av_packet_unref(packet);
    }

    LOG("======== Play End ========");
    LOG("======== Release ========");
    sws_freeContext(sws_ctx);
    av_free(out_buffer);
    av_frame_free(&rgba_frame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    ANativeWindow_release(native_window);
    avcodec_close(video_ctx);
    avformat_close_input(&format_context);
}