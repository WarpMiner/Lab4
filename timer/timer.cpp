#include "timer.h"

Timer::Timer() {
    start = std::chrono::high_resolution_clock::now();
}

Timer::~Timer() {
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    std::cout << "Duration: " << duration.count() << " seconds" << std::endl;
}
