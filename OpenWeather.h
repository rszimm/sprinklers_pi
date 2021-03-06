// OpenWeather.h
// This file manages the retrieval of Weather related information and adjustment of durations
//   from OpenWeather
//

#ifndef _OW_h
#define _OW_h

#include "port.h"
#include "Weather.h"

class OpenWeather : public Weather
{
public:
	OpenWeather(void);
private:
	const char* m_openWeatherAPIHost;
	Weather::ReturnVals InternalGetVals(const Weather::Settings & settings) const;
};

#endif
