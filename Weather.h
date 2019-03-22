// Weather.h
// This file manages the retrieval of Weather related information and adjustment of durations
//   from Weather Underground
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#ifndef _WEATHER_h
#define _WEATHER_h

#include "port.h"
#include "settings.h"

#define NEUTRAL_HUMIDITY 30

#define PRECIP_FACTOR 100.0
#define WIND_FACTOR 10.0
#define UV_FACTOR 10.0

class Weather
{
public:
	struct ReturnVals
	{
		bool valid;
		bool keynotfound;
		const char* resolvedIP;
		// Yesterday's Values
		short minhumidity;
		short maxhumidity;
		short meantempi;
		short precipi;
		short windmph;
		// Today's Values
		short precip_today;
		short UV;
	};
	struct Settings
	{
		char key[17]; //16 character hex
		char apiId[LEN_APIID+1]; // 32 character string
		char apiSecret[LEN_APISECRET+1]; // 64 character string
		uint32_t zip;
		char pws[LEN_PWS+1]; // 11 character string
		bool usePws;
		char location[LEN_LOC+1]; // 50 character string
	};
public:
	static Settings GetSettings(void);
	int16_t GetScale(void) const;
	int16_t GetScale(const Settings & settings) const;
	int16_t GetScale(const ReturnVals & vals) const;
	ReturnVals GetVals(void) const;
    ReturnVals GetVals(const Settings & settings) const;
};

#endif
