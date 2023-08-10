#include <absl/strings/string_view.h>
#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
}

namespace {
#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wnarrowing"
#pragma clang diagnostic ignored "-Wconversion"
std::string error_string(int err_num) {
  constexpr int kMaxSize = 64;
  std::array<char, kMaxSize> buffer{ 0 };
  av_make_error_string(buffer.data(), kMaxSize, err_num);
  return std::string{ std::begin(buffer), std::end(buffer) };
}

void save_gray_frame(std::span<uint8_t> buf, int wrap, int xsize, int ysize, const absl::string_view filename) {
  auto file = std::unique_ptr<FILE, int (*)(FILE *)>(fopen(filename.data(), "w"), &fclose);
  // writing the minimal required header for a pgm file format
  // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
  // fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

  const int kByte = 255;
  fprintf(file.get(), "P5\n%d %d\n%d\n", xsize, ysize, kByte); // NOLINT: Can't do shit
  // writing line by line
  for (size_t i = 0; i < size_t{ ysize }; i++) {
    auto size = fwrite(&buf[i * wrap], 1, xsize, file.get());
    if (size == 0) { spdlog::info("Wrote {} bytes to {}", size, filename); }
  }
}

int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame) {
  // Supply raw packet data as input to a decoder
  // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
  int response = avcodec_send_packet(pCodecContext, pPacket);

  if (response < 0) {
    spdlog::info("Error while sending a packet to the decoder: {}", error_string(response));
    return response;
  }

  while (response >= 0) {
    // Return decoded output data (into a frame) from a decoder
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
    response = avcodec_receive_frame(pCodecContext, pFrame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      spdlog::info("Error while receiving a frame from the decoder: {}", error_string(response));
      return response;
    }

    if (response >= 0) {
      spdlog::info("Frame {} (type={}, size={} bytes, format={}) pts {} key_frame {} [DTS {}]",
        pCodecContext->frame_number,
        av_get_picture_type_char(pFrame->pict_type),
        pFrame->pkt_size,
        pFrame->format,
        pFrame->pts,
        pFrame->key_frame,
        pFrame->coded_picture_number);

      // char frame_filename[1024];
      const std::string frame_filename = fmt::format("frame-{}.pgm", pCodecContext->frame_number);
      // snprintf(frame_filename.data(), sizeof(frame_filename), "%s-{}.pgm", "frame", pCodecContext->frame_number);
      // Check if the frame is a planar YUV 4:2:0, 12bpp
      // That is the format of the provided .mp4 file
      // RGB formats will definitely not give a gray image
      // Other YUV image may do so, but untested, so give a warning
      spdlog::info("Processing frame_filename {}", frame_filename);
      if (pFrame->format != AV_PIX_FMT_YUV420P) {
        spdlog::info(
          "Warning: the generated file may not be a grayscale image, but could e.g. be just the R component if the "
          "video format is RGB");
      }
      // save a grayscale frame into a .pgm file
      const int buffer_size = av_image_get_buffer_size(
        static_cast<AVPixelFormat>(pFrame->format), pFrame->width, pFrame->height, pFrame->linesize[0]);
      save_gray_frame(
        std::span(pFrame->data[0], buffer_size), pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);
    }
  }
  return 0;
}
}// namespace

