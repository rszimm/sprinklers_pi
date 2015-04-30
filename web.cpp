// web.cpp
// This file manages the web server for the Sprinkler System
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include "web.h"
#include "settings.h"
#ifdef ARDUINO
#include "nntp.h"
#endif

#include "Weather.h"
#include "sysreset.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Event.h"

web::web(void)
		: m_server(0)
{
}

web::~web(void)
{
	if (m_server)
		delete m_server;
	m_server = 0;
}

bool web::Init()
{
	uint16_t port = GetWebPort();
	if ((port > 65000) || (port < 80))
		port = 80;
	trace(F("Listening on Port %u\n"), port), 
	m_server = new EthernetServer(port);
#ifdef ARDUINO
	m_server->begin();
	return true;
#else
	return m_server->begin();
#endif
}

static char sendbuf[512];

#ifdef ARDUINO
static char * sendbufptr;
static inline void setup_sendbuf()
{
	sendbufptr = sendbuf;
}

static int flush_sendbuf(EthernetClient & client)
{
	int ret = 0;
	if (sendbufptr > sendbuf)
	{
		ret = client.write((uint8_t*)sendbuf, sendbufptr-sendbuf);
		setup_sendbuf();
	}
	return ret;
}

static int stream_putchar(char c, FILE *stream)
{
	if (sendbufptr >= sendbuf + sizeof(sendbuf))
	{
		int send_len = ((EthernetClient*)(stream->udata))->write((uint8_t*)sendbuf, sizeof(sendbuf));
		if (!send_len)
		return 0;
		setup_sendbuf();
	}
	*(sendbufptr++) = c;
	return 1;
}
#endif

static void ServeHeader(FILE * stream_file, int code, const char * pReason, bool cache, const char * type = "text/html")
{
	fprintf_P(stream_file, PSTR("HTTP/1.1 %d %s\nContent-Type: %s\n"), code, pReason, type);
	if (cache)
		fprintf_P(stream_file, PSTR("Last-Modified: Fri, 02 Jun 2006 09:46:32 GMT\nExpires: Sun, 17 Jan 2038 19:14:07 GMT\n"));
	else
		fprintf_P(stream_file, PSTR("Cache-Control: no-cache\n"));
	fputc('\n', stream_file);
}

static void Serve404(FILE * stream_file)
{
	ServeHeader(stream_file, 404, "NOT FOUND", false);
	fprintf(stream_file, "NOT FOUND");
}

static void ServeError(FILE * stream_file)
{
	ServeHeader(stream_file, 405, "NOT ALLOWED", false);
	fprintf(stream_file, "NOT ALLOWED");
}

static void JSONSchedules(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, "OK", false, "text/plain");
	int iNumSchedules = GetNumSchedules();
	fprintf(stream_file, "{\n\"Table\" : [\n");
	Schedule sched;
	for (int i = 0; i < iNumSchedules; i++)
	{
		LoadSchedule(i, &sched);
		fprintf_P(stream_file, PSTR("%s\t{\"id\" : %d, \"name\" : \"%s\", \"e\" : \"%s\" }"), (i == 0) ? "" : ",\n", i, sched.name,
				(sched.IsEnabled()) ? "on" : "off");
	}
	fprintf(stream_file, "\n]}");
}

static void JSONZones(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, "OK", false, "text/plain");
	fprintf_P(stream_file, PSTR("{\n\"zones\" : [\n"));
	FullZone zone = {0};
	for (int i = 0; i < NUM_ZONES; i++)
	{
		LoadZone(i, &zone);
		fprintf_P(stream_file, PSTR("%s\t{\"name\" : \"%s\", \"enabled\" : \"%s\", \"pump\" : \"%s\", \"state\" : \"%s\" }"), (i == 0) ? "" : ",\n", zone.name,
				zone.bEnabled ? "on" : "off", zone.bPump ? "on" : "off", isZoneOn(i + 1) ? "on" : "off");
	}
	fprintf(stream_file, "\n]}");
}

