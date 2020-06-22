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
private:
	const char* m_darkSkyAPIHost;
	Weather::ReturnVals InternalGetVals(const Weather::Settings & settings) const;
};

#endif
