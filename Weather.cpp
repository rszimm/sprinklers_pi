// Weather.cpp
// This file manages the retrieval of Weather related information and adjustment of durations
//   from Weather Underground
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include "Weather.h"
#include "core.h"
#include "port.h"
#include <string.h>
#include <stdlib.h>

Weather::Weather(void)
{
	 m_wundergroundAPIHost="api.wunderground.com";
}

static void ParseResponse(EthernetClient & client, Weather::ReturnVals * ret)
{
	freeMemory();
	ret->valid = false;
	enum
	{
		FIND_QUOTE1 = 0, PARSING_KEY, FIND_QUOTE2, PARSING_VALUE, PARSING_QVALUE, ERROR
	} current_state = FIND_QUOTE1;
	char recvbuf[100];
	char * recvbufptr = recvbuf;
	char * recvbufend = recvbuf;
	char key[30], val[30];
	char * keyptr = key;
	char * valptr = val;
	while (true)
	{
		if (recvbufptr >= recvbufend)
		{
			int len = client.read((uint8_t*) recvbuf, sizeof(recvbuf));
//			trace(F("Received Bytes:%d\n"), len);
			if (len <= 0)
			{
				if (!client.connected()) {
					trace("Client Disconnected\n");
					break;
				} else {
					continue;  //TODO:  implement a timeout here.  Same in testing parse headers.
				}
			}
			else
			{
				recvbufptr = recvbuf;
				recvbufend = recvbuf + len;
				//trace(recvbuf);
			}
		}
		char c = *(recvbufptr++);

		switch (current_state)
		{
		case FIND_QUOTE1:
			if (c == '"')
			{
				current_state = PARSING_KEY;
				keyptr = key;
			}
			break;
		case PARSING_KEY:
			if (c == '"')
			{
				current_state = FIND_QUOTE2;
				*keyptr = 0;
			}
			else
			{
				if ((keyptr - key) < (long)(sizeof(key) - 1))
				{
					*keyptr = c;
					keyptr++;
				}
			}
			break;
		case FIND_QUOTE2:
			if (c == '"')
			{
				current_state = PARSING_QVALUE;
				valptr = val;
			}
			else if (c == '{')
			{
				current_state = FIND_QUOTE1;
			}
			else if ((c >= '0') && (c <= '9'))
			{
				current_state = PARSING_VALUE;
				valptr = val;
				*valptr = c;
				valptr++;
			}
			break;
		case PARSING_VALUE:
			if (((c >= '0') && (c <= '9')) || (c == '.'))
			{
				*valptr = c;
				valptr++;
			}
			else
			{
				current_state = FIND_QUOTE1;
				*valptr = 0;
			}
			break;
		case PARSING_QVALUE:
			if (c == '"')
			{
				current_state = FIND_QUOTE1;
				*valptr = 0;
				//trace("%s:%s\n", key, val);
				if (strcmp(key, "maxhumidity") == 0)
				{
					ret->valid = true;
					ret->keynotfound = false;
					ret->maxhumidity = atoi(val);
				}
				else if (strcmp(key, "minhumidity") == 0)
				{
					ret->minhumidity = atoi(val);
				}
				else if (strcmp(key, "meantempi") == 0)
				{
					ret->meantempi = atoi(val);
				}
				else if (strcmp(key, "precip_today_in") == 0)
				{
					ret->precip_today = (atof(val) * 100.0);
				}
				else if (strcmp(key, "precipi") == 0)
				{
					ret->precipi = (atof(val) * 100.0);
				}
				else if (strcmp(key, "UV") == 0)
				{
					ret->UV = (atof(val) * 10.0);
				}
				else if (strcmp(key, "meanwindspdi") == 0)
				{
					ret->windmph = (atof(val) * 10.0);
				}
				else if (strcmp(key, "type") == 0)
				{
					if (strcmp(val, "keynotfound") == 0)
						ret->keynotfound = true;
				}

			}
			else
			{
				if ((valptr - val) < (long)(sizeof(val) - 1))
				{
					*valptr = c;
					valptr++;
				}
			}
			break;
		case ERROR:
			break;
		} // case
	} // while (true)
}

int Weather::GetScale(const char * key, uint32_t zip, const char * pws, bool usePws) const
{
	ReturnVals vals = GetVals(key, zip, pws, usePws);
	return GetScale(vals);
}

int Weather::GetScale(const ReturnVals & vals) const
{
	if (!vals.valid)
		return 100;
	const int humid_factor = 30 - (vals.maxhumidity + vals.minhumidity) / 2;
	const int temp_factor = (vals.meantempi - 70) * 4;
	const int rain_factor = (vals.precipi + vals.precip_today) * -2;
	const int adj = min(max(0, 100+humid_factor+temp_factor+rain_factor), 200);
	trace(F("Adjusting H(%d)T(%d)R(%d):%d\n"), humid_factor, temp_factor, rain_factor, adj);
	return adj;
}

Weather::ReturnVals Weather::GetVals(const char * key, uint32_t zip, const char * pws, bool usePws) const
{
	ReturnVals vals = {0};
	EthernetClient client;
	if (client.connect(m_wundergroundAPIHost, 80))
	{
		char getstring[255];
		trace(F("Connected\n"));
		if (usePws)
			snprintf(getstring, sizeof(getstring), "GET http://%s/api/%s/yesterday/conditions/q/pws:%s.json HTTP/1.1\r\n",m_wundergroundAPIHost, key, pws);
		else
			snprintf(getstring, sizeof(getstring), "GET http://%s/api/%s/yesterday/conditions/q/%ld.json HTTP/1.1\r\n",m_wundergroundAPIHost, key, (long) zip);

		//trace("GetString:%s\n",getstring);
		client.write((uint8_t*) getstring, strlen(getstring));
		
		//send host header
		snprintf(getstring, sizeof(getstring), "Host: %s\r\nConnection: close\r\n\r\n",m_wundergroundAPIHost);
		//trace("GetString:%s\n",getstring);
		client.write((uint8_t*) getstring, strlen(getstring));

		ParseResponse(client, &vals);
		vals.resolvedIP=client.GetIpAddress();

		client.stop();
		if (!vals.valid)
		{
			if (vals.keynotfound)
				trace("Invalid WUnderground Key\n");
			else
				trace("Bad WUnderground Response\n");
		}
	}
	else
	{
		trace(F("connection failed\n"));
		client.stop();
	}
	return vals;
}