#ifdef LOGGING
static void JSONLogs(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, "OK", false, "text/plain");
	fprintf(stream_file, "{\n");

	time_t sdate = 0;
	time_t edate = 0;
	Logging::GROUPING grouping = Logging::NONE;
	// Iterate through the kv pairs and search for the start and end dates.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp(key, "sdate") == 0)
		{
			sdate = strtol(value, 0, 10);
		}
		else if (strcmp(key, "edate") == 0)
		{
			edate = strtol(value, 0, 10);
		}
		else if (strcmp(key, "g") == 0)
		{
			if (value[0] == 'h')
				grouping = Logging::HOURLY;
			else if (value[0] == 'd')
				grouping = Logging::DAILY;
			else if (value[0] == 'm')
				grouping = Logging::MONTHLY;
		}
	}

	log.GraphZone(stream_file, sdate, edate, grouping);
	fprintf(stream_file, "}");
}

static void JSONtLogs(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, "OK", false, "text/plain");
	fprintf(stream_file, "{\n\t\"logs\": [\n");
	time_t sdate = 0;
	time_t edate = 0;
	// Iterate through the kv pairs and search for the start and end dates.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp(key, "sdate") == 0)
		{
			sdate = strtol(value, 0, 10);
		}
		else if (strcmp(key, "edate") == 0)
		{
			edate = strtol(value, 0, 10);
		}
	}
	log.TableZone(stream_file, sdate, edate);
	fprintf(stream_file, "\t]\n}");
}

#endif

static void JSONSettings(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, "OK", false, "text/plain");
	IPAddress ip;
	fprintf(stream_file, "{\n");
#ifdef ARDUINO
	ip = GetIP();
	fprintf_P(stream_file, PSTR("\t\"ip\" : \"%d.%d.%d.%d\",\n"), ip[0], ip[1], ip[2], ip[3]);
	ip = GetNetmask();
	fprintf_P(stream_file, PSTR("\t\"netmask\" : \"%d.%d.%d.%d\",\n"), ip[0], ip[1], ip[2], ip[3]);
	ip = GetGateway();
	fprintf_P(stream_file, PSTR("\t\"gateway\" : \"%d.%d.%d.%d\",\n"), ip[0], ip[1], ip[2], ip[3]);
	ip = GetNTPIP();
	fprintf_P(stream_file, PSTR("\t\"NTPip\" : \"%d.%d.%d.%d\",\n"), ip[0], ip[1], ip[2], ip[3]);
	fprintf_P(stream_file, PSTR("\t\"NTPoffset\" : \"%d\",\n"), GetNTPOffset());
#endif
	fprintf_P(stream_file, PSTR("\t\"webport\" : \"%u\",\n"), GetWebPort());
	fprintf_P(stream_file, PSTR("\t\"ot\" : \"%d\",\n"), GetOT());
	ip = GetWUIP();
	fprintf_P(stream_file, PSTR("\t\"wuip\" : \"%d.%d.%d.%d\",\n"), ip[0], ip[1], ip[2], ip[3]);
	fprintf_P(stream_file, PSTR("\t\"wutype\" : \"%s\",\n"), GetUsePWS() ? "pws" : "zip");
	fprintf_P(stream_file, PSTR("\t\"zip\" : \"%ld\",\n"), (long) GetZip());
	fprintf_P(stream_file, PSTR("\t\"sadj\" : \"%ld\",\n"), (long) GetSeasonalAdjust());
	char ak[17];
	GetApiKey(ak);
	fprintf_P(stream_file, PSTR("\t\"apikey\" : \"%s\",\n"), ak);
	GetPWS(ak);
	ak[11] = 0;
	fprintf_P(stream_file, PSTR("\t\"pws\" : \"%s\"\n"), ak);
	fprintf(stream_file, "}");
}

