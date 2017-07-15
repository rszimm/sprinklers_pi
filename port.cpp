// port.cpp
// This file defines certain functions necessary to port the original
//  AVR (e.g. Arduino) system to a more standard Linux based (e.g. Raspberry Pi) system.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include "port.h"
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
void trace(const char * fmt, ...)
{
	time_t curTime = time(0);
	struct tm * ti = localtime (&curTime);
	printf("%.4d/%.2d/%.2d %.2d:%.2d:%.2d ", 1900 + ti->tm_year, ti->tm_mon+1, ti->tm_mday, ti->tm_hour, ti->tm_min, ti->tm_sec);
	va_list parms;
	va_start(parms, fmt);
	vprintf(fmt, parms);
	va_end(parms);
	fflush(stdout);
}

IPAddress::IPAddress()
{
	_address[0] = 0;
	_address[1] = 0;
	_address[2] = 0;
	_address[3] = 0;
}

IPAddress::IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet)
{
	_address[0] = first_octet;
	_address[1] = second_octet;
	_address[2] = third_octet;
	_address[3] = fourth_octet;
}

EEPROMClass::EEPROMClass()
		: m_changed(false)
{
	memset(m_buf, 0, sizeof(m_buf));
	FILE * fd = fopen("settings", "rb");
	if (!fd)
		return;
	fread(m_buf, 1, sizeof(m_buf), fd);
	fclose(fd);
}

EEPROMClass::~EEPROMClass()
{
}

uint8_t EEPROMClass::read(int addr)
{
	return m_buf[addr];
}

void EEPROMClass::write(int addr, uint8_t val)
{
	m_buf[addr] = val;
	m_changed = true;
}

void EEPROMClass::Store()
{
	if (m_changed)
	{
		m_changed = false;
		FILE * fd = fopen("settings", "wb");
		if (!fd)
			return trace("Failed to open settings file\n");
		fwrite(m_buf, 1, sizeof(m_buf), fd);
		fclose(fd);
	}
}

EEPROMClass EEPROM;

EthernetServer::EthernetServer(uint16_t port)
		: m_port(port), m_sock(0)
{
}

EthernetServer::~EthernetServer()
{
	close(m_sock);
}

bool EthernetServer::begin()
{
	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_port = htons(m_port);
	sin.sin_addr.s_addr = INADDR_ANY;

	if ((m_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		trace("Can't create shell listen socket %d (%d)%s\n", m_sock, errno, strerror(errno));
		return false;
	}
	int on = 1;
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
	{
		trace("Can't setsockopt %d (%d)%s\n", m_sock, errno, strerror(errno));
		return false;
	}
	if (bind(m_sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)
	{
		trace("shell bind error (%d)%s\n", errno, strerror(errno));
		return false;
	}
	if (ioctl(m_sock, FIONBIO, (char*) &on) < 0)
	{
		trace("setting nonblock failed(%d)%s\n", errno, strerror(errno));
		return false;
	}
	if (listen(m_sock, 2) < 0)
	{
		trace("shell listen error(%d)%s\n", errno, strerror(errno));
		return false;
	}
	return true;
}

//  This function blocks until we get a client connected.
//   It will timeout after 50ms and return a blank client.
//   If it succeeds it will return an EthernetClient.
EthernetClient EthernetServer::available()
{
	fd_set sock_set;
	FD_ZERO(&sock_set);
	FD_SET(m_sock, &sock_set);
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 50 * 1000; // 50ms

	select(m_sock + 1, &sock_set, NULL, NULL, &timeout);
	if (FD_ISSET(m_sock, &sock_set))
	{
		int client_sock = 0;
		struct sockaddr_in cli_addr;
		unsigned int clilen = sizeof(cli_addr);
		if ((client_sock = accept(m_sock, (struct sockaddr *) &cli_addr, &clilen)) <= 0)
			return EthernetClient(0);
		return EthernetClient(client_sock);
	}
	return EthernetClient(0);
}

EthernetClient::EthernetClient()
		: m_sock(0), m_connected(false)
{
}

EthernetClient::EthernetClient(int sock)
		: m_sock(sock), m_connected(true)
{
}

EthernetClient::~EthernetClient()
{
	stop();
}

int EthernetClient::connect(IPAddress ip, uint16_t port)
{
	if (m_sock)
		return 0;
	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = *(uint32_t*) (ip.raw_address());
	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (::connect(m_sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)
	{
		trace("Error Connecting(%d)%s\n", errno, strerror(errno));
		return false;
	}
	m_connected = true;
	return 1;
}
int EthernetClient::connect(const char* host, uint16_t port)
{
	if (m_sock)
		return 0;
	trace("Trying to connect to:%s\n", host);

	hostent * resolvedHost = gethostbyname(host);
	if(resolvedHost == NULL)
	{
		trace("Error resolving %s (%d)%s\n",host, errno, strerror(errno));
	}
	
	in_addr * actualIP = (in_addr * )resolvedHost->h_addr;
	char* chIpAddress = inet_ntoa(* actualIP);
	m_ipAddress=chIpAddress;
	trace("Resolved %s to:%s\n",host, chIpAddress);

	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr=*actualIP;
	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (::connect(m_sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)
	{
		trace("Error Connecting(%d)%s\n", errno, strerror(errno));
		return false;
	}
	m_connected = true;
	return 1;
}
bool EthernetClient::connected()
{
	if (!m_sock)
		return false;
	int error = 0;
	socklen_t len = sizeof(error);
	int retval = getsockopt(m_sock, SOL_SOCKET, SO_ERROR, &error, &len);
	return (retval == 0) && m_connected;
}

void EthernetClient::stop()
{
	if (m_sock)
	{
		close(m_sock);
		m_sock = 0;
		m_connected = false;
	}
}

EthernetClient::operator bool()
{
	return m_sock != 0;
}

// read data from the client into the buffer provided
//  This function will block until either data is received OR a timeout happens.
//  If an error occurs or a timeout happens, we set the disconnect flag on the socket
//  and return 0;
int EthernetClient::read(uint8_t *buf, size_t size)
{
	fd_set sock_set;
	FD_ZERO(&sock_set);
	FD_SET(m_sock, &sock_set);
	struct timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	select(m_sock + 1, &sock_set, NULL, NULL, &timeout);
	if (FD_ISSET(m_sock, &sock_set))
	{
		int retval = ::read(m_sock, buf, size);
		if (retval <= 0) // socket closed
			m_connected = false;
		return retval;
	}
	m_connected = false;
	return 0;
}

size_t EthernetClient::write(const uint8_t *buf, size_t size)
{
	return ::write(m_sock, buf, size);
}

SdFile::SdFile()
		: m_fid(0)
{
}

SdFile::~SdFile()
{
	close();
}

bool SdFile::open(const char* path, uint8_t oflag)
{
	m_fid = fopen(path, "r");
	return m_fid != 0;
}

bool SdFile::isFile() const
{
	return true;
}

bool SdFile::close()
{
	if (m_fid)
		fclose(m_fid);
	m_fid = 0;
	return true;
}

bool SdFile::available()
{
	return !feof(m_fid);
}

int SdFile::read(void* buf, size_t nbyte)
{
	return fread(buf, 1, nbyte, m_fid);
}

