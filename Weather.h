// Weather.h
// This file manages the retrieval of Weather related information and adjustment of durations
//   from Weather Underground
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#ifndef _WEATHER_h
#define _WEATHER_h

#include "port.h"

class Weather
{
public:
	struct ReturnVals
	{
		bool valid;
		bool keynotfound;
		short minhumidity;
		short maxhumidity;
		short meantempi;
		short precip_today;
		short precipi;
		short windmph;
		short UV;
		const char* resolvedIP;
	};
public:
	Weather(void);
	int GetScale(const char * key, uint32_t zip, const char * pws, bool usePws) const;
	int GetScale(const ReturnVals & vals) const;
	ReturnVals GetVals(const char * key, uint32_t zip, const char * pws, bool usePws) const;
private:
	const char* m_wundergroundAPIHost;
};

#endif