static void JSONwCheck(const KVPairs & key_value_pairs, FILE * stream_file)
{
	Weather w;
	ServeHeader(stream_file, 200, "OK", false, "text/plain");
	char key[17];
	GetApiKey(key);
	char pws[12] = {0};
	GetPWS(pws);
	const Weather::ReturnVals vals = w.GetVals(GetWUIP(), key, GetZip(), pws, GetUsePWS());
	const int scale = w.GetScale(vals);

	fprintf(stream_file, "{\n");
	fprintf_P(stream_file, PSTR("\t\"valid\" : \"%s\",\n"), vals.valid ? "true" : "false");
	fprintf_P(stream_file, PSTR("\t\"keynotfound\" : \"%s\",\n"), vals.keynotfound ? "true" : "false");
	fprintf_P(stream_file, PSTR("\t\"minhumidity\" : \"%d\",\n"), vals.minhumidity);
	fprintf_P(stream_file, PSTR("\t\"maxhumidity\" : \"%d\",\n"), vals.maxhumidity);
	fprintf_P(stream_file, PSTR("\t\"meantempi\" : \"%d\",\n"), vals.meantempi);
	fprintf_P(stream_file, PSTR("\t\"precip_today\" : \"%d\",\n"), vals.precip_today);
	fprintf_P(stream_file, PSTR("\t\"precip\" : \"%d\",\n"), vals.precipi);
	fprintf_P(stream_file, PSTR("\t\"wind_mph\" : \"%d\",\n"), vals.windmph);
	fprintf_P(stream_file, PSTR("\t\"UV\" : \"%d\",\n"), vals.UV);
	fprintf_P(stream_file, PSTR("\t\"scale\" : \"%d\"\n"), scale);
	fprintf(stream_file, "}");
}

static void JSONState(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, "OK", false, "text/plain");
	fprintf_P(stream_file,
			PSTR("{\n\t\"version\" : \"%s\",\n\t\"run\" : \"%s\",\n\t\"zones\" : \"%d\",\n\t\"schedules\" : \"%d\",\n\t\"timenow\" : \"%lu\",\n\t\"events\" : \"%d\""),
			VERSION, GetRunSchedules() ? "on" : "off", GetNumEnabledZones(), GetNumSchedules(), nntpTimeServer.LocalNow(), iNumEvents);
	if (runState.isSchedule() || runState.isManual())
	{
		FullZone zone;
		LoadZone(runState.getZone() - 1, &zone);
		long time_check = runState.getEndTime() * 60L - (nntpTimeServer.LocalNow() - previousMidnight(nntpTimeServer.LocalNow()));
		if (runState.isManual())
			time_check = 99999;
		fprintf_P(stream_file, PSTR(",\n\t\"onzone\" : \"%s\",\n\t\"offtime\" : \"%ld\""), zone.name, time_check);
	}
	fprintf_P(stream_file, (PSTR("\n}")));
}

static void JSONSchedule(const KVPairs & key_value_pairs, FILE * stream_file)
{
	int sched_num = -1;
	freeMemory();

	// Iterate through the kv pairs and search for the id.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp(key, "id") == 0)
		{
			sched_num = atoi(value);
		}
	}

	// Now check to see if the id is in range.
	const uint8_t numSched = GetNumSchedules();
	if ((sched_num < 0) || (sched_num >= numSched))
	{
		ServeError(stream_file);
		return;
	}

	// Now construct the response and send it
	ServeHeader(stream_file, 200, "OK", false, "text/plain");
	Schedule sched;
	LoadSchedule(sched_num, &sched);
	fprintf_P(stream_file,
			PSTR("{\n\t\"name\" : \"%s\",\n\t\"enabled\" : \"%s\",\n\t\"wadj\" : \"%s\",\n\t\"type\" : \"%s\",\n\t\"d1\" : \"%s\",\n\t\"d2\" : \"%s\""),
			sched.name, sched.IsEnabled() ? "on" : "off", sched.IsWAdj() ? "on" : "off", sched.IsInterval() ? "off" : "on", sched.day & 0x01 ? "on" : "off",
			sched.day & 0x02 ? "on" : "off");
	fprintf_P(stream_file,
			PSTR(",\n\t\"d3\" : \"%s\",\n\t\"d4\" : \"%s\",\n\t\"d5\" : \"%s\",\n\t\"d6\" : \"%s\",\n\t\"d7\" : \"%s\",\n\t\"interval\" : \"%d\",\n\t\"times\" : [\n"),
			sched.day & 0x04 ? "on" : "off", sched.day & 0x08 ? "on" : "off", sched.day & 0x10 ? "on" : "off", sched.day & 0x20 ? "on" : "off",
			sched.day & 0x40 ? "on" : "off", sched.interval);
	for (int i = 0; i < 4; i++)
	{
		if (sched.time[i] == -1)
		{
			fprintf_P(stream_file, PSTR("%s\t\t{\"t\" : \"00:00\", \"e\" : \"off\" }"), (i == 0) ? "" : ",\n");
		}
		else
		{
			fprintf_P(stream_file, PSTR("%s\t\t{\"t\" : \"%02d:%02d\", \"e\" : \"on\" }"), (i == 0) ? "" : ",\n", sched.time[i] / 60, sched.time[i] % 60);
		}
	}
	fprintf_P(stream_file, PSTR("\n\t],\n\t\"zones\" : [\n"));
	for (int i = 0; i < NUM_ZONES; i++)
	{
		FullZone zone;
		LoadZone(i, &zone);
		fprintf(stream_file, "%s\t\t{\"name\" : \"%s\", \"e\":\"%s\", \"duration\" : %d}", (i == 0) ? "" : ",\n", zone.name, zone.bEnabled ? "on" : "off",
				sched.zone_duration[i]);
	}
	fprintf(stream_file, " ]\n}");
}

