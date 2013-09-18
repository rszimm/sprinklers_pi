// sprinklers_pi.cpp
// Main entry point
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include "core.h"
#include "settings.h"
#include <unistd.h>
#include <signal.h>

bool bTermSignal = false;

void signal_callback_handler(int signum)
{
   printf("Caught signal %d\n",signum);
   bTermSignal = true;
}

int main(int argc, char **argv)
{
	// Register signal handlers
	signal(SIGTERM, signal_callback_handler);
	signal(SIGINT, signal_callback_handler);

	char * logfile = 0;
	int c = -1;
	while ((c = getopt(argc, argv, "?L:Vv")) != -1)
		switch (c)
		{
		case 'L':
			logfile = optarg;
			break;
		case 'V':
		case 'v':
			fprintf(stderr, "Version %s\n", VERSION);
			return 0;
			break;
        case '?':
          if (optopt == 'L')
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          else
            fprintf (stderr, "Usage: %s [ -L(LOGFILE) ]'.\n", argv[0]);
          return 1;
        default:
          return 1;
		}

	if (logfile)
	{
		if (freopen(logfile, "a", stdout) == 0)
		{
			trace("FILE OUTPUT to %s FAILED!\n", logfile);
			return 1;
		}
	}
	trace("Starting v%s..\n", VERSION);
	while (!bTermSignal)
	{
		mainLoop();
		usleep(1000);  // sleep for 1 ms
	}
	trace("Exiting.\n");
	return 0;
}

