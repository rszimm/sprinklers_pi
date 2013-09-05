// Weather.cpp
// This file manages the retrieval of Weather related information and adjustment of durations
//   from Weather Underground
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//  $Id: Weather.cpp 613 2013-06-30 23:11:59Z rzimmerman $
//

#include "Weather.h"
#include "core.h"
#include "port.h"
#include <string.h>
#include <stdlib.h>

Weather::Weather(void)
{
}

struct ReturnVals
{
	short minhumidity;
	short maxhumidity;
	short meantempi;
	short precip_today;
	short precipi;
};

static void ParseResponse(EthernetClient & client, ReturnVals * ret)
{
	freeMemory();
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
				if (!client.connected())
					break;
				else
					continue;  //TODO:  implement a timeout here.  Same in testing parse headers.
			}
			else
			{
				recvbufptr = recvbuf;
				recvbufend = recvbuf + len;
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
			if ((c >= '0') && (c <= '9'))
			{
				*valptr = c;
				valptr++;
			}
			else
			{
				current_state = FIND_QUOTE1;
				*valptr = 0;
				// TODO:  Parse things here.
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

int Weather::GetScale(const IPAddress & ip, const char * key, uint32_t zip) const
{
	EthernetClient client;
	if (client.connect(ip, 80))
	{
		char getstring[80];
		trace(F("Connected\n"));
		snprintf(getstring, sizeof(getstring), "GET /api/%s/yesterday/conditions/q/%ld.json HTTP/1.0\n\n", key, (long) zip);
		//trace(getstring);
		client.write((uint8_t*) getstring, strlen(getstring));
		ReturnVals vals = {0};

		ParseResponse(client, &vals);
		client.stop();
		//trace("%d, %d, %d, %d %d\n", vals.maxhumidity, vals.minhumidity, vals.meantempi, vals.precip_today, vals.precipi);

		const int humid_factor = 30 - (vals.maxhumidity + vals.minhumidity) / 2;
		const int temp_factor = (vals.meantempi - 70) * 4;
		const int rain_factor = (vals.precipi + vals.precip_today) * -2;
		const int adj = min(max(0, 100+humid_factor+temp_factor+rain_factor), 200);
		trace(F("Adjusting H(%d)T(%d)R(%d):%d\n"), humid_factor, temp_factor, rain_factor, adj);
		return adj;
	}
	else
	{
		trace(F("connection failed\n"));
		client.stop();
		return 100;
	}
}
