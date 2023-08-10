#include <absl/status/statusor.h>
#include <span>
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}
namespace ffmpeg {

class FfmpegDecoder {
public:
  FfmpegDecoder(const AVCodec *codec, AVCodecContext *codec_context, AVFrame *frame)
    : codec_(codec), codec_context_(codec_context), frame_(frame) {}

  static absl::StatusOr<FfmpegDecoder> Create(AVCodecID codec_id);

  ~FfmpegDecoder() {
    av_frame_free(&frame_);
    avcodec_close(codec_context_);
  }

  template<typename Function>
  void DecodeStreamData(std::span<uint8_t> data, Function callback);

private:
  const AVCodec *codec_;
  AVCodecContext *codec_context_;
  AVFrame *frame_;
};
}// namespace ffmpeg