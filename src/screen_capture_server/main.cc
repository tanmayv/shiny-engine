#pragma GCC diagnostic push

// turn off the specific warning. Can also use "-Wall"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wconversion"
// #include "ScreenCapture.h"// NOLINT
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <uvgrtp/lib.hh>
// turn the warnings back on
#pragma GCC diagnostic pop
#include <atomic>
#include <chrono>
#include <span>
#include <fstream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
}

namespace {
constexpr std::string_view kRemoteAddress = "127.0.0.1";
constexpr std::string_view kFilename = "output.mp4";
constexpr uint16_t kRemotePort = 3000;
constexpr uint16_t kBufferSize = 100;
}// namespace

int main() {
  spdlog::info("Starting server");
  uvgrtp::context ctx;
  uvgrtp::session *session = ctx.create_session(kRemoteAddress.data());
  uvgrtp::session *receive_session = ctx.create_session(kRemoteAddress.data());

  const int flags = RCE_SEND_ONLY | RCE_FRAGMENT_GENERIC;
  uvgrtp::media_stream *hvec_stream = session->create_stream(kRemotePort, RTP_FORMAT_GENERIC, flags);
  uvgrtp::media_stream *recv =
    receive_session->create_stream(kRemotePort, RTP_FORMAT_GENERIC, RCE_RECEIVE_ONLY | RCE_FRAGMENT_GENERIC);

  recv->install_receive_hook(nullptr, [](void *arg, uvgrtp::frame::rtp_frame *frame){
    spdlog::info("Received data {}", frame->payload_len);
    uvgrtp::frame::dealloc_frame(frame);
  });
  std::ifstream ifs(kFilename, std::ios::in | std::ios::binary);
  std::array<char, kBufferSize> buffer = {};

  AVFormatContext *pFormatContext = avformat_alloc_context();
  if (pFormatContext == nullptr) {
    spdlog::info("ERROR could not allocate memory for Format Context");
    return -1;
  }

  if (avformat_open_input(&pFormatContext, kFilename.data(), nullptr, nullptr) != 0) {
    spdlog::info("ERROR could not open the file");
    return -1;
  }

  spdlog::info("finding stream info from format");
  if (avformat_find_stream_info(pFormatContext, nullptr) < 0) {
    spdlog::info("ERROR could not get the stream info");
    return -1;
  }

  const AVCodec *pCodec = nullptr;
  AVCodecParameters *pCodecParameters = nullptr;
  unsigned int video_stream_index = 0;
  bool stream_found = false;
  auto streams = std::span(pFormatContext->streams, size_t(pFormatContext->nb_streams));
  for (unsigned int i = 0; i < pFormatContext->nb_streams; i++) {
    AVCodecParameters *pLocalCodecParameters = nullptr;
    pLocalCodecParameters = streams[i]->codecpar;
    const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

    if (pLocalCodec == nullptr) {
      spdlog::info("ERROR unsupported codec!");
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
  AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
  if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0) {
    spdlog::info("failed to copy codec params to codec context");
    return -1;
  }

  if (avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
    spdlog::info("failed to open codec through avcodec_open2");
    return -1;
  }

  AVFrame *pFrame = av_frame_alloc();
  AVPacket *pPacket = av_packet_alloc();
  int response = 0;
  constexpr static int kMaxPackets = 8;
  while (av_read_frame(pFormatContext, pPacket) >= 0) {
    if (pPacket->stream_index == static_cast<int>(video_stream_index)) {
      // spdlog::info("AVPacket->pts {}", pPacket->pts);
      // for (int i = 0; i < 10; i++) {
      //   fmt::print("Hex value {0:x}", pPacket->buf->data[i]);
      // }
      // break;
      auto status = hvec_stream->push_frame(pPacket->buf->data, pPacket->buf->size, 0);
      if (status != RTP_OK) {
        spdlog::error("Error sending bytes");
        break;
      }

      // response = decode_packet(pPacket, pCodecContext, pFrame);
      // spdlog::info("Decode respone {}", response);
      // if (--how_many_packets_to_process <= 0) { break; }
    }
    // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
    av_packet_unref(pPacket);
  }

  spdlog::info("releasing all the resources");


  if (hvec_stream != nullptr && ifs) {
    // do the sending
    while (!ifs.eof()) {
      break;
      spdlog::info("Read into buffer of size {}", buffer.max_size());
      ifs.read(buffer.data(), buffer.max_size());
      spdlog::info("Read {} bytes", ifs.gcount());
    }
    session->destroy_stream(hvec_stream);
    receive_session->destroy_stream(recv);
  } else {
    spdlog::error("Unable to create stream");
  }

  if (session != nullptr) { ctx.destroy_session(session); }

  ctx.destroy_session(receive_session);
  avformat_close_input(&pFormatContext);
  av_packet_free(&pPacket);
  av_frame_free(&pFrame);
  avcodec_free_context(&pCodecContext);

  return 0;
}
// int main() {
//     // auto last = std::chrono::system_clock::now();
//     // auto mons = SL::Screen_Capture::GetMonitors();
//     // auto framgrabber =
//     //   SL::Screen_Capture::CreateCaptureConfiguration([&]() { return mons; })
//     //     ->onNewFrame([&](const SL::Screen_Capture::Image& img, const SL::Screen_Capture::Monitor& ) {
//     //       const auto current = std::chrono::system_clock::now();
//     //       std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(current - last).count()
//     //                 << " milliseconds\n";
//     //       last = current;
//     //     })
//     //     ->start_capturing();
//     // constexpr int kWaitTime = 100;
//     // framgrabber->setFrameChangeInterval(std::chrono::milliseconds(kWaitTime));
//     // std::cin.ignore();
// // auto framgrabber =  SL::Screen_Capture::CreateCaptureConfiguration([]() {
// //     //add your own custom filtering here if you want to capture only some monitors
// //     return SL::Screen_Capture::GetWindows();
// //   })->onFrameChanged([&](const SL::Screen_Capture::Image& , const SL::Screen_Capture::Window& ) {
// //     fmt::print("OnFrameChanged");
// //   })->onNewFrame([&](const SL::Screen_Capture::Image& , const SL::Screen_Capture::Window& ) {
// //     fmt::print("onNewFrame");
// //   })->onMouseChanged([&](const SL::Screen_Capture::Image* , const SL::Screen_Capture::MousePoint& ) {
// //     fmt::print("onMouseChanged");
// //   })->start_capturing();
// }