// OpenMeteo.h
// This file manages the retrieval of Weather related information and adjustment of durations
//   from OpenMeteo
//

#ifndef _OM_h
#define _OM_h

#include "port.h"
#include "Weather.h"

class OpenMeteo : public Weather
{
public:
	OpenMeteo(void);
private:
	const char* m_OpenMeteoAPIHost;
	Weather::ReturnVals InternalGetVals(const Weather::Settings & settings) const;
};

#endif
