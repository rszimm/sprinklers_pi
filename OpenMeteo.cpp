// OpenMeteo.cpp
// This file manages the retrieval of Weather related information and adjustment of durations
//   from OpenMeteo

#include "config.h"
#ifdef WEATHER_OPENMETEO

#include "OpenMeteo.h"
#include "core.h"
#include "port.h"
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

OpenMeteo::OpenMeteo(void)
{
	 m_OpenMeteoAPIHost="api.open-meteo.com";
}

static void ParseResponse(json &data, Weather::ReturnVals * ret)
{
	freeMemory();
	ret->valid = false;
	ret->maxhumidity = -999;
	ret->minhumidity = 999;
	

	float temp=0;
	float v = 0;
	float sum = 0;
	float wind = 0;
	float uv = 0;
	float rainYesterday = 0;
	float rainToday = 0;
	short i = 0;
	short size = 0;
	json day;
	json aVal;
	json hours;
	bool valid = true;

	try
	{	
		day = data["daily"];
		hours = data["hourly"];

		//*************************************
		// avarage temperature from yesterday
		//*************************************
		sum = 0;
		aVal = hours["temperature_2m"];
		size = aVal.size();  
		if (size >= 24)
		{
			// use temperature from yesterday only (0...23)
			for (i = 0; i < 24; i++)
			{
				v = aVal[i];
				sum += v;
			}	
			temp = sum / 24;
		}	
		else valid = false;

		
		//*************************************
		// max humidity from yesterday
		//*************************************
		aVal = day["relative_humidity_2m_max"];
		size = aVal.size();
		if (size > 0)
		{
			ret->maxhumidity = aVal[0];
		}
		else valid = false;

		
		//*************************************
		// min humidity from yesterday
		//*************************************
		aVal = day["relative_humidity_2m_min"];
		size = aVal.size();
		if (size > 0)
		{
			ret->minhumidity = aVal[0];
		}
		else valid = false;

		//*************************************
		// rain total
		//*************************************
		sum = 0;
		aVal = day["rain_sum"];
		if (aVal.size() == 2)
		{
			rainYesterday = aVal[0]; //total rain yesterday
			rainToday = aVal[1];     //total rain today
		}	
		else valid = false;


		//*************************************
		// Wind spead mean from yesterday
		//*************************************
		sum = 0;
		aVal = day["wind_speed_10m_mean"];
		size = aVal.size();
		if (size > 0)
		{
			wind = aVal[0];
		}	
		else valid = false;

		//*************************************
		// uv index max from today
		//*************************************
		sum = 0;
		aVal = day["uv_index_max"];
		if (aVal.size() > 1)
		{
			if (!aVal[1].is_null()) uv = aVal[1]; 
		}	
		else valid = false;


		ret->valid = valid;
		ret->meantempi = (short) std::round(temp);
		ret->windmph = (short) std::round(wind * WIND_FACTOR);
		ret->precipi = (short) std::round(rainYesterday * PRECIP_FACTOR); 
		ret->precip_today = (short) std::round(rainToday * PRECIP_FACTOR);
		ret->UV = (short) std::round(uv * UV_FACTOR);

	} catch(std::exception &err) 
	{
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

static void GetData(const Weather::Settings & settings,const char *m_OpenMeteoAPIHost,time_t timestamp, Weather::ReturnVals * ret)
{
	char cmd[500];

	// split location into lat, long
	char * loc = strdup(settings.location);
	char * lat = strtok(loc, ", ");
	char * lon = strtok(NULL, ", ");

	// get weather json
	if (timestamp != 0) {
        snprintf(cmd, sizeof(cmd),
                 "/usr/bin/curl -sS -o /tmp/OpenMeteo.json 'https://%s/v1/forecast?latitude=%s&longitude=%s&hourly=temperature_2m&daily=relative_humidity_2m_max,relative_humidity_2m_min,rain_sum,wind_speed_10m_mean,uv_index_max&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timezone=Europe%%2FBerlin&past_days=1&forecast_days=1&models=icon_seamless'",
                 m_OpenMeteoAPIHost, lat, lon);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "/usr/bin/curl -sS -o /tmp/OpenMeteo.json 'https://%s/v1/forecast?latitude=%s&longitude=%s&hourly=temperature_2m&daily=relative_humidity_2m_max,relative_humidity_2m_min,rain_sum,wind_speed_10m_mean,uv_index_max&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timezone=Europe%%2FBerlin&past_days=1&forecast_days=1&models=icon_seamless'",
                 m_OpenMeteoAPIHost, lat, lon);
	}
	trace("cmd: %s\n",cmd);
	
	FILE *fh;
	char buf[500];
	
	buf[0]=0;
	
	if ((fh = popen(cmd, "r")) != NULL) {
	    size_t byte_count = fread(buf, 1, sizeof(buf) - 1, fh);
	    buf[byte_count] = 0;
	}
	
	(void) pclose(fh);
	trace("curl error output: %s\n",buf);

	json j;
	std::ifstream ifs("/tmp/OpenMeteo.json");
	ifs >> j;
	
	ParseResponse(j, ret);

	ifs.close();
	
	if (!ret->valid)
	{
		if (ret->keynotfound)
			trace("Invalid OpenMeteo Key\n");
		else
			trace("Bad OpenMeteo Response\n");
	}
}

Weather::ReturnVals OpenMeteo::InternalGetVals(const Weather::Settings & settings) const
{
	ReturnVals vals = {0};
	const time_t 	now = nntpTimeServer.utcNow();

	// today and yesterday
	trace("Get weather for today and from yesterday\n");
	GetData(settings, m_OpenMeteoAPIHost, 0, &vals);

	static char strDate[300];
	struct tm * ti = localtime (&now);
	snprintf(strDate, sizeof(strDate), "%s  %.2d.%.2d.%.4d %.2d:%.2d:%.2d ", m_OpenMeteoAPIHost, ti->tm_mday, ti->tm_mon+1, 1900 + ti->tm_year, ti->tm_hour, ti->tm_min, ti->tm_sec);
	vals.resolvedIP = strDate;

	return vals;
}

#endif
