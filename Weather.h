// Weather.h
// This file manages the retrieval of Weather related information and adjustment of durations
//   from Weather Underground
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//  $Id: Weather.h 609 2013-06-28 00:25:31Z rzimmerman $
//

#ifndef _WEATHER_h
#define _WEATHER_h

#include "port.h"

class Weather
{
public:
	Weather(void);
	int GetScale(const IPAddress & ip, const char * key, uint32_t zip) const;
};

#endif
