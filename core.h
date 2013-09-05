// core.h
// This file constitutes the core functions that run the scheduling for the Sprinkler system.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//  $Id: core.h 609 2013-06-28 00:25:31Z rzimmerman $
//

#ifndef _CORE_h
#define _CORE_h

#ifdef ARDUINO
#include "nntp.h"
#endif
#include <inttypes.h>
#include "port.h"
#ifdef LOGGING
#include "Logging.h"
extern Logging log;
#endif

#define VERSION "1.0.1"

void mainLoop();
void ClearEvents();
void LoadSchedTimeEvents(int8_t sched_num, bool bQuickSchedule = false);
void ReloadEvents(bool bAllEvents = false);
bool isZoneOn(int iNum);
void TurnOnZone(int iValve);
void TurnOffZones();
void io_setup();

class runStateClass
{
public:
	runStateClass();
	void SetSchedule(bool val, int8_t iSchedNum = -1);
	void ContinueSchedule(int8_t zone, short endTime);
	void SetManual(bool val, int8_t zone = -1);
	bool isSchedule()
	{
		return m_bSchedule;
	}
	bool isManual()
	{
		return m_bManual;
	}
	int8_t getZone()
	{
		return m_zone;
	}
	short getEndTime()
	{
		return m_endTime;
	}
private:
	void LogSchedule();
	bool m_bSchedule;
	bool m_bManual;
	int8_t m_iSchedule;
	int8_t m_zone;
	short m_endTime;
	time_t m_eventTime;
};

extern runStateClass runState;
extern nntp nntpTimeServer;

#endif

