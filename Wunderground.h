// Wunderground.h
// This file manages the retrieval of Weather related information and adjustment of durations
//   from Weather Underground
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#ifndef _WU_h
#define _WU_h

#include "port.h"
#include "Weather.h"

class Wunderground : public Weather
{
public:
	Wunderground(void);
private:
	const char* m_wundergroundAPIHost;
	Weather::ReturnVals InternalGetVals(const Weather::Settings & settings) const;
};

#endif