static bool SetQSched(const KVPairs & key_value_pairs)
{

	// So, we first end any schedule that's currently running by turning things off then on again.
	ReloadEvents();

	int sched = -1;

	// Iterate through the kv pairs and update the appropriate structure values.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if ((key[0] == 'z') && (key[1] > 'a') && (key[1] <= ('a' + NUM_ZONES)) && (key[2] == 0))
		{
			quickSchedule.zone_duration[key[1] - 'b'] = atoi(value);
		}
		if (strcmp(key, "sched") == 0)
		{
			sched = atoi(value);
		}
	}

	if (sched == -1)
		LoadSchedTimeEvents(0, true);
	else
		LoadSchedTimeEvents(sched);
	return true;
}

static void ServeEventPage(FILE * stream_file)
{
	ServeHeader(stream_file, 200, "OK", false);
	freeMemory();
	const time_t timeNow = nntpTimeServer.LocalNow();
	fprintf_P(stream_file, PSTR("<h1>%d Events</h1><h3>%02d:%02d:%02d %d/%d/%d (%d)</h3>"), iNumEvents, hour(timeNow), minute(timeNow), second(timeNow),
			year(timeNow), month(timeNow), day(timeNow), weekday(timeNow));
	for (uint8_t i = 0; i < iNumEvents; i++)
		fprintf_P(stream_file, PSTR("Event [%02d] Time:%02d:%02d(%d) Command %d data %d,%d<br/>"), i, events[i].time / 60, events[i].time % 60, events[i].time,
				events[i].command, events[i].data[0], events[i].data[1]);
}

static void ServeSchedPage(FILE * stream_file)
{
	Schedule sched;
	freeMemory();
	ServeHeader(stream_file, 200, "OK", false);
	const uint8_t numSched = GetNumSchedules();
	for (uint8_t iSchedNum = 0; iSchedNum < numSched; iSchedNum++)
	{
		LoadSchedule(iSchedNum, &sched);
		fprintf_P(stream_file, PSTR("<hr/>Schedule #%d<br/>"), iSchedNum);
		if (sched.IsEnabled())
			fprintf_P(stream_file, PSTR("Enabled"));
		else
			fprintf_P(stream_file, PSTR("Not Enabled"));
		fprintf_P(stream_file, PSTR("<br/>Name:%s<br/>"), sched.name);
		if (sched.IsInterval())
			fprintf_P(stream_file, PSTR("Interval : %d"), sched.interval);
		else
		{
			fprintf_P(stream_file, PSTR("Day :"));
			if (sched.day & 0x01)
				fprintf(stream_file, "Su");
			if (sched.day & 0x02)
				fprintf(stream_file, "M");
			if (sched.day & 0x04)
				fprintf(stream_file, "T");
			if (sched.day & 0x08)
				fprintf(stream_file, "W");
			if (sched.day & 0x10)
				fprintf(stream_file, "R");
			if (sched.day & 0x20)
				fprintf(stream_file, "F");
			if (sched.day & 0x40)
				fprintf(stream_file, "Sa");
			fprintf_P(stream_file, PSTR("(%d)"), sched.day);
		}
		for (uint8_t i = 0; i < 4; i++)
			fprintf_P(stream_file, PSTR("<br/>Time %d:%02d:%02d(%d)"), i + 1, sched.time[i] / 60, sched.time[i] % 60, sched.time[i]);
		for (uint8_t i = 0; i < NUM_ZONES; i++)
			fprintf_P(stream_file, PSTR("<br/>Zone %d Duration:%d"), i + 1, sched.zone_duration[i]);
	}
}

