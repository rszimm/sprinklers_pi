// Aeris.cpp
// This file manages the retrieval of Weather related information and adjustment of durations
//   from Aeris Weather
// Author: Nick Horvath
//

#include "Aeris.h"
#include "core.h"
#include "port.h"
#include <string.h>
#include <stdlib.h>
#include <ctime>

Aeris::Aeris(void)
{
	m_aerisAPIHost="api.aerisapi.com";
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
	char key[30], parentKey[30], val[30];
	char * keyptr = key;
	char * valptr = val;
	while (true)
	{
		if (recvbufptr >= recvbufend)
		{
			int len = client.read((uint8_t*) recvbuf, sizeof(recvbuf));
			//trace(F("RCV: %s\n"), recvbuf);
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
					//trace("Found Key: %s\n", key);
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
					//trace("Found new object\n");
					strcpy(parentKey, key);
					current_state = FIND_QUOTE1;
				}
				else if (((c >= '0') && (c <= '9')) || c == '-')
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

					if (strcmp(key, "avgF") == 0) {
						if (strcmp(parentKey, "temp") == 0) {
							ret->valid = true;
							ret->keynotfound = false;
							ret->meantempi = atoi(val);
							trace("Avg Temp Yesterday: %d\n", ret->meantempi);
						}
					} else if (strcmp(key, "max") == 0) {
						if (strcmp(parentKey, "rh") == 0) {
							ret->valid = true;
							ret->keynotfound = false;
							ret->maxhumidity = atoi(val);
							// prevent invalid humidity from getting through
							if (ret->maxhumidity > 100 || ret->maxhumidity < 0) {
								ret->maxhumidity = NEUTRAL_HUMIDITY;
							}
							trace("Humidity Max Yesterday: %d\n", ret->maxhumidity);
						}
					} else if (strcmp(key, "min") == 0) {
						if (strcmp(parentKey, "rh") == 0) {
							ret->valid = true;
							ret->keynotfound = false;
							ret->minhumidity = atoi(val);
							// prevent invalid humidity from getting through
							if (ret->minhumidity > 100 || ret->minhumidity < 0) {
								ret->minhumidity = NEUTRAL_HUMIDITY;
							}
							trace("Humidity Min Yesterday: %d\n", ret->minhumidity);
						}
					} else if (strcmp(key, "totalIN") == 0) {
						if (strcmp(parentKey, "precip") == 0) {
							ret->valid = true;
							ret->keynotfound = false;
							ret->precipi = (atof(val) * 100.0);
							trace("Precip Yesterday: %d\n", ret->precipi);
						}
					} else if (strcmp(key, "avgMPH") == 0) {
						if (strcmp(parentKey, "wind") == 0) {
							ret->valid = true;
							ret->keynotfound = false;
							ret->windmph = (atof(val) * 10.0);
							trace("Wind Yesterday: %d\n", ret->windmph);
						}
					} else if (strcmp(key, "precipIN") == 0) {
						ret->valid = true;
						ret->keynotfound = false;
						ret->precip_today = (atof(val) * 100.0);
						trace("Precip Today: %d\n", ret->precip_today);
					} else if (strcmp(key, "uvi") == 0) {
						ret->valid = true;
						ret->keynotfound = false;
						ret->UV = (atof(val) * 10.0);
						trace("UV Today: %d\n", ret->UV);
					}
				}
				break;
			case PARSING_QVALUE:
				if (c == '"')
				{
					current_state = FIND_QUOTE1;
					*valptr = 0;
					//trace("%s:%s\n", key, val);
					if (strcmp(key, "type") == 0) {
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

Weather::ReturnVals Aeris::GetVals(void) const
{
	Settings settings = GetSettings();
	return GetVals(settings);
}

Weather::ReturnVals Aeris::GetVals(const Weather::Settings & settings) const
{
	ReturnVals vals = {0};
	EthernetClient client;

	time_t rawtime;
	struct tm * timeinfo;
	char yesterday[12];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	timeinfo->tm_mday--;
	// convert again to fix month wrapping
	rawtime = mktime(timeinfo);
	timeinfo = localtime(&rawtime);

	strftime(yesterday,sizeof(yesterday),"%m/%d/%Y", timeinfo);

	trace("Yesterday: %s\n", yesterday);

	if (client.connect(m_aerisAPIHost, 80))
	{
		char getstring[512];
		trace(F("Connected\n"));

		snprintf(getstring, sizeof(getstring), "GET https://%s/observations/summary/%s?&format=json&from=%s&filter=allstations&limit=1&client_id=%s&client_secret=%s HTTP/1.1\r\n",
				 m_aerisAPIHost,
				 settings.location,
				 yesterday,
				 settings.apiId, settings.apiSecret);

		//trace("GetString: %s\n",getstring);
		client.write((uint8_t*) getstring, strlen(getstring));

		//send host header
		snprintf(getstring, sizeof(getstring), "Host: %s\r\nConnection: close\r\n\r\n",m_aerisAPIHost);
		//trace("GetString: %s\n",getstring);
		client.write((uint8_t*) getstring, strlen(getstring));

		ParseResponse(client, &vals);
		vals.resolvedIP=client.GetIpAddress();

		client.stop();
		if (!vals.valid)
		{
			if (vals.keynotfound)
				trace("Invalid Aeris Key\n");
			else
				trace("Bad Aeris Response\n");
		}
	}
	else
	{
		trace(F("connection failed\n"));
		client.stop();
	}


	if (client.connect(m_aerisAPIHost, 80))
	{
		char getstring[512];
		trace(F("Connected\n"));

		snprintf(getstring, sizeof(getstring), "GET https://%s/forecasts/%s?&format=json&filter=day&limit=1&client_id=%s&client_secret=%s HTTP/1.1\r\n",
				 m_aerisAPIHost,
				 settings.location,
				 settings.apiId, settings.apiSecret);

		//trace("GetString: %s\n",getstring);
		client.write((uint8_t*) getstring, strlen(getstring));

		//send host header
		snprintf(getstring, sizeof(getstring), "Host: %s\r\nConnection: close\r\n\r\n",m_aerisAPIHost);
		//trace("GetString: %s\n",getstring);
		client.write((uint8_t*) getstring, strlen(getstring));

		ParseResponse(client, &vals);
		vals.resolvedIP=client.GetIpAddress();

		client.stop();
		if (!vals.valid)
		{
			if (vals.keynotfound)
				trace("Invalid Aeris Key\n");
			else
				trace("Bad Aeris Response\n");
		}
	}
	else
	{
		trace(F("connection failed\n"));
		client.stop();
	}

	return vals;
}
