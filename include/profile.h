#ifndef PROFILE
#define PROFILE

#include "avrdevice.h"
#include "funktor.h"
#include "systemclock.h"

class ProfileFunktor : public Funktor {

protected:
  AvrDevice *dev;
  std::string name;
  unsigned int indent;
  SystemClockOffset clockStart;

public:
  ProfileFunktor(AvrDevice *_dev, std::string _name, unsigned int _indent)
      : dev{_dev}, indent{_indent}, name{_name},
        clockStart{SystemClock::Instance().GetCurrentTime()} {}
  ~ProfileFunktor() {}
  void operator()();
  Funktor *clone() { return new ProfileFunktor(*this); }
};

#endif