#
# Makefile for the Sprinkling System

VERSION=1.1.1

BUILD_DIR=build
CC=gcc
CCFLAGS=-O3 -Wall -fmessage-length=0 -MMD -MP -DLOGGING -DVERSION=\"$(VERSION)\"

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
	-@cp -rf web/* /web/
	cp -f sprinklers_init.d.sh /etc/init.d/sprinklers_pi
	chmod a+x /etc/init.d/sprinklers_pi
	mkdir -p /usr/local
	mkdir -p /usr/local/etc
	mkdir -p /usr/local/etc/sprinklers_pi
	update-rc.d sprinklers_pi defaults
	
	@echo "done"

upgrade: install
	/etc/init.d/sprinklers_pi restart

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
	rm -rf $(BUILD_DIR) $(LIBNAME) settings db.sql *.tar.gz

FORCE:
# DO NOT DELETE
