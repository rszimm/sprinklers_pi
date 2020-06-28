# sprinklers_pi
Sprinklers Pi is a sophisticated Irrigation control system for the Raspberry Pi.  The system contains a built in mobile web page server that can be used to configure and monitor the system status from anywhere.  It can also be cross compiled on an AVR/Arduino platform with appropriate networking and storage hardware.

## Major Features
* Fully contained system with control logic and web serving.
* Same code can be compiled to run on the Atmel/AVR/Arduino platform.
* Web based control (including mobile Android iOS)
* Automatic adjustment of intervals based on weather conditions. (weather underground API)
* Weather conditions can be pulled from individual personal weather stations or from general weather data based on zipcode.
* Manual Control
* Scheduled Control
* Quick Schedule
* Named Zones
* Full Graphing feature of historic logs
* Ability to run with OpenSprinkler module, direct relay outputs or an [external script](https://github.com/rszimm/sprinklers_pi/wiki/External-Zone-Control-Script).
* Supports master valve/pump output
* Supports expansion zone board (up to 15 zones)
* Very simple installation
* Seasonal adjustment.


## Weather Setup
By default, we now ship with no weather provider enabled, and therefore no adjustment performed.
Follow the directions below to enable a weather provider. If you change weather providers be sure to run "make clean"
before rebuilding.

### OpenWeather / OpenWeatherMap
This is the current recommended free provider, however, it is currently unsupported on Arduino/AVR Platforms due to a dependence on curl and a JSON library that is currently untested on Arduino. It works on Linux devices including raspberry pi.

1. Uncomment `#define WEATHER_OPENWEATHER` in config.h before building.
1. Build and start the server.
1. Navigate to the Settings Page.
1. Fill in API Secret with your api key from this page: https://home.openweathermap.org/api_keys
1. Fill in Location in the format: lattidue,longitude (eg. "40.749748,-73.991618")
1. Click "OK" at the top to save.
1. Navigate to the Advanced -> Weather Provider Diagnostics page to test everything is working.

### DarkSky Weather
WARNING: Darksky has been aquired by Apple and is shutting down their free API at the end of 2021. 

Uncomment `#define WEATHER_DARKSKY` in config.h before building.

DarkSky is currently unsupported on Arduino/AVR Platforms due to a dependence on curl and a JSON library that is currently untested on Arduino.

Once you have built and started the server fill out the API Secret Key in the Settings page (API Id is unused).
Your Secret can be found here: https://darksky.net/dev (account required, free).
Location must be (without the quotes):
  * GPS Latitude,Longitude (eg. "40.749748,-73.991618")

### Aeris Weather
Uncomment `#define WEATHER_AERIS` in config.h before building.

Once you have built and started the server fill out the API Id, API Secret, and Location in the Settings page.
Your API Id and Secret can be found here: https://www.aerisweather.com/account/apps (account required, free,
must be renewed every 2 months).
Location can be any of the following:
  * GPS Latitude,Longitude (eg. "40.749748,-73.991618")
  * City, State (eg. "New York, NY")
  * City, Country (eg. "London, United Kingdom")
  * ZIP Code (eg. 10001)

### Testing Weather Data
You can click on Advanced -> Weather Provider Diagnostics to verify your setup is working.

See the wiki for more information: https://github.com/rszimm/sprinklers_pi/wiki/Weather-adjustments


## Building
```Shell
make
sudo make install
```
NOTE: If you are running an older version of g++ compiler you may see the error `unrecognized command line option '-std=c++11'`. You should either update to g++ version 4.8+, or if you are not using DarkSky as a weather provider, you can remove `-std=c++11` at the end of line 8 in the Makefile. You can see what version of g++ you have by running `g++ --version`.

## Running
`sudo /etc/init.d/sprinklers_pi start`

See the wiki for more information: https://github.com/rszimm/sprinklers_pi/wiki

## Live Demo
A live demo site is available here: http://kilby.kewlshells.com:8080/
Please keep it clean.