static void ServeZonesPage(FILE * stream_file)
{
	FullZone zone;
	ServeHeader(stream_file, 200, "OK", false);
	for (uint8_t iZoneNum = 0; iZoneNum < NUM_ZONES; iZoneNum++)
	{
		LoadZone(iZoneNum, &zone);
		fprintf_P(stream_file, PSTR("<hr/>Zone #%d<br/>"), iZoneNum);
		if (zone.bEnabled)
			fprintf_P(stream_file, PSTR("Enabled"));
		else
			fprintf_P(stream_file, PSTR("Not Enabled"));
		fprintf_P(stream_file, PSTR("<br/>Name:%s<br/>"), zone.name);
		if (zone.bPump)
			fprintf_P(stream_file, PSTR("Pump ON"));
		else
			fprintf_P(stream_file, PSTR("Pump OFF"));
	}
}

static bool RunSchedules(const KVPairs & key_value_pairs)
{
	// Iterate through the kv pairs and update the appropriate structure values.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp(key, "system") == 0)
		{
			SetRunSchedules(strcmp(value, "on") == 0);
		}
	}
	return true;
}

static bool ManualZone(const KVPairs & key_value_pairs)
{
	freeMemory();

	// Turn off the current schedules.
	SetRunSchedules(false);

	bool bOn = false;
	int iZoneNum = -1;

	// Iterate through the kv pairs and update the appropriate structure values.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if ((strcmp(key, "zone") == 0) && (value[0] == 'z') && (value[1] > 'a') && (value[1] <= ('a' + NUM_ZONES)))
		{
			iZoneNum = value[1] - 'a';
		}
		else if (strcmp(key, "state") == 0)
		{
			if (strcmp(value, "on") == 0)
				bOn = true;
			else
				bOn = false;
		}
	}
	if ((iZoneNum >= 0) && bOn)
	{
		TurnOnZone(iZoneNum);
		runState.SetManual(true, iZoneNum);
	}
	else
	{
		TurnOffZones();
		runState.SetManual(false);
	}
	return true;
}

static void ServeFile(FILE * stream_file, const char * fname, SdFile & theFile, EthernetClient & client)
{
	freeMemory();
	const char * ext;
	for (ext=fname + strlen(fname); ext>fname; ext--)
		if (*ext == '.')
		{
			ext++;
			break;
		}
	if (ext > fname)
	{
		if (strcmp(ext, "jpg") == 0)
			ServeHeader(stream_file, 200, "OK", true, "image/jpeg");
		else if (strcmp(ext, "gif") == 0)
			ServeHeader(stream_file, 200, "OK", true, "image/gif");
		else if (strcmp(ext, "css") == 0)
			ServeHeader(stream_file, 200, "OK", true, "text/css");
		else if (strcmp(ext, "js") == 0)
			ServeHeader(stream_file, 200, "OK", true, "application/javascript");
		else if (strcmp(ext, "ico") == 0)
			ServeHeader(stream_file, 200, "OK", true, "image/x-icon");
		else
			ServeHeader(stream_file, 200, "OK", true);
	}
	else
		ServeHeader(stream_file, 200, "OK", true);


#ifdef ARDUINO
	flush_sendbuf(client);
#else
	fflush(stream_file);
#endif
	while (theFile.available())
	{
		int bytes = theFile.read(sendbuf, 512);
		if (bytes <= 0)
			break;
		client.write((uint8_t*) sendbuf, bytes);
	}
}

