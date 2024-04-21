// OpenWeather3.cpp
// This file manages the retrieval of Weather related information and adjustment of durations
//   from OpenWeather

#include "config.h"
#ifdef WEATHER_OPENWEATHER3

#include "OpenWeather3.h"
#include "core.h"
#include "port.h"
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

OpenWeather3::OpenWeather3(void)
{
	 m_openWeatherAPIHost="api.openweathermap.org";
}

static void ParseResponse(json &data, Weather::ReturnVals * ret)
{
	freeMemory();
	ret->valid = false;
	ret->maxhumidity = -999;
	ret->minhumidity = 999;

	float temp=0;
	float wind=0;
	float rain=0;
	float precip=0;
	short humidity;
	short i=0;

	try {
		for (auto &hour : data["hourly"]) {
            rain = 0;
			temp += hour["temp"].get<float>();
			wind += hour["wind_speed"].get<float>();
			if (hour.count("rain") > 0 && hour["rain"].count("1h") > 0) {
                rain = hour["rain"]["1h"].get<float>();
                precip += rain;
			}
			humidity = hour["humidity"].get<short>();
/*
            trace("collected the following values:\ntemp: %0.2f\nwind: %0.2f\nprecip: %0.2f\nhumid: %0.2f\n",
                  hour["temp"].get<float>(), hour["wind_speed"].get<float>(), rain, humidity);

            trace("totals so far:\ntemp: %0.2f\nwind: %0.2f\nprecip: %0.2f\n\n",
                  temp, wind, precip);
*/
			if (humidity > ret->maxhumidity) {
				ret->maxhumidity = humidity;
			}
			if (humidity < ret->minhumidity) {
				ret->minhumidity = humidity;
			}
			if (++i > 24) {
			    break;
			}
		}
if (i==0){
for (auto &hour : data["data"]) {
            rain = 0;
			temp += hour["temp"].get<float>();
			wind += hour["wind_speed"].get<float>();
			if (hour.count("rain") > 0 && hour["rain"].count("1h") > 0) {
                rain = hour["rain"]["1h"].get<float>();
                precip += rain;
			}
			humidity = hour["humidity"].get<short>();
/*
            trace("collected the following values:\ntemp: %0.2f\nwind: %0.2f\nprecip: %0.2f\nhumid: %0.2f\n",
                  hour["temp"].get<float>(), hour["wind_speed"].get<float>(), rain, humidity);
            trace("totals so far:\ntemp: %0.2f\nwind: %0.2f\nprecip: %0.2f\n\n",
                  temp, wind, precip);
*/
			if (humidity > ret->maxhumidity) {
				ret->maxhumidity = humidity;
			}
			if (humidity < ret->minhumidity) {
				ret->minhumidity = humidity;
			}
			if (++i > 24) {
			    break;
			}
		}
}

if (i > 0) {
			ret->valid = true;
			ret->meantempi = (short) std::round(temp/i);
			ret->windmph = (short) std::round(wind/i * WIND_FACTOR);
			ret->precipi = (short) std::round(precip / MM_TO_IN * PRECIP_FACTOR); // we want total not average
			ret->UV = (short) std::round(data["current"]["uvi"].get<float>() * UV_FACTOR);
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

static void GetData(const Weather::Settings & settings,const char *m_openWeatherAPIHost,time_t timestamp, Weather::ReturnVals * ret)
{
	char cmd[255];

	// split location into lat, long
	char * loc = strdup(settings.location);
	char * lat = strtok(loc, ", ");
	char * lon = strtok(NULL, ", ");

	// get weather json
	if (timestamp != 0) {
        snprintf(cmd, sizeof(cmd),
                 "/usr/bin/curl -sS -o /tmp/openWeather.json 'https://%s/data/3.0/onecall/timemachine?appid=%s&lat=%s&lon=%s&dt=%ld&units=imperial'",
                 m_openWeatherAPIHost, settings.apiSecret, lat, lon, timestamp);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "/usr/bin/curl -sS -o /tmp/openWeather.json 'https://%s/data/3.0/onecall?appid=%s&lat=%s&lon=%s&units=imperial'",
                 m_openWeatherAPIHost, settings.apiSecret, lat, lon);
	}
	//trace("cmd: %s\n",cmd);
	
	FILE *fh;
	char buf[255];
	
	buf[0]=0;
	
	if ((fh = popen(cmd, "r")) != NULL) {
	    size_t byte_count = fread(buf, 1, sizeof(buf) - 1, fh);
	    buf[byte_count] = 0;
	}
	
	(void) pclose(fh);
	trace("curl error output: %s\n",buf);

	json j;
	std::ifstream ifs("/tmp/openWeather.json");
	ifs >> j;
	
	ParseResponse(j, ret);

	ifs.close();
	
	if (!ret->valid)
	{
		if (ret->keynotfound)
			trace("Invalid OpenWeather Key\n");
		else
			trace("Bad OpenWeather Response\n");
	}
}

Weather::ReturnVals OpenWeather3::InternalGetVals(const Weather::Settings & settings) const
{
	ReturnVals vals = {0};
	const time_t 	now = nntpTimeServer.utcNow();

	// today
	trace("Get Today's Weather\n");
	GetData(settings, m_openWeatherAPIHost, 0, &vals);
	if (vals.valid) {
		// save today's values
		short precip_today = vals.precipi;
		short uv_today = vals.UV;

		//trace("local hour: %d\n", nntpTimeServer.LocalHour());
		if (nntpTimeServer.LocalHour() >= 8) {
            trace("Get Today's Weather for the hours between midnight and now\n");
            GetData(settings, m_openWeatherAPIHost, now - 8 * 3600, &vals);
            if (vals.valid) {
                // add precip to today's values
                precip_today += vals.precipi;
            }
		}

		// yesterday
		trace("Get Yesterday's Weather\n");
		GetData(settings, m_openWeatherAPIHost, now - 24 * 3600, &vals);
		if (vals.valid) {
			// restore today's values
			vals.precip_today = precip_today;
			vals.UV = uv_today;
		}
	}
	
	return vals;
}

#endif
