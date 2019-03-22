// DarkSky.h
// This file manages the retrieval of Weather related information and adjustment of durations
//   from DarkSky
//

#ifndef _DS_h
#define _DS_h

#include "port.h"
#include "Weather.h"

class DarkSky : public Weather
{
public:
	DarkSky(void);
	Weather::ReturnVals GetVals(void) const;
	Weather::ReturnVals GetVals(const Weather::Settings & settings) const;
private:
	const char* m_darkSkyAPIHost;
};

#endif