// change a character represented hex digit (0-9, a-f, A-F) to the numeric value
static inline char hex2int(const char ch)
{
	if (ch < 48)
		return 0;
	else if (ch <=57) // 0-9
		return (ch - 48);
	else if (ch < 65)
		return 0;
	else if (ch <=70) // A-F
		return (ch - 55);
	else if (ch < 97)
		return 0;
	else if (ch <=102) // a-f
		return ch - 87;
	else
		return 0;
}

//  Pass in a connected client, and this function will parse the HTTP header and return the requested page 
//   and a KV pairs structure for the variable assignments.
static bool ParseHTTPHeader(EthernetClient & client, KVPairs * key_value_pairs, char * sPage, int iPageSize)
{
	enum
	{
		INITIALIZED = 0, PARSING_PAGE, PARSING_KEY, PARSING_VALUE, PARSING_VALUE_PERCENT, PARSING_VALUE_PERCENT1, LOOKING_FOR_BLANKLINE, FOUND_BLANKLINE, DONE, ERROR
	} current_state = INITIALIZED;
	// an http request ends with a blank line
	static const char get_text[] = "GET /";
	const char * gettext_ptr = get_text;
	char * page_ptr = sPage;
	key_value_pairs->num_pairs = 0;
	char * key_ptr = key_value_pairs->keys[0];
	char * value_ptr = key_value_pairs->values[0];
	char recvbuf[4*1024];  // note:  trial and error has shown that it doesn't help to increase this number.. few ms at the most.
	char * recvbufptr = recvbuf;
	char * recvbufend = recvbuf;
	while (true)
	{
		if (recvbufptr >= recvbufend)
		{
			int len = client.read((uint8_t*) recvbuf, sizeof(recvbuf));
			if (len <= 0)
			{
				if (!client.connected())
					break;
				else
					continue;
			}
			else
			{
				recvbufptr = recvbuf;
				recvbufend = recvbuf + len;
			}
		}
		char c = *(recvbufptr++);
		//Serial.print(c);

		switch (current_state)
		{
		case INITIALIZED:
			if (c == *gettext_ptr)
			{
				gettext_ptr++;
				if (gettext_ptr - get_text >= (long)sizeof(get_text) - 1)
				{
					current_state = PARSING_PAGE;
				}
			}

			break;
		case PARSING_PAGE:
			if (c == '?')
			{
				*page_ptr = 0;
				current_state = PARSING_KEY;
			}
			else if (c == ' ')
			{
				*page_ptr = 0;
				current_state = LOOKING_FOR_BLANKLINE;
			}
			else if (c == '\n')
			{
				*page_ptr = 0;
				current_state = FOUND_BLANKLINE;
			}
			else if ((c > 32) && (c < 127))
			{
				if (page_ptr - sPage >= iPageSize - 1)
				{
					current_state = ERROR;
				}
				else
					*page_ptr++ = c;
			}
			break;
		case PARSING_KEY:
			if (c == ' ')
				current_state = LOOKING_FOR_BLANKLINE;
			else if (c == '\n')
			{
				current_state = FOUND_BLANKLINE;
			}
			else if (c == '&')
			{
				current_state = ERROR;
			}
			else if (c == '=')
			{
				*key_ptr = 0;
				current_state = PARSING_VALUE;
			}
			else if ((c > 32) && (c < 127))
			{
				if (key_ptr - key_value_pairs->keys[key_value_pairs->num_pairs] >= KEY_SIZE - 1)
				{
					current_state = ERROR;
				}
				else
					*key_ptr++ = c;
			}
			break;
		case PARSING_VALUE:
		case PARSING_VALUE_PERCENT:
		case PARSING_VALUE_PERCENT1:
			if ((c == ' ') || c == '&')
			{
				*value_ptr = 0;
				trace(F("Found a KV pair : %s -> %s\n"), key_value_pairs->keys[key_value_pairs->num_pairs], key_value_pairs->values[key_value_pairs->num_pairs]);

				if ((c == '&') && (key_value_pairs->num_pairs >= NUM_KEY_VALUES - 1))
				{
					current_state = ERROR;
					break;
				}
				if (c == '&')
				{
					key_value_pairs->num_pairs++;
					key_ptr = key_value_pairs->keys[key_value_pairs->num_pairs];
					value_ptr = key_value_pairs->values[key_value_pairs->num_pairs];
					current_state = PARSING_KEY;
				}
				else
				{
					key_value_pairs->num_pairs++;
					current_state = LOOKING_FOR_BLANKLINE;
				}
				break;
			}
			else if ((c > 32) && (c < 127))
			{
				if (value_ptr - key_value_pairs->values[key_value_pairs->num_pairs] >= VALUE_SIZE - 1)
				{
					current_state = ERROR;
					break;
				}
				switch (current_state)
				{
				case PARSING_VALUE_PERCENT:
					if (isxdigit(c))
					{
						*value_ptr = hex2int(c) << 4;
						current_state = PARSING_VALUE_PERCENT1;
					}
					else
						current_state = PARSING_VALUE;
					break;
				case PARSING_VALUE_PERCENT1:
					if (isxdigit(c))
					{
						*value_ptr += hex2int(c);
						// let's check this value to see if it's legal
						if (((*value_ptr >= 0 ) && (*value_ptr < 32)) || (*value_ptr == 127) || (*value_ptr == '"') || (*value_ptr == '\\'))
							*value_ptr = ' ';
						value_ptr++;
					}
					current_state = PARSING_VALUE;
					break;
				default:
					if (c == '+')
						*value_ptr++ = ' ';
					else if (c == '%')
						current_state = PARSING_VALUE_PERCENT;
					else
						*value_ptr++ = c;
					break;
				}
			}
			else
				current_state = ERROR;
			break;
		case LOOKING_FOR_BLANKLINE:
			if (c == '\n')
				current_state = FOUND_BLANKLINE;
			break;
		case FOUND_BLANKLINE:
			if (c == '\n')
				current_state = DONE;
			else if (c != '\r')
				current_state = LOOKING_FOR_BLANKLINE;
			break;
		default:
			break;
		} // switch
		if (current_state == DONE)
			return true;
		else if (current_state == ERROR)
			return false;
	} // true
	return false;
}

