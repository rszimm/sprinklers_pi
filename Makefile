#
# Makefile for the Sprinkling System
# $Id: Makefile 578 2012-10-08 22:42:44Z rzimmerman $

VERSION=1.0.1

BUILD_DIR=build
CC=gcc
CCFLAGS=-O3 -Wall -fmessage-length=0 -MMD -MP -DLOGGING

CPP_SRCS += \
Event.cpp \
Logging.cpp \
Weather.cpp \
core.cpp \
port.cpp \
settings.cpp \
sprinklers_pi.cpp \
sysreset.cpp \
web.cpp 

LIBS := -lsqlite3 -lwiringPi
LIBNAME=sprinklers_pi

OBJS=$(CPP_SRCS:%.cpp=$(BUILD_DIR)/%.o)

# Load the svnversion.cpp file assuming the version has not changed.
DUMMY:=$(shell echo -n 'static const char* svn_version(void) { return "' > svnversion.cpp.tmp )
DUMMY:=$(shell svnversion -c .. | awk -F ':' '{ printf("%s", $$2); }' >> svnversion.cpp.tmp )
DUMMY:=$(shell echo '"; }' >> svnversion.cpp.tmp )
DUMMY:=$(shell echo 'static const char* lib_version(void) { return "$(VERSION)"; }' >> svnversion.cpp.tmp )
DUMMY:=$(shell if [ ! -f svnversion.cpp ]; then touch svnversion.cpp; fi )
DUMMY:=$(shell diff --brief svnversion.cpp svnversion.cpp.tmp; if [ $$? = 1 ]; then cp -f svnversion.cpp.tmp svnversion.cpp; fi )
DUMMY:=$(shell rm -f svnversion.cpp.tmp)

all: build_dir $(LIBNAME)

$(LIBNAME): $(OBJS)
	@echo 'Building target: $@'
	g++  -o "$@" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

build_dir: ${BUILD_DIR}

${BUILD_DIR}:
	mkdir -p ${BUILD_DIR}

$(BUILD_DIR)/%.o: %.cpp
	$(CC) $(CCFLAGS) -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"

.PHONY: build_dir

#Misc stuff below here..

IUSER = $(shell whoami)

install: all
ifneq  ($(IUSER),root)
	$(error You are not ROOT.  Rerun with sudo)
endif
	@cp -f $(LIBNAME) /usr/local/sbin
	mkdir -p /web
	-@cp -f web/* /web/
	cp -f sprinklers_init.d.sh /etc/init.d/sprinklers_pi
	chmod a+x /etc/init.d/sprinklers_pi
	mkdir -p /usr/local
	mkdir -p /usr/local/etc
	mkdir -p /usr/local/etc/sprinklers_pi
	update-rc.d sprinklers_pi defaults
	
	@echo "done"

remove:
ifneq  ($(IUSER),root)
	$(error You are not ROOT.  Rerun with sudo)
endif
	/etc/init.d/sprinklers_pi stop
	rm -f /etc/init.d/sprinklers_pi
	update-rc.d sprinklers_pi remove
	rm -rf /usr/local/etc/sprinklers_pi
	rm -f /usr/local/sbin/sprinklers_pi
	rm -rf /web

package: clean
	@mkdir -p /tmp/$(LIBNAME)-$(VERSION)
	@cp -rf * /tmp/$(LIBNAME)-$(VERSION)
	@rm -rf /tmp/$(LIBNAME)-$(VERSION)/web/json
	@rm -rf /tmp/$(LIBNAME)-$(VERSION)/web/bin
	tar -C/tmp -czvhf $(LIBNAME)-$(VERSION).tar.gz $(LIBNAME)-$(VERSION)
	rm -rf /tmp/$(LIBNAME)-$(VERSION)

version:
	@echo $(VERSION)

clean:
	rm -rf $(BUILD_DIR) $(LIBNAME) svnversion.cpp *.tar.gz

FORCE:
# DO NOT DELETE
