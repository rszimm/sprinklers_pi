// DarkSky.cpp
// This file manages the retrieval of Weather related information and adjustment of durations
//   from DarkSky

#include "DarkSky.h"
#include "core.h"
#include "port.h"
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

DarkSky::DarkSky(void)
{
	 m_darkSkyAPIHost="api.darksky.net";
}

static void ParseResponse(json &data, Weather::ReturnVals * ret)
{
	freeMemory();
	ret->valid = false;
	ret->maxhumidity = -999;
	ret->minhumidity = 999;

	float temp=0;
	float wind=0;
	float precip=0;
	float uv=0;
	short humidity;
	short i=0;

	try {
		for (auto &hour : data["hourly"]["data"]) {
			temp += hour["temperature"].get<float>();
			wind += hour["windSpeed"].get<float>();
			precip += hour["precipIntensity"].get<float>();
			uv += hour["uvIndex"].get<float>();
			humidity = (short) std::round(hour["humidity"].get<float>() * 100.0);
			if (humidity > ret->maxhumidity) {
				ret->maxhumidity = humidity;
			}
			if (humidity < ret->minhumidity) {
				ret->minhumidity = humidity;
			}
			i++;
		}
		if (i > 0) {
			ret->valid = true;
			ret->meantempi = (short) std::round(temp/i);
			ret->windmph = (short) std::round(wind/i * WIND_FACTOR);
			ret->precipi = (short) std::round(precip * PRECIP_FACTOR); // we want total not average
			ret->UV = (short) std::round(uv/i * UV_FACTOR);
		}
	} catch(std::exception &err) {
		trace(err.what());
	}


	if (ret->maxhumidity == -999 || ret->maxhumidity > 100) {
		ret->maxhumidity = NEUTRAL_HUMIDITY;
	}
	if (ret->minhumidity == 999 || ret->minhumidity < 0) {
		ret->minhumidity = NEUTRAL_HUMIDITY;
	}

	trace("Parsed the following values:\ntemp: %d\nwind: %0.2f\nprecip: %0.2f\nuv: %0.2f\n",
			ret->meantempi, ret->windmph/WIND_FACTOR, ret->precipi/PRECIP_FACTOR, ret->UV/UV_FACTOR);
}

static void GetData(const Weather::Settings & settings,const char *m_darkSkyAPIHost,time_t timstamp, Weather::ReturnVals * ret)
{
	char cmd[255];
	
	snprintf(cmd, sizeof(cmd), "/usr/bin/curl -sS -o /tmp/darksky.json 'https://%s/forecast/%s/%s,%ld?exclude=currently,daily,minutely,flags'", m_darkSkyAPIHost, settings.apiSecret, settings.location, timstamp);
	// trace("cmd: %s\n",cmd);
	
	FILE *fh;
	char buf[255];
	
	buf[0]=0;
	
	if ((fh = popen(cmd, "r")) != NULL) {
	    size_t byte_count = fread(buf, 1, BUFSIZ - 1, fh);
	    buf[byte_count] = 0;
	}
	
	(void) pclose(fh);
	trace("curl error output: %s\n",buf);

	json j;
	std::ifstream ifs("/tmp/darksky.json");
	ifs >> j;
	
	ParseResponse(j, ret);

	ifs.close();
	
	if (!ret->valid)
	{
		if (ret->keynotfound)
			trace("Invalid DarkSky Key\n");
		else
			trace("Bad DarkSky Response\n");
	}
}

Weather::ReturnVals DarkSky::GetVals(void) const
{
	Settings settings = GetSettings();
	return GetVals(settings);
}

Weather::ReturnVals DarkSky::GetVals(const Weather::Settings & settings) const
{
	ReturnVals vals = {0};
	const time_t 	local_now = nntpTimeServer.LocalNow();

	// today
	trace("Get Today's Weather\n");
	GetData(settings, m_darkSkyAPIHost, local_now, &vals);
	if (vals.valid) {
		// save today's values
		short precip_today = vals.precipi;
		short uv_today = vals.UV;

		// yesterday
		trace("Get Yesterday's Weather\n");
		GetData(settings, m_darkSkyAPIHost, local_now - 24 * 3600, &vals);
		if (vals.valid) {
			// restore today's values
			vals.precip_today = precip_today;
			vals.UV = uv_today;
		}
	}
	
	return vals;
}
