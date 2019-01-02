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

int Weather::GetScale(const char * key, uint32_t zip, const char * pws, bool usePws) const
{
	ReturnVals vals = GetVals(key, zip, pws, usePws);
	return GetScale(vals);
}

int Weather::GetScale(const ReturnVals & vals) const
{
	if (!vals.valid)
		return 100;
	const int humid_factor = NEUTRAL_HUMIDITY - (vals.maxhumidity + vals.minhumidity) / 2;
	const int temp_factor = (vals.meantempi - 70) * 4;
	const int rain_factor = (vals.precipi + vals.precip_today) * -2;
	const int adj = spi_min(spi_max(0, 100+humid_factor+temp_factor+rain_factor), 200);
	trace(F("Adjusting H(%d)T(%d)R(%d):%d\n"), humid_factor, temp_factor, rain_factor, adj);
	return adj;
}
