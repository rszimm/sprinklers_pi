// port.h
// This file defines certain functions necessary to port the original
//  AVR (e.g. Arduino) system to a more standard Linux based (e.g. Raspberry Pi) system.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#ifndef _PORT_H_
#define _PORT_H_
#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>
#define F(d) d
#define PSTR(d) d
#define fprintf_P fprintf

void trace(const char * fmt, ...);

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

static inline void freeMemory()
{
}

class IPAddress
{
private:
	uint8_t _address[4];
public:
	IPAddress();
	IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet);
	uint8_t* raw_address()
	{
		return _address;
	}

	uint8_t operator[](int index) const
	{
		return _address[index];
	}
	;
	uint8_t& operator[](int index)
	{
		return _address[index];
	}
	;
};

class EEPROMClass
{
public:
	EEPROMClass();
	~EEPROMClass();
	uint8_t read(int addr);
	void write(int addr, uint8_t);
	void Store();
private:
	uint8_t m_buf[2048];
	bool m_changed;
};

extern EEPROMClass EEPROM;

const IPAddress INADDR_NONE(0, 0, 0, 0);

#include <time.h>
class nntp
{
public:
	time_t LocalNow()
	{
		time_t t = time(0);
		struct tm * ti = localtime(&t);
		return t + ti->tm_gmtoff;
	}
	void checkTime()
	{
	}
};
static inline time_t now()
{
	return time(0);
}

// time manipulation functions
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)
#define DAYS_PER_WEEK (7UL)
#define SECS_PER_WEEK (SECS_PER_DAY * DAYS_PER_WEEK)
#define SECS_PER_YEAR (SECS_PER_WEEK * 52UL)
#define previousMidnight(_time_) (( _time_ / SECS_PER_DAY) * SECS_PER_DAY)  // time at the start of the given day
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  // this is number of days since Jan 1 1970
static inline int hour(time_t t)
{
	struct tm * ptm;
	ptm = gmtime(&t);
	return ptm->tm_hour;
}

static inline int minute(time_t t)
{
	struct tm * ptm;
	ptm = gmtime(&t);
	return ptm->tm_min;
}

static inline int second(time_t t)
{
	struct tm * ptm;
	ptm = gmtime(&t);
	return ptm->tm_sec;
}

static inline int year(time_t t)
{
	struct tm * ptm;
	ptm = gmtime(&t);
	return ptm->tm_year + 1900;
}

static inline int month(time_t t)
{
	struct tm * ptm;
	ptm = gmtime(&t);
	return ptm->tm_mon + 1;
}

static inline int day(time_t t)
{
	struct tm * ptm;
	ptm = gmtime(&t);
	return ptm->tm_mday;
}

static inline int weekday(time_t t)
{
	struct tm * ptm;
	ptm = gmtime(&t);
	return ptm->tm_wday + 1;
}

class EthernetServer;

class EthernetClient
{
public:
	EthernetClient();
	EthernetClient(int sock);
	~EthernetClient();
	int connect(IPAddress ip, uint16_t port);
	int connect(const char* host, uint16_t port);
	bool connected();
	void stop();
	int read(uint8_t *buf, size_t size);
	size_t write(const uint8_t *buf, size_t size);
	operator bool();
	int GetSocket()
	{
		return m_sock;
	}
	char* GetIpAddress()
	{
		return m_ipAddress;
	}
private:
	char* m_ipAddress;
	int m_sock;
	bool m_connected;
	friend class EthernetServer;
};

class EthernetServer
{
public:
	EthernetServer(uint16_t port);
	~EthernetServer();

	bool begin();
	EthernetClient available();
private:
	uint16_t m_port;
	int m_sock;
};

uint8_t const O_READ = 0X01;
class SdFile
{
public:
	SdFile();
	~SdFile();
	bool open(const char* path, uint8_t oflag = O_READ);
	bool isFile() const;
	bool close();
	bool available();
	int read(void* buf, size_t nbyte);
private:
	FILE * m_fid;
};

#endif /* PORT_H_ */
