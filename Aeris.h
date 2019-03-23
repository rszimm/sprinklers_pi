// Aeris.h
// This file manages the retrieval of Weather related information and adjustment of durations
//   from Aeris (www.aerisweather.com)
//

#ifndef _AERIS_h
#define _AERIS_h

#include "port.h"
#include "Weather.h"

class Aeris : public Weather
{
public:
	Aeris(void);
private:
	const char* m_aerisAPIHost;
	Weather::ReturnVals InternalGetVals(const Weather::Settings & settings) const;
};

#endif
