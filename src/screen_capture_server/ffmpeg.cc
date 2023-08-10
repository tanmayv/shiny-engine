#include "ffmpeg.h"
#include <absl/status/statusor.h>
#include <span>
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

namespace ffmpeg {
namespace {

  std::string error_string(int err_num) {
    constexpr int kMaxSize = 64;
    std::array<char, kMaxSize> buffer{ 0 };
    av_make_error_string(buffer.data(), kMaxSize, err_num);
    return std::string{ std::begin(buffer), std::end(buffer) };
  }

}// namespace
absl::StatusOr<FfmpegDecoder> FfmpegDecoder::Create(AVCodecID codec_id) {
  const AVCodec *codec = avcodec_find_decoder(codec_id);
  if (codec == nullptr) { return absl::InvalidArgumentError("Could not find codec"); }
  AVCodecContext *codec_context = avcodec_alloc_context3(codec);
  if (codec_context == nullptr) {
    return absl::InvalidArgumentError("Could not allocate context");
  }
  if (const int ret = avcodec_open2(codec_context, codec, nullptr); ret < 0) {
    return absl::InvalidArgumentError(error_string(ret));
  }
  AVFrame* frame = av_frame_alloc();
  return FfmpegDecoder(codec, codec_context, frame);
}
}// namespace ffmpeg