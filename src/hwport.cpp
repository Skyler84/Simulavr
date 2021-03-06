/*
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2001, 2002, 2003   Klaus Rudolph
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 ****************************************************************************
 *
 *  $Id$
 */

#include <iostream>

#include "hwport.h"
#include "avrdevice.h"
#include "avrerror.h"
#include <assert.h>

HWPort::HWPort(AvrDevice *core, const std::string &name, bool portToggle, int size):
    Hardware(core),
    TraceValueRegister(core, "PORT" + name),
    myName(name),
    portSize(size),
    portToggleFeature(portToggle),
    port_reg(core, this, "PORT"+ name,
             this, &HWPort::GetPort, &HWPort::SetPort),

    pin_reg(core, this, "PIN"+ name,
            this, &HWPort::GetPin, &HWPort::SetPin,
            &HWPort::GetPinBit, &HWPort::SetPinBit),

    ddr_reg(core, this, "DDR"+ name,
            this, &HWPort::GetDdr, &HWPort::SetDdr)
{
    assert((portSize >= 1) && (portSize <= sizeof(p)/sizeof(p[0])));
    portMask = (unsigned char)((1 << portSize) - 1);

    for(unsigned int tt = 0; tt < portSize; tt++) {
        // register pin to give access to pin by name
        std::string dummy = name + (char)('0' + tt);
        core->RegisterPin(dummy, &p[tt]);
        // connect to output pin
        p[tt].mask = 1 << tt;
        p[tt].pinOfPort= &pin;
        p[tt].pinRegOfPort= &pin_reg;
        // register pin output trace
        std::string tname = GetTraceValuePrefix() + name + (char)('0' + tt) + "-Out";
        pintrace[tt] = new TraceValueOutput(tname);
        pintrace[tt]->set_written(Pin::TRISTATE); // initial output driver state is tristate
        RegisterTraceValue(pintrace[tt]);
    }

    Reset();
}

HWPort::~HWPort() {
    for(int tt = portSize - 1; tt >= 0; tt--) {
        UnregisterTraceValue(pintrace[tt]);
        delete pintrace[tt];
    }
}

void HWPort::Reset(void) {
    port = 0;
    pin = 0;
    ddr = 0;

    for(int tt = portSize - 1; tt >= 0; tt--)
        p[tt].ResetOverride();
    CalcOutputs();
}

Pin& HWPort::GetPin(unsigned char pinNo) {
    assert(pinNo < sizeof(p)/sizeof(p[0]));
    return p[pinNo];
}

void HWPort::CalcOutputs(void) { // Calculate the new output value to be transmitted to the environment
    unsigned char tmpPin = 0;

    for(unsigned int actualBitNo = 0; actualBitNo < portSize; actualBitNo++) {
        unsigned char actualBit = 1 << actualBitNo;
        bool regPort = (bool)(port & actualBit);
        bool regDDR = (bool)(ddr & actualBit);

        if(p[actualBitNo].CalcPinOverride(regDDR, regPort, false))
            tmpPin |= actualBit;
        pintrace[actualBitNo]->change(p[actualBitNo].outState);
    }
//    pin = tmpPin; //BUGFIX: OpenDrain reads wrong value from PIN register
    pin_reg.hardwareChange(pin);
}

std::string HWPort::GetPortString() {
    std::string dummy;
    dummy.resize(portSize);
    for(unsigned int tt = 0; tt < portSize; tt++)
        dummy[portSize - 1 - tt] = p[tt];  // calls Pin::operator char()

    return dummy;
}

void HWPort::SetPort(unsigned char val) {
    port = val & portMask;
    CalcOutputs();
    port_reg.hardwareChange(port);
}

void HWPort::SetDdr(unsigned char val) {
    ddr = val & portMask;
    CalcOutputs();
    ddr_reg.hardwareChange(ddr);
}

void HWPort::SetPin(unsigned char val) {
    if(portToggleFeature) {
        port ^= val;
        CalcOutputs();
        port_reg.hardwareChange(port);
    } else
        avr_warning("Writing of 'PORT%s.PIN' (with %d) is not supported.", myName.c_str(), val);
}

// TODO: Please check and improve ! Untested and only copied from different sources here..
void HWPort::SetPinBit(bool val, unsigned int bitaddr) {
    if(portToggleFeature) {
        unsigned char tmpPin = pin;

        unsigned char actualBit = 1 << bitaddr;
        tmpPin &= ~actualBit;
        port ^= actualBit;

        // copied from CalcOutputs
        bool regPort = (bool)(port & actualBit);
        bool regDDR = (bool)(ddr & actualBit);

        if(p[bitaddr].CalcPinOverride(regDDR, regPort, false)) {
            tmpPin |= actualBit;
            pin=tmpPin;
        }
        pintrace[bitaddr]->change(p[bitaddr].outState);

        port_reg.hardwareChange(port);
    } else {
        avr_warning("Writing of 'PORT%s.PIN' (with %d) is not supported.", myName.c_str(), val);
    }
}

/* EOF */
