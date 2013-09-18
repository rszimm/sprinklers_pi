// sysreset.cpp
// System Reset wrapper
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include "sysreset.h"

#ifndef ARDUINO
/////////////////////////////
//  Linux Code
//
#include <sys/reboot.h>
#include <stdlib.h>

void sysreset()
{
	reboot(RB_AUTOBOOT);
	exit(0);
}

#else
/////////////////////////////
//  Arduino AVR Code
//
#include <avr/io.h>
#include <avr/wdt.h>

// Function Pototype
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

// Function Implementation
void wdt_init(void)
{
    MCUSR = 0;
    wdt_disable();

    return;
}

void sysreset()
{
        //wdt_enable(WDTO_120MS);
        //while(1) {};
        asm volatile ("  jmp 0");
}

#endif
