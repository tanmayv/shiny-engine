#include <absl/status/statusor.h>
#include <span>
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

// template <typename T, typename... Types>
// void CreatePipeline()
// {

// };



// class Decoder {
//     public:
//     char* Process(char*);
// };

// class FramesReader {
//     public:
//     char* Process(std::string filename);
// };

// auto display = [](char* ) -> void {};


// CreatePipeline<Decoder, FramesReader>();

// int main() {
//     return 0;
// }

// template<typename FirstStage, typename... Stages, typename LastStage>
// class Pipeline {
//     public:
//     void Add(std::tuple<std::string_view stage_name, std::variant<Stages>> stage) {
//        std::cout << "Adding stage " << stage_name << '\n';
//        stages.push_back(std::move(stage));
//     }

//     decltype(LastStage::Step()) 
//     private:
//     std::vector<std::variant<FirstStage, Stages, LastStage>> stages;
// }