#pragma GCC diagnostic push 

// turn off the specific warning. Can also use "-Wall"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wconversion"
#include "ScreenCapture.h" // NOLINT
#include <fmt/format.h>
// turn the warnings back on
#pragma GCC diagnostic pop
#include <iostream>
#include <chrono>
#include <atomic>


int main() {
    auto mons = SL::Screen_Capture::GetMonitors();
    auto framgrabber =
      SL::Screen_Capture::CreateCaptureConfiguration([&]() { return mons; })
        ->onNewFrame([&](const SL::Screen_Capture::Image &img, const SL::Screen_Capture::Monitor &monitor) {
          fmt::print("IMG {}", img.isContiguous);
          fmt::print("Monitor {}", monitor.Name);
        })
        ->start_capturing();
    constexpr int kWaitTime = 100;
    framgrabber->setFrameChangeInterval(std::chrono::milliseconds(kWaitTime));
    std::cin.ignore();
// auto framgrabber =  SL::Screen_Capture::CreateCaptureConfiguration([]() {
//     //add your own custom filtering here if you want to capture only some monitors
//     return SL::Screen_Capture::GetWindows();
//   })->onFrameChanged([&](const SL::Screen_Capture::Image& , const SL::Screen_Capture::Window& ) {
//     fmt::print("OnFrameChanged");
//   })->onNewFrame([&](const SL::Screen_Capture::Image& , const SL::Screen_Capture::Window& ) {
//     fmt::print("onNewFrame");
//   })->onMouseChanged([&](const SL::Screen_Capture::Image* , const SL::Screen_Capture::MousePoint& ) {
//     fmt::print("onMouseChanged");
//   })->start_capturing();
}