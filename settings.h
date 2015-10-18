// settings.h
// This file manages the storage of data to a non volatile structure (e.g. EEPROM or filesystem)
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#ifndef _SETTINGS_h
#define _SETTINGS_h
#define MAX_SCHEDULES 10
#define NUM_ZONES 8
#include <inttypes.h>

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
	void SetEnabled(bool val) { m_type = val ? (m_type | 0x01) : (m_type & ~0x01); }
	void SetInterval(bool val) { m_type = val ? (m_type | 0x02) : (m_type & ~0x02); }
	void SetWAdj(bool val) { m_type = val ? (m_type | 0x04) : (m_type & ~0x04); }
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

