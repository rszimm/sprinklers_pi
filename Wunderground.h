// Weather.h
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
	Weather::ReturnVals GetVals(const char * key, uint32_t zip, const char * pws, bool usePws) const override;
private:
	const char* m_wundergroundAPIHost;
};

#endif
