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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ****************************************************************************
 *
 *  $Id$
 */

#include "attiny25_45_85.h"

#include "hardware.h"
#include "irqsystem.h"
#include "hwport.h"
#include "hwstack.h"
#include "hweeprom.h"
#include "hwwado.h"
#include "hwsreg.h"
#include "flashprog.h"

#include "avrfactory.h"

AVR_REGISTER(attiny25, AvrDevice_attiny25);
AVR_REGISTER(attiny45, AvrDevice_attiny45);
AVR_REGISTER(attiny85, AvrDevice_attiny85);

AvrDevice_attinyX5::~AvrDevice_attinyX5() {
    // destroy subsystems in reverse order, you've created it in constructor
    delete timer0;
    delete timer01irq;
    delete prescaler0;
    delete gtccr_reg;
    delete gpior2_reg;
    delete gpior1_reg;
    delete gpior0_reg;
    delete portb;
    delete stack;
    delete eeprom;
    delete irqSystem;
}

AvrDevice_attinyX5::AvrDevice_attinyX5(unsigned ram_bytes,
                                       unsigned flash_bytes,
                                       unsigned ee_bytes):
    AvrDevice(64 ,          // I/O space above General Purpose Registers
              ram_bytes,    // RAM size
              0,            // External RAM size
              flash_bytes)  // Flash Size
{
    flagJMPInstructions = false;
    flagMULInstructions = false;
    irqSystem = new HWIrqSystem(this, 2, 15); // 2 bytes per vector, 15 vectors
    eeprom = new HWEeprom(this, irqSystem, ee_bytes, 6, HWEeprom::DEVMODE_EXTENDED); 
    stack = new HWStackSram(this, 12);
    
    portb = new HWPort(this, "B", true, 6);

    gpior0_reg = new GPIORegister(this, &coreTraceGroup, "GPIOR0");
    gpior1_reg = new GPIORegister(this, &coreTraceGroup, "GPIOR1");
    gpior2_reg = new GPIORegister(this, &coreTraceGroup, "GPIOR2");
    
    // GTCCR register an timer 0
    gtccr_reg = new IOSpecialReg(&coreTraceGroup, "GTCCR");
    prescaler0 = new HWPrescaler(this, "0", gtccr_reg, 0, 7);

    timer01irq = new TimerIRQRegister(this, irqSystem);
    timer01irq->registerLine(1, new IRQLine("TOV0",   5));
    timer01irq->registerLine(2, new IRQLine("TOV1",   4));
    timer01irq->registerLine(3, new IRQLine("OCF0B", 11));
    timer01irq->registerLine(4, new IRQLine("OCF0A", 10));
    timer01irq->registerLine(5, new IRQLine("OCF1B",  9));
    timer01irq->registerLine(6, new IRQLine("OCF1A",  3));
    
    timer0 = new HWTimer8_2C(this,
                             new PrescalerMultiplexerExt(prescaler0, PinAtPort(portb, 2)),
                             0,
                             timer01irq->getLine("TOV0"),
                             timer01irq->getLine("OCF0A"),
                             new PinAtPort(portb, 0),
                             timer01irq->getLine("OCF0B"),
                             new PinAtPort(portb, 1));

    // IO register set
    rw[0x5f]= statusRegister;
    rw[0x5e]= & ((HWStackSram *)stack)->sph_reg;
    rw[0x5d]= & ((HWStackSram *)stack)->spl_reg;
    //rw[0x5c] reserved
    //rw[0x5b] reserved
    //rw[0x5a] reserved
    rw[0x59]= & timer01irq->timsk_reg;
    rw[0x58]= & timer01irq->tifr_reg;
    //rw[0x57] reserved
    //rw[0x56] reserved
    //rw[0x55] reserved
    //rw[0x54] reserved
    rw[0x53]= & timer0->tccrb_reg;
    rw[0x52]= & timer0->tcnt_reg;
    //rw[0x51] reserved
    //rw[0x50] reserved

    //rw[0x4f] reserved
    //rw[0x4e] reserved
    //rw[0x4d] reserved
    //rw[0x4c] reserved
    //rw[0x4b] reserved
    rw[0x4a]= & timer0->tccra_reg;
    rw[0x49]= & timer0->ocra_reg;
    rw[0x48]= & timer0->ocrb_reg;
    //rw[0x47] reserved
    //rw[0x46] reserved
    //rw[0x45] reserved
    //rw[0x44] reserved
    //rw[0x43] reserved
    //rw[0x42] reserved
    //rw[0x41] reserved
    //rw[0x40] reserved

    rw[0x3f]= & eeprom->eearh_reg;
    rw[0x3e]= & eeprom->eearl_reg;
    rw[0x3d]= & eeprom->eedr_reg;
    rw[0x3c]= & eeprom->eecr_reg;
    //rw[0x3b] reserved
    //rw[0x3a] reserved
    //rw[0x39] reserved
    rw[0x38]= & portb->port_reg;
    rw[0x37]= & portb->ddr_reg;
    rw[0x36]= & portb->pin_reg;
    //rw[0x35] reserved
    //rw[0x34] reserved
    rw[0x33]= gpior2_reg;
    rw[0x32]= gpior1_reg;
    rw[0x31]= gpior0_reg;
    //rw[0x30] reserved

    //rw[0x2f] reserved
    //rw[0x2e] reserved
    //rw[0x2d] reserved
    //rw[0x2c] reserved
    //rw[0x2b] reserved
    //rw[0x2a] reserved
    //rw[0x29] reserved
    //rw[0x28] reserved
    //rw[0x27] reserved
    //rw[0x26] reserved
    //rw[0x25] reserved
    //rw[0x24] reserved
    //rw[0x23] reserved
    //rw[0x22] reserved
    //rw[0x21] reserved
    //rw[0x20] reserved

    Reset();
}

/* EOF */

