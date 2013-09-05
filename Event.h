// Event.h
// This defines the Event Object used for timing in the Sprinkling System.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//  $Id: Event.h 609 2013-06-28 00:25:31Z rzimmerman $
//

#ifndef _EVENT_h
#define _EVENT_h

#include <inttypes.h>
class Event
{
public:
	short time;
	uint8_t command;
	uint8_t data[3];
};

#define MAX_EVENTS 60

extern Event events[];
extern int iNumEvents;

#endif
