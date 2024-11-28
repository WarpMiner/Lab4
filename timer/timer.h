#ifndef TIMER_H
#define TIMER_H

#include <iostream>
#include <chrono>

class Timer {
public:
    Timer();
    ~Timer();
private:
    std::chrono::time_point<std::chrono::_V2::system_clock> start, end;
};

#include "timer.cpp"

#endif
