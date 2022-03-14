#include "profile.h"

#include <iostream>

void ProfileFunktor::operator()() {
  SystemClockOffset freq = dev->GetClockFreq();
  avr_message("freq: %lld", freq);
  avr_message("starting time: %lld", clockStart);
  avr_message("current time : %lld", SystemClock::Instance().GetCurrentTime());
  SystemClockOffset cycles =
      (SystemClock::Instance().GetCurrentTime() - clockStart) / freq;
  std::cout << std::string(indent * 2, ' ') << "Profiling " << name << " took "
            << cycles << " cycles\n";
}