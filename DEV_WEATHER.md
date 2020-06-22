# Developing New Weather Provider Plugins

1. Inherit Weather class and Override
```
private:
	Weather::ReturnVals InternalGetVals(const Weather::Settings & settings) const;
```
2. Add commented out #define WEATHER_YOUR_PROVIDER line to config.h
3. Add #elif defined(WEATHER_YOUR_PROVIDER) blocks to the following places:
  * core.cpp includes
  * core.cpp `static runStateClass::DurationAdjustments AdjustDurations(Schedule * sched)` function
  * web.cpp includes
  * web.cpp `static void JSONSettings(const KVPairs & key_value_pairs, FILE * stream_file)` function
  * web.cpp `static void JSONwCheck(const KVPairs & key_value_pairs, FILE * stream_file)` function
4. Add any new files to CMakeLists.txt and Makefile