void web::ProcessWebClients()
{
	// listen for incoming clients
	EthernetClient client = m_server->available();
	if (client)
	{
		bool bReset = false;
#ifdef ARDUINO
		FILE stream_file;
		FILE * pFile = &stream_file;
		setup_sendbuf();
		fdev_setup_stream(pFile, stream_putchar, NULL, _FDEV_SETUP_WRITE);
		stream_file.udata = &client;
#else
		FILE * pFile = fdopen(client.GetSocket(), "w");
#endif
		freeMemory();
		trace(F("Got a client\n"));
		//ShowSockStatus();
		KVPairs key_value_pairs;
		char sPage[35];

		if (!ParseHTTPHeader(client, &key_value_pairs, sPage, sizeof(sPage)))
		{
			trace(F("ERROR!\n"));
			ServeError(pFile);
		}
		else
		{

			trace(F("Page:%s\n"), sPage);
			//ShowSockStatus();

			if (strcmp(sPage, "bin/setSched") == 0)
			{
				if (SetSchedule(key_value_pairs))
				{
					if (GetRunSchedules())
						ReloadEvents();
					ServeHeader(pFile, 200, "OK", false);
				}
				else
					ServeError(pFile);
			}
			else if (strcmp(sPage, "bin/setZones") == 0)
			{
				if (SetZones(key_value_pairs))
				{
					ReloadEvents();
					ServeHeader(pFile, 200, "OK", false);
				}
				else
					ServeError(pFile);
			}
			else if (strcmp(sPage, "bin/delSched") == 0)
			{
				if (DeleteSchedule(key_value_pairs))
				{
					if (GetRunSchedules())
						ReloadEvents();
					ServeHeader(pFile, 200, "OK", false);
				}
				else
					ServeError(pFile);
			}
			else if (strcmp(sPage, "bin/setQSched") == 0)
			{
				if (SetQSched(key_value_pairs))
				{
					ServeHeader(pFile, 200, "OK", false);
				}
				else
					ServeError(pFile);
			}
			else if (strcmp(sPage, "bin/settings") == 0)
			{
				if (SetSettings(key_value_pairs))
				{
					ReloadEvents();
					ServeHeader(pFile, 200, "OK", false);
				}
				else
					ServeError(pFile);
			}
			else if (strcmp(sPage, "bin/manual") == 0)
			{
				if (ManualZone(key_value_pairs))
				{
					ServeHeader(pFile, 200, "OK", false);
				}
				else
					ServeError(pFile);
			}
			else if (strcmp(sPage, "bin/run") == 0)
			{
				if (RunSchedules(key_value_pairs))
				{
					ReloadEvents();
					ServeHeader(pFile, 200, "OK", false);
				}
				else
					ServeError(pFile);
			}
			else if (strcmp(sPage, "bin/factory") == 0)
			{
				ResetEEPROM();
				ReloadEvents();
				ServeHeader(pFile, 200, "OK", false);
			}
			else if (strcmp(sPage, "bin/reset") == 0)
			{
				ServeHeader(pFile, 200, "OK", false);
				bReset = true;
			}
			else if (strcmp(sPage, "json/schedules") == 0)
			{
				JSONSchedules(key_value_pairs, pFile);
			}
			else if (strcmp(sPage, "json/zones") == 0)
			{
				JSONZones(key_value_pairs, pFile);
			}
			else if (strcmp(sPage, "json/settings") == 0)
			{
				JSONSettings(key_value_pairs, pFile);
			}
			else if (strcmp(sPage, "json/state") == 0)
			{
				JSONState(key_value_pairs, pFile);
			}
			else if (strcmp(sPage, "json/schedule") == 0)
			{
				JSONSchedule(key_value_pairs, pFile);
			}
			else if (strcmp(sPage, "json/wcheck") == 0)
			{
				JSONwCheck(key_value_pairs, pFile);
			}
#ifdef LOGGING
			else if (strcmp(sPage, "json/logs") == 0)
			{
				JSONLogs(key_value_pairs, pFile);
			}
			else if (strcmp(sPage, "json/tlogs") == 0)
			{
				JSONtLogs(key_value_pairs, pFile);
			}
#endif
			else if (strcmp(sPage, "ShowSched") == 0)
			{
				freeMemory();
				ServeSchedPage(pFile);
			}
			else if (strcmp(sPage, "ShowZones") == 0)
			{
				freeMemory();
				ServeZonesPage(pFile);
			}
			else if (strcmp(sPage, "ShowEvent") == 0)
			{
				ServeEventPage(pFile);
			}
			else if (strcmp(sPage, "ReloadEvent") == 0)
			{
				ReloadEvents(true);
				ServeEventPage(pFile);
			}
			else
			{
				if (strlen(sPage) == 0)
					strcpy(sPage, "index.htm");
				// prepend path
				memmove(sPage + 5, sPage, sizeof(sPage) - 5);
				memcpy(sPage, "/web/", 5);
				sPage[sizeof(sPage)-1] = 0;
				trace(F("Serving Page: %s\n"), sPage);
				SdFile theFile;
				if (!theFile.open(sPage, O_READ))
					Serve404(pFile);
				else
				{
					if (theFile.isFile())
						ServeFile(pFile, sPage, theFile, client);
					else
						Serve404(pFile);
					theFile.close();
				}
			}
		}

#ifdef ARDUINO
		flush_sendbuf(client);
		// give the web browser time to receive the data
		delay(1);
#else
		fflush(pFile);
		fclose(pFile);
#endif
		// close the connection:
		client.stop();

		if (bReset)
			sysreset();
	}
}

