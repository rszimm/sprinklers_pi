// Logging.h
// This manages the Logging to SQL for the Sprinkling System.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//  $Id: $
//

#ifndef LOGGING_H_
#define LOGGING_H_

#include "port.h"

class sqlite3;

class Logging
{
public:
	enum GROUPING {NONE, HOURLY, DAILY, MONTHLY};
	Logging();
	~Logging();
	bool Init();
	void Close();
	// Log an actual row to the database
	bool LogZoneEvent(time_t start, int zone, int duration, int schedule, int sadj, int wunderground);
	// Retrieve data sutible for graphing
	bool GraphZone(FILE * stream_file, time_t start, time_t end, GROUPING group);
	// Retrieve data suitble for putting into a table
	bool TableZone(FILE* stream_file, time_t start, time_t end);
private:
	sqlite3 *m_db;
};

#endif /* LOGGING_H_ */
