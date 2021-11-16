#include "GameTime.h"

Clock ztime::clock[128] = { TimePoint() };
//std::array<Clock, 128> ztime::clock = std::array<Clock, 128>();

TimePoint ztime::Main()
{
    return clock[CLOCK_MAIN].Now();
}

TimePoint ztime::Game()
{
    return clock[CLOCK_GAME].Now();
}