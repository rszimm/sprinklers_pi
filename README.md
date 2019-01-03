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
* Ability to run with OpenSprinkler module or direct relay outputs.
* Supports master valve/pump output
* Supports expansion zone board (up to 15 zones)
* Very simple installation
* Seasonal adjustment.


## Weather Setup
By default, we now ship with no weather provider enabled, and therefore no adjustment performed.
Follow the directions below to enable a weather provider.

### Aeris Weather
Uncomment `#define WEATHER_AERIS` in settings.h before building.

Once you have built and started the server fill out the API Id, API Secret, and Location in the Settings page.
Your API Id and Secret can be found here: https://www.aerisweather.com/account/apps (account required, free).
Location can be any of the following:
  * GPS Latitude,Longitude (eg. "40.749748,-73.991618")
  * City, State (eg. "New York, NY")
  * City, Country (eg. "London, United Kingdom")
  * ZIP Code (eg. 10001)

### Wunderground
Wunderground setup requires you to have previously created an API Key, and for them to have not shut it down yet.

Uncomment `#define WEATHER_WUNDERGROUND` in settings.h before building.

### Testing Weather Data
You can click on Advanced -> Weather Provider Diagnostics to verify your setup is working.

See the wiki for more information: https://github.com/rszimm/sprinklers_pi/wiki/Weather-adjustments


## Building
```Shell
make -lwiringPi
sudo make install
```

## Running
`sudo /etc/init.d/sprinklers_pi start`

See the wiki for more information: https://github.com/rszimm/sprinklers_pi/wiki
