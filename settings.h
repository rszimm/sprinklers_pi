// settings.h
// This file manages the storage of data to a non volatile structure (e.g. EEPROM or filesystem)
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#ifndef _SETTINGS_h
#define _SETTINGS_h

// If you're looking for the user settings #defines they have been moved to config.h
#include "config.h"

// EEPROM Memory locations
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
#define LEN_PWS					11
#define ADDR_APIID				1023
#define LEN_APIID				32
#define ADDR_APISECRET			1055
#define LEN_APISECRET			64
#define ADDR_LOC				1119
#define LEN_LOC					50
#define ADDR_					1169

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

#include <inttypes.h>
#include <string.h>

#include "core.h"
#include "web.h"
#include "port.h"

class Schedule
{
private:
	uint8_t m_type;
public:
	union
	{
		uint8_t day;
		uint8_t interval;
	};
	char name[20];
	short time[4];
	uint8_t zone_duration[15];
	Schedule();
	bool IsEnabled() const { return m_type & 0x01; }
	bool IsInterval() const { return m_type & 0x02; }
	bool IsWAdj() const { return m_type & 0x04; }
	bool IsRestricted() const { return m_type & 0x08; }
	uint8_t GetRestriction() const {
		if (IsRestricted()) {
			if (m_type & 0x10) {
				return 2;
			} else {
				return 1;
			}
		}
		return 0;
	}
	bool IsRunToday(time_t time_now) {
		uint8_t restrictions = GetRestriction();
		if ((IsEnabled())	// do nothing if not enabled
			&& (((IsInterval()) && ((elapsedDays(time_now) % interval) == 0))	// if interval is enabled, check days since last run
				|| (!(IsInterval())
					&& ((restrictions == 0) || ((mday(time_now)%2) == (restrictions%2)))	// if even/odd restrictions are enabled check day of month number
					&& (day & (0x01 << (weekday(time_now) - 1))))))	// if day of week schedule check day of week
			return true;
		return false;
	}
	bool IsRunTomorrow(time_t time_now) {
		return IsRunToday(time_now+SECS_PER_DAY);
	}
	int NextRun(time_t time_now, char* str) {
		char scheduledTimes[100];

		if (GetEnabledTimes(scheduledTimes) == -1) {
			return sprintf(str, "n/a");
		}
		if (!IsRunToday(time_now) && !IsRunTomorrow(time_now)) {
			int days = 2;
			while (days <= 14 && !IsRunToday(time_now + (SECS_PER_DAY*days))) {
				days++;
			}
			if (days > 14) {
				return sprintf(str, "In 14+ days @ %s", scheduledTimes);
			}
			return sprintf(str, "In %d days @ %s", days, scheduledTimes);
		}

		return sprintf(str, "%s @ %s",
					   IsRunToday(time_now) ? "Today" : "Tomorrow",
					   scheduledTimes);
	}
	int GetEnabledTimes(char* str) {
		if (!IsEnabled()) {
			return -1;
		}
		char buff[10];
		int h;
		short x;
		bool enabled = false;
		str[0] = '\0';

		for (int i=0; i<4; i++) {
			x = time[i];
			if (x != -1) {
				if (enabled) {
					strcat(str, ", ");
				} else {
					enabled = true;
				}
				h = x/60;
				sprintf(buff, "%d:%.2d %s",
						(h%12 == 0 ? 12 : h%12),
						x%60,
						(h < 12 ? "AM" : "PM"));
				strcat(str, buff);
			}
		}
		if (!enabled) {
			return -1;
		}
		return 1;
	}
	void SetEnabled(bool val) { m_type = val ? (m_type | 0x01) : (m_type & ~0x01); }
	void SetInterval(bool val) { m_type = val ? (m_type | 0x02) : (m_type & ~0x02); }
	void SetWAdj(bool val) { m_type = val ? (m_type | 0x04) : (m_type & ~0x04); }
	void SetRestriction(uint8_t val) {
		if (val == 1) {
			m_type = (m_type | 0x08) & ~0x10;
		} else if (val == 2) {
			m_type = (m_type | 0x18);
		} else {
			m_type = (m_type & ~0x18);
		}
	}
};

struct FullZone
{
	bool bEnabled :1;
	bool bPump :1;
	char name[20];
};

struct ShortZone
{
	bool bEnabled :1;
	bool bPump :1;
};

////////////////////
//  EEPROM Getter/Setters
void SetNumSchedules(uint8_t iNum);
uint8_t GetNumSchedules();
void SetNTPOffset(const int8_t value);
int8_t GetNTPOffset();
IPAddress GetNTPIP();
void SetNTPIP(const IPAddress & value);
IPAddress GetIP();
void SetIP(const IPAddress & value);
IPAddress GetNetmask();
void SetNetmask(const IPAddress & value);
IPAddress GetGateway();
void SetGateway(const IPAddress & value);
IPAddress GetWUIP();
void SetWUIP(const IPAddress & value);
uint32_t GetZip();
void SetZip(const uint32_t zip);
void GetApiKey(char * key);
void SetApiKey(const char * key);
void GetApiId(char * key);
void SetApiId(const char * key);
void GetApiSecret(char * key);
void SetApiSecret(const char * key);
bool GetRunSchedules();
void SetRunSchedules(bool value);
bool GetDHCP();
void SetDHCP(const bool value);
enum EOT {OT_NONE, OT_DIRECT_POS, OT_DIRECT_NEG, OT_OPEN_SPRINKLER};
EOT GetOT();
void SetOT(EOT oType);
uint16_t GetWebPort();
void SetWebPort(uint16_t);
uint8_t GetSeasonalAdjust();
void SetSeasonalAdjust(uint8_t);
void GetPWS(char * key);
void SetPWS(const char * key);
void GetLoc(char * key);
void SetLoc(const char * key);
bool GetUsePWS();
void SetUsePWS(bool value);
void LoadSchedule(uint8_t num, Schedule * pSched);
void LoadZone(uint8_t num, FullZone * pZone);
void LoadShortZone(uint8_t index, ShortZone * pZone);

// KV Pairs Setters
bool SetSchedule(const KVPairs & key_value_pairs);
bool SetZones(const KVPairs & key_value_pairs);
bool DeleteSchedule(const KVPairs & key_value_pairs);
bool SetSettings(const KVPairs & key_value_pairs);

// Misc
bool IsFirstBoot();
void ResetEEPROM();
int GetNumEnabledZones();

// For storing info related to the Quick Schedule
extern Schedule quickSchedule;

#endif

