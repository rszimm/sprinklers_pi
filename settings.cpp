// settings.cpp
// This file manages the storage of data to a non volatile structure (e.g. EEPROM or filesystem)
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//  $Id: settings.cpp 614 2013-08-27 22:31:13Z rzimmerman $
//

#include "settings.h"
#include "port.h"
#include <string.h>
#include <stdlib.h>

#define ADDR_SCHEDULE_COUNT		4
#define ADDR_OP1				5
#define END_OF_ZONE_BLOCK		950
#define END_OF_SCHEDULE_BLOCK	2048
#define ADDR_NTP_IP				950
#define ADDR_NTP_OFFSET			954
#define ADDR_HOST				955 // NOT USED
#define MAX_HOST_LEN			20  // NOT USED
#define ADDR_IP					976
#define ADDR_NETMASK			980
#define ADDR_GATEWAY			984
#define ADDR_DHCP				988 // NOT USED
#define ADDR_WUIP				992
#define ADDR_ZIP				996
#define ADDR_APIKEY				1000
#define ADDR_OTYPE				1008
#define ADDR_WEB				1009
#define ADDR_SADJ				1011
#define ADDR_PWS				1012
#define ADDR_					1023

#define SCHEDULE_OFFSET 1200
#define SCHEDULE_INDEX 60
#define ZONE_OFFSET 20
#define ZONE_INDEX 25

#if ZONE_OFFSET + (ZONE_INDEX * NUM_ZONES) > END_OF_ZONE_BLOCK
#error Number of Zones is too large
#endif

#if SCHEDULE_OFFSET + (SCHEDULE_INDEX * MAX_SCHEDULES) > END_OF_SCHEDULE_BLOCK
#error Number of Schedules is too large
#endif

Schedule::Schedule() : m_type(0), day(0)
{
	name[0] = 0;
	for (uint8_t i=0; i<sizeof(time)/sizeof(time[0]); i++)
		time[i] = -1;
	for (uint8_t i=0; i<sizeof(zone_duration)/sizeof(zone_duration[0]); i++)
		zone_duration[i] = 0;
}

void LoadSchedule(uint8_t num, Schedule * pSched)
{
	if (num < 0 || num >= MAX_SCHEDULES)
		return;
	for (uint8_t i = 0; i < sizeof(Schedule); ++i)
	{
		*(((char*) pSched) + i) = EEPROM.read(SCHEDULE_OFFSET + i + SCHEDULE_INDEX * num);
	}
}

void SaveSchedule(uint8_t num, const Schedule * pSched)
{
	if (num < 0 || num >= MAX_SCHEDULES)
		return;
	for (uint8_t i = 0; i < sizeof(Schedule); i++)
		EEPROM.write(SCHEDULE_OFFSET + i + SCHEDULE_INDEX * num, *((char*) pSched + i));
}

void LoadZone(uint8_t num, FullZone * pZone)
{
	if (num < 0 || num >= NUM_ZONES)
		return;
	for (uint8_t i = 0; i < sizeof(FullZone); i++)
		*((char*) pZone + i) = EEPROM.read(ZONE_OFFSET + i + ZONE_INDEX * num);
}

void SaveZone(uint8_t num, const FullZone * pZone)
{
	if (num < 0 || num >= NUM_ZONES)
		return;
	for (uint8_t i = 0; i < sizeof(FullZone); i++)
		EEPROM.write(ZONE_OFFSET + i + ZONE_INDEX * num, *((char*) pZone + i));
}

void LoadShortZone(uint8_t num, ShortZone * pZone)
{
	if (num < 0 || num >= NUM_ZONES)
		return;
	for (uint8_t i = 0; i < sizeof(ShortZone); i++)
		*((char*) pZone + i) = EEPROM.read(ZONE_OFFSET + i + ZONE_INDEX * num);
}

