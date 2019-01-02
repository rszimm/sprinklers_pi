// Weather.cpp
// This file manages the retrieval of Weather related information and adjustment of durations
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include "Weather.h"
#include "core.h"
#include "port.h"
#include <string.h>
#include <stdlib.h>

Weather::Settings Weather::GetSettings(void) {
	Settings settings = {0};

	GetApiKey(settings.key);
	GetApiId(settings.apiId);
	GetApiSecret(settings.apiSecret);
	GetPWS(settings.pws);
	GetLoc(settings.location);

	settings.zip = GetZip();
	settings.usePws = GetUsePWS();

	return settings;
}

int16_t Weather::GetScale(void) const
{
	ReturnVals vals = GetVals();
	return GetScale(vals);
}

int16_t Weather::GetScale(const Weather::Settings & settings) const
{
	ReturnVals vals = GetVals(settings);
	return GetScale(vals);
}

int16_t Weather::GetScale(const ReturnVals & vals) const
{
	if (!vals.valid)
		return 100;
	const int humid_factor = NEUTRAL_HUMIDITY - (vals.maxhumidity + vals.minhumidity) / 2;
	const int temp_factor = (vals.meantempi - 70) * 4;
	const int rain_factor = (vals.precipi + vals.precip_today) * -2;
	const int16_t adj = (uint16_t)spi_min(spi_max(0, 100+humid_factor+temp_factor+rain_factor), 200);
	trace(F("Adjusting H(%d)T(%d)R(%d):%d\n"), humid_factor, temp_factor, rain_factor, adj);
	return adj;
}

Weather::ReturnVals Weather::GetVals(void) const
{
	Settings settings = GetSettings();
	return GetVals(settings);
}

Weather::ReturnVals Weather::GetVals(const Settings & settings) const
{
	// You must override and implement this function
	ReturnVals vals = {0};
	vals.valid = false;
	return vals;
}