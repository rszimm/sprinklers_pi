// User configuration settings.
// Created by nhorvath on 3/24/19.
//

#ifndef SPRINKLERS_PI_CONFIG_H
#define SPRINKLERS_PI_CONFIG_H

// Uncomment to display times in 24H format
//#define CLOCK_24H 1

// Uncomment to use GreenIQ V2 board
//#define GREENIQ 1

// max number of schedules you will be allowed to create
#define MAX_SCHEDULES 10
#ifdef GREENIQ
// maximum number of zones allowed
#define NUM_ZONES 6
#else
// maximum number of zones allowed
#define NUM_ZONES 15
#endif

// Uncomment the next line if you want schedules to turn off when you use manual control
//#define DISABLE_SCHED_ON_MANUAL

/*************************************************
 * Weather Provider Section
 * Only uncomment one weather provider below.
 *************************************************/

// Open Weather https://openweathermap.org/darksky-openweather
//#define WEATHER_OPENWEATHER

// Aeris Weather https://www.aerisweather.com
//#define WEATHER_AERIS

// DarkSky Weather https://darksky.net/dev
// WARNING: this API will only work through the end of 2021
//#define WEATHER_DARKSKY

// Weather Underground
// IMPORTANT: Do not use. No longer working.
//#define WEATHER_WUNDERGROUND

// END WEATHER PROVIDER SECTION

// Set the on/off delay for "ChatterBox" (in microseconds)
// If your relay/solenoid isn't click-clacking try increasing this
#define CHATTERBOX_DELAY 100000
// Number of on/off cycles to execute per button press
#define CHATTERBOX_CYCLES 10

// External script to use when "None" Output Type is selected.
// If this script does not exist or is not executable, nothing will happen.
// Has no effect if ARDUINO is defined.
#define EXTERNAL_SCRIPT "/usr/local/bin/sprinklers_pi_zone"

#endif //SPRINKLERS_PI_CONFIG_H