// Decode an IP address in dotted decimal format.
static IPAddress decodeIP(const char * value)
{
	uint8_t ip[4];
	const char * pEnd = value;
	int i = 0;
	while (i < 4)
	{
		ip[i++] = strtoul(pEnd, (char**) &pEnd, 10);
		if (!pEnd || (*pEnd++ != '.'))
			break;
	}
	if (i == 4)
		return IPAddress(ip[0], ip[1], ip[2], ip[3]);
	else
		return INADDR_NONE;
}

//************************************
// Method:    SetSchedule
// FullName:  SetSchedule
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const KVPairs & key_value_pairs
//************************************
bool SetSchedule(const KVPairs & key_value_pairs)
{
	freeMemory();
	Schedule sched;
	int sched_num = -1;
	sched.day = 0;
	sched.time[0] = -1;
	sched.time[1] = -1;
	sched.time[2] = -1;
	sched.time[3] = -1;
	bool time_enable[4] = {0};

	// Iterate through the kv pairs and update the appropriate structure values.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp(key, "id") == 0)
		{
			sched_num = atoi(value);
		}
		else if (strcmp(key, "type") == 0)
			sched.SetInterval(strcmp(value, "on") != 0);
		else if (strcmp(key, "enable") == 0)
			sched.SetEnabled(strcmp(value, "on") == 0);
		else if (strcmp(key, "wadj") == 0)
			sched.SetWAdj(strcmp(value, "on") == 0);
		else if (strcmp(key, "name") == 0)
			strncpy(sched.name, value, sizeof(sched.name));
		else if (strcmp(key, "interval") == 0)
		{
			if (sched.IsInterval())
				sched.interval = atoi(value);
		}
		else if ((key[0] == 'd') && (key[2] == 0) && ((key[1] >= '1') && (key[1] <= '7')) && !(sched.IsInterval()))
		{
			if (strcmp(value, "on") == 0)
				sched.day = sched.day | 0x01 << (key[1] - '1');
			else
				sched.day = sched.day & ~(0x01 << (key[1] - '1'));
		}
		else if ((key[0] == 't') && (key[2] == 0) && ((key[1] >= '1') && (key[1] <= '4')))
		{
			const char * colon_loc = strstr(value, "%3A");
			if (colon_loc > 0)
			{
				int hour = strtol(value, NULL, 10);
				int minute = strtol(colon_loc + 3, NULL, 10);
				bool bIsPM = strstr(value, "PM") || strstr(value, "pm");
				if (bIsPM)
					hour += 12;
				if ((hour >= 24) || (hour < 0) || (minute >= 60) || (minute < 0))
				{
					trace(F("Invalid Date Input\n"));
					return false;
				}
				sched.time[key[1] - '1'] = hour * 60 + minute;
			}
		}
		else if ((key[0] == 'e') && (key[2] == 0) && ((key[1] >= '1') && (key[1] <= '4')))
		{
			if (strcmp(value, "on") == 0)
				time_enable[key[1] - '1'] = true;
			else
				time_enable[key[1] - '1'] = false;
		}
		else if ((key[0] == 'z') && (key[2] == 0) && ((key[1] >= 'b') && (key[1] <= ('a' + NUM_ZONES))))
		{
			sched.zone_duration[key[1] - 'b'] = atoi(value);
		}
	}

	// cycle through the time enable bits and set our special code for disabled times:
	for (int i = 0; i < 4; i++)
	{
		if (!time_enable[i])
			sched.time[i] = -1;
	}

	// Now let's determine what schedule index we are dumping this into.
	int iNumSchedules = GetNumSchedules();
	if (sched_num == -1)
	{
		// Check to see if we've exceeded the number of schedules.
		if (iNumSchedules == MAX_SCHEDULES )
		{
			trace(F("Too Many Schedules\n"));
			return false;
		}
		sched_num = iNumSchedules++;
		SetNumSchedules(iNumSchedules);
	}

	// check to see if we've got a valid schedule number
	if ((sched_num < 0) || (sched_num >= iNumSchedules))
	{
		trace(F("Invalid Schedule Number :%d\n"), sched_num);
		return false;
	}
	// and save it
	SaveSchedule(sched_num, &sched);
	return true;
}

bool DeleteSchedule(const KVPairs & key_value_pairs)
{
	int sched_num = -1;
	// Iterate through the kv pairs and update the appropriate structure values.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp(key, "id") == 0)
		{
			sched_num = atoi(value);
		}
	}

	// Now let's determine what schedule index we are deleting this into.
	const int iNumSchedules = GetNumSchedules();

	// see if we're in the proper range
	if ((sched_num < 0) || (sched_num >= iNumSchedules))
		return false;

	Schedule sched;
	for (int i = sched_num; i < (iNumSchedules - 1); i++)
	{
		LoadSchedule(i + 1, &sched);
		SaveSchedule(i, &sched);
	}
	SetNumSchedules(iNumSchedules - 1);
	return true;
}

bool SetZones(const KVPairs & key_value_pairs)
{
	FullZone zones[NUM_ZONES] = {0};

	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if ((key[0] == 'z') && (key[1] >= 'b') && (key[1] <= ('a' + NUM_ZONES)))
		{
			int zone_num = key[1] - 'b';
			if (memcmp(key + 2, "name", 5) == 0)
				strncpy(zones[zone_num].name, value, sizeof(zones[zone_num].name));
			else if ((key[2] == 'e') && (key[3] == 0))
			{
				if (strcmp(value, "on") == 0)
					zones[zone_num].bEnabled = true;
				else
					zones[zone_num].bEnabled = false;
			}
			else if ((key[2] == 'p') && (key[3] == 0))
			{
				if (strcmp(value, "on") == 0)
					zones[zone_num].bPump = true;
				else
					zones[zone_num].bPump = false;
			}
		}
	}
	for (int i = 0; i < NUM_ZONES; i++)
		SaveZone(i, &zones[i]);
	return true;
}

bool SetSettings(const KVPairs & key_value_pairs)
{
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp(key, "ip") == 0)
		{
			SetIP(decodeIP(value));
		}
		else if (strcmp(key, "netmask") == 0)
		{
			SetNetmask(decodeIP(value));
		}
		else if (strcmp(key, "gateway") == 0)
		{
			SetGateway(decodeIP(value));
		}
		else if (strcmp(key, "wuip") == 0)
		{
			SetWUIP(decodeIP(value));
		}
		else if (strcmp(key, "apikey") == 0)
		{
			SetApiKey(value);
		}
		else if (strcmp(key, "zip") == 0)
		{
			SetZip(strtoul(value, 0, 10));
		}
		else if (strcmp(key, "NTPip") == 0)
		{
			SetNTPIP(decodeIP(value));
		}
		else if (strcmp(key, "NTPoffset") == 0)
		{
			SetNTPOffset(atoi(value));
		}
		else if (strcmp(key, "ot") == 0)
		{
			SetOT((EOT)atoi(value));
		}
		else if (strcmp(key, "webport") == 0)
		{
			SetWebPort(atoi(value));
		}
		else if (strcmp(key, "sadj") == 0)
		{
			SetSeasonalAdjust(atoi(value));
		}
		else if (strcmp(key, "pws") == 0)
		{
			SetPWS(value);
		}
		else if (strcmp(key, "wutype") == 0)
		{
			SetUsePWS(strcmp(value, "pws") == 0);
		}

	}
	return true;
}

static const char * const sHeader = "S1.2";
void ResetEEPROM()
{
	trace(F("Reseting EEPROM\n"));
	for (int i = 0; i <= 3; i++)
		EEPROM.write(i, sHeader[i]);
	SetNumSchedules(0);
	FullZone zone = {0};
	for (int i = 0; i < NUM_ZONES; i++)
	{
		zone.bEnabled = (i == 0) ? 0x01 : 0x00;
		zone.bPump = true;
		sprintf(zone.name, "Zone %d", i + 1);
		SaveZone(i, &zone);
	}
	SetNTPOffset(0);
	SetNTPIP(INADDR_NONE);
	SetWUIP(INADDR_NONE);
	SetApiKey("");
	SetZip(0);
	SetIP(IPAddress(192, 168, 10, 20));
	SetNetmask(IPAddress(255, 255, 255, 0));
	SetGateway(IPAddress(192, 168, 10, 1));
	SetRunSchedules(false);
	SetWebPort(8080);
	SetSeasonalAdjust(100);
	SetPWS("");
	SetUsePWS(false);
	SetOT(OT_NONE);
}

void SetNumSchedules(const uint8_t iNum)
{
	EEPROM.write(ADDR_SCHEDULE_COUNT, iNum);
}

uint8_t GetNumSchedules()
{
	return EEPROM.read(ADDR_SCHEDULE_COUNT);
}

void SetNTPOffset(const int8_t value)
{
	EEPROM.write(ADDR_NTP_OFFSET, value);
}

int8_t GetNTPOffset()
{
	return EEPROM.read(ADDR_NTP_OFFSET);
}

IPAddress GetNTPIP()
{
	return IPAddress(EEPROM.read(ADDR_NTP_IP), EEPROM.read(ADDR_NTP_IP + 1), EEPROM.read(ADDR_NTP_IP + 2), EEPROM.read(ADDR_NTP_IP + 3));
}

void SetNTPIP(const IPAddress & value)
{
	for (int i = 0; i < 4; i++)
		EEPROM.write(ADDR_NTP_IP + i, value[i]);
}

IPAddress GetIP()
{
	return IPAddress(EEPROM.read(ADDR_IP), EEPROM.read(ADDR_IP + 1), EEPROM.read(ADDR_IP + 2), EEPROM.read(ADDR_IP + 3));
}

void SetIP(const IPAddress & value)
{
	for (int i = 0; i < 4; i++)
		EEPROM.write(ADDR_IP + i, value[i]);
}

IPAddress GetNetmask()
{
	return IPAddress(EEPROM.read(ADDR_NETMASK), EEPROM.read(ADDR_NETMASK + 1), EEPROM.read(ADDR_NETMASK + 2), EEPROM.read(ADDR_NETMASK + 3));
}

void SetNetmask(const IPAddress & value)
{
	for (int i = 0; i < 4; i++)
		EEPROM.write(ADDR_NETMASK + i, value[i]);
}

IPAddress GetGateway()
{
	return IPAddress(EEPROM.read(ADDR_GATEWAY), EEPROM.read(ADDR_GATEWAY + 1), EEPROM.read(ADDR_GATEWAY + 2), EEPROM.read(ADDR_GATEWAY + 3));
}

void SetGateway(const IPAddress & value)
{
	for (int i = 0; i < 4; i++)
		EEPROM.write(ADDR_GATEWAY + i, value[i]);
}

IPAddress GetWUIP()
{
	return IPAddress(EEPROM.read(ADDR_WUIP), EEPROM.read(ADDR_WUIP + 1), EEPROM.read(ADDR_WUIP + 2), EEPROM.read(ADDR_WUIP + 3));
}

void SetWUIP(const IPAddress & value)
{
	for (int i = 0; i < 4; i++)
		EEPROM.write(ADDR_WUIP + i, value[i]);
}

uint32_t GetZip()
{
	return (uint32_t) EEPROM.read(ADDR_ZIP) << 24 | (uint32_t) EEPROM.read(ADDR_ZIP + 1) << 16 | (uint32_t) EEPROM.read(ADDR_ZIP + 2) << 8
			| (uint32_t) EEPROM.read(ADDR_ZIP + 3);
}

void SetZip(const uint32_t zip)
{
	for (int i = 0; i < 4; i++)
		EEPROM.write(ADDR_ZIP + i, zip >> (8 * (3 - i)));
}

void GetPWS(char * key)
{
	for (int i=0; i<11; i++)
		key[i] = EEPROM.read(ADDR_PWS+i);
}

void SetPWS(const char * key)
{
	for (int i=0; i<11; i++)
		EEPROM.write(ADDR_PWS+i, key[i]);
}

void GetApiKey(char * key)
{
	sprintf(key, "%02x%02x%02x%02x%02x%02x%02x%02x", EEPROM.read(ADDR_APIKEY), EEPROM.read(ADDR_APIKEY + 1), EEPROM.read(ADDR_APIKEY + 2),
			EEPROM.read(ADDR_APIKEY + 3), EEPROM.read(ADDR_APIKEY + 4), EEPROM.read(ADDR_APIKEY + 5), EEPROM.read(ADDR_APIKEY + 6),
			EEPROM.read(ADDR_APIKEY + 7));
}

static uint8_t toHex(char val)
{
	if ((val >= '0') && (val <= '9'))
		return val - '0';
	else if ((val >= 'A') && (val <= 'F'))
		return val - 'A' + 10;
	else if ((val >= 'a') && (val <= 'f'))
		return val - 'a' + 10;
	else
		return 0;
}

void SetApiKey(const char * key)
{
	if (strlen(key) != 16)
	{
		for (int i = 0; i < 8; i++)
			EEPROM.write(ADDR_APIKEY + i, 0);
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			EEPROM.write(ADDR_APIKEY + i, (toHex(key[i * 2]) << 4) | toHex(key[i * 2 + 1]));
		}
	}
}

bool GetRunSchedules()
{
	return EEPROM.read(ADDR_OP1) & 0x01;
}

void SetRunSchedules(bool value)
{
	uint8_t current = EEPROM.read(ADDR_OP1);
	if (value)
		EEPROM.write(ADDR_OP1, current | 0x01);
	else
		EEPROM.write(ADDR_OP1, current & ~0x01);
}

bool GetUsePWS()
{
	return EEPROM.read(ADDR_OP1) & 0x02;
}

void SetUsePWS(bool value)
{
	uint8_t current = EEPROM.read(ADDR_OP1);
	if (value)
		EEPROM.write(ADDR_OP1, current | 0x02);
	else
		EEPROM.write(ADDR_OP1, current & ~0x02);
}

bool GetDHCP()
{
	return EEPROM.read(ADDR_DHCP);
}

void SetDHCP(const bool value)
{
	EEPROM.write(ADDR_DHCP, value);
}

EOT GetOT()
{
	return (EOT)EEPROM.read(ADDR_OTYPE);
}

void SetOT(EOT oType)
{
	// if things have changed make sure we re-run the io_setup routine.
	if (GetOT() != oType)
	{
		EEPROM.write(ADDR_OTYPE, oType);
		io_setup();
	}
}

uint16_t GetWebPort()
{
	return EEPROM.read(ADDR_WEB)<<8 | EEPROM.read(ADDR_WEB+1);
}

void SetWebPort(uint16_t port)
{
	EEPROM.write(ADDR_WEB, port>>8);
	EEPROM.write(ADDR_WEB+1, port&0x00FF);
}

uint8_t GetSeasonalAdjust()
{
	return EEPROM.read(ADDR_SADJ);
}

void SetSeasonalAdjust(uint8_t val)
{
	EEPROM.write(ADDR_SADJ, min(val, 200));
}

bool IsFirstBoot()
{
	if ((SCHEDULE_INDEX < sizeof(Schedule)) || (ZONE_INDEX < sizeof(FullZone)))
	{
		trace (F("Size mismatch."));
		exit(1);
	}

	if ((EEPROM.read(0) == sHeader[0]) && (EEPROM.read(1) == sHeader[1]) && (EEPROM.read(2) == sHeader[2]) && (EEPROM.read(3) == sHeader[3]))
		return false;
	return true;
}

int GetNumZones()
{
	return NUM_ZONES;
}

Schedule quickSchedule;