int main(int argc, char **argv) {
  if (argc == 1) {
    spdlog::error("Usage screen_capture_server [file_path]");
    return 1;
  }
  auto args = std::span(argv, size_t(argc));
  const absl::string_view path = args[1];
  spdlog::info("path: {}", path);
  // AVFormatContext holds the header information from the format (Container)
  // Allocating memory for this component
  // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
  AVFormatContext *pFormatContext = avformat_alloc_context();
  if (pFormatContext == nullptr) {
    spdlog::info("ERROR could not allocate memory for Format Context");
    return -1;
  }

  spdlog::info("opening the input file {} and loading format (container) header", path);
  // Open the file and read its header. The codecs are not opened.
  // The function arguments are:
  // AVFormatContext (the component we allocated memory for),
  // url (filename),
  // AVInputFormat (if you pass nullptr it'll do the auto detect)
  // and AVDictionary (which are options to the demuxer)
  // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
  if (avformat_open_input(&pFormatContext, args[1], nullptr, nullptr) != 0) {
    spdlog::info("ERROR could not open the file");
    return -1;
  }

  // now we have access to some information about our file
  // since we read its header we can say what format (container) it's
  // and some other information related to the format itself.
  spdlog::info("format {}, duration {} us, bit_rate {}",
    pFormatContext->iformat->name,
    pFormatContext->duration,
    pFormatContext->bit_rate);

  spdlog::info("finding stream info from format");
  // read Packets from the Format to get stream information
  // this function populates pFormatContext->streams
  // (of size equals to pFormatContext->nb_streams)
  // the arguments are:
  // the AVFormatContext
  // and options contains options for codec corresponding to i-th stream.
  // On return each dictionary will be filled with options that were not found.
  // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
  if (avformat_find_stream_info(pFormatContext, nullptr) < 0) {
    spdlog::info("ERROR could not get the stream info");
    return -1;
  }

  // the component that knows how to enCOde and DECode the stream
  // it's the codec (audio or video)
  // http://ffmpeg.org/doxygen/trunk/structAVCodec.html
  const AVCodec *pCodec = nullptr;
  // this component describes the properties of a codec used by the stream i
  // https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
  AVCodecParameters *pCodecParameters = nullptr;
  unsigned int video_stream_index = 0;
  bool stream_found = false;

  // loop though all the streams and print its main information
  // #pragma clang diagnostic ignored "-warnings-as-errors"
  // #pragma GCC diagnostic ignored "-warnings-as-errors"
  auto streams = std::span(pFormatContext->streams, size_t(pFormatContext->nb_streams));
  for (unsigned int i = 0; i < pFormatContext->nb_streams; i++) {
    AVCodecParameters *pLocalCodecParameters = nullptr;
    pLocalCodecParameters = streams[i]->codecpar;
    spdlog::info("AVStream->time_base before open coded {}/{}", streams[i]->time_base.num, streams[i]->time_base.den);
    spdlog::info(
      "AVStream->r_frame_rate before open coded {}/{}", streams[i]->r_frame_rate.num, streams[i]->r_frame_rate.den);
    spdlog::info("AVStream->start_time {}", streams[i]->start_time);
    spdlog::info("AVStream->duration {}", streams[i]->duration);

    spdlog::info("finding the proper decoder (CODEC)");

    // AVCodec *pLocalCodec = nullptr;

    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

    if (pLocalCodec == nullptr) {
      spdlog::info("ERROR unsupported codec!");
      // In this example if the codec is not found we just skip it
      continue;
    }

    // when the stream is a video we store its index, codec parameters and codec
    if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      if (!stream_found) {
        stream_found = true;
        video_stream_index = i;
        pCodec = pLocalCodec;
        pCodecParameters = pLocalCodecParameters;
      }

      spdlog::info("Video Codec: resolution {} x {}", pLocalCodecParameters->width, pLocalCodecParameters->height);
    } else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
      spdlog::info("Audio Codec: {} channels, sample rate {}",
        pLocalCodecParameters->channels,
        pLocalCodecParameters->sample_rate);
    }

    // print its name, id and bitrate
    spdlog::info("\tCodec {} ID {} bit_rate {}", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
  }
  if (pCodec == nullptr || pCodecParameters == nullptr) { return 1; }
  spdlog::info("Stream index {}", video_stream_index);
  // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
  AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
  // if (pCodecContext == nullptr) {
  //   spdlog::info("failed to allocated memory for AVCodecContext");
  //   return -1;
  // }

  // Fill the codec context based on the values from the supplied codec parameters
  // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
  if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0) {
    spdlog::info("failed to copy codec params to codec context");
    return -1;
  }

  // Initialize the AVCodecContext to use the given AVCodec.
  // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
  if (avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
    spdlog::info("failed to open codec through avcodec_open2");
    return -1;
  }

  // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
  AVFrame *pFrame = av_frame_alloc();
  // if (pFrame == nullptr) {
  //   spdlog::info("failed to allocate memory for AVFrame");
  //   return -1;
  // }
  // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
  AVPacket *pPacket = av_packet_alloc();
  // if (pPacket == nullptr) {
  //   spdlog::info("failed to allocate memory for AVPacket");
  //   return -1;
  // }

  int response = 0;
  constexpr static int kMaxPackets = 8;
  int how_many_packets_to_process = kMaxPackets;
  // fill the Packet with data from the Stream
  // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61
  while (av_read_frame(pFormatContext, pPacket) >= 0) {
    // if it's the video stream
    if (pPacket->stream_index == static_cast<int>(video_stream_index)) {
      spdlog::info("AVPacket->pts {}", pPacket->pts);
      response = decode_packet(pPacket, pCodecContext, pFrame);
      spdlog::info("Decode respone {}", response);
      // if (response < 0) { break; }
      // stop it, otherwise we'll be saving hundreds of frames
      if (--how_many_packets_to_process <= 0) { break; }
    }
    // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
    av_packet_unref(pPacket);
  }

  spdlog::info("releasing all the resources");

  avformat_close_input(&pFormatContext);
  av_packet_free(&pPacket);
  av_frame_free(&pFrame);
  avcodec_free_context(&pCodecContext);
  return 0;

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
  return 0;
}