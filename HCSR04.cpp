/*
 *  HCSR04.cpp
 *
 *  US Sensor (HC-SR04) functions.
 *  The non blocking functions are using pin change interrupts and need the PinChangeInterrupt library to be installed.
 *
 *  58,23 us per centimeter and 17,17 cm/ms (forth and back).
 *
 *  Supports 1 Pin mode as you get on the HY-SRF05 if you connect OUT to ground.
 *  You can modify the HC-SR04 modules to 1 Pin mode by:
 *  Old module with 3 16 pin chips: Connect Trigger and Echo direct or use a resistor < 4.7 kOhm.
 *        If you remove both 10 kOhm pullup resistors you can use a connecting resistor < 47 kOhm, but I suggest to use 10 kOhm which is more reliable.
 *  Old module with 3 16 pin chips but with no pullup resistors near the connector row: Connect Trigger and Echo with a resistor > 200 Ohm. Use 10 kOhm.
 *  New module with 1 16 pin and 2 8 pin chips: Connect Trigger and Echo by a resistor > 200 Ohm and < 22 kOhm.
 *  All modules: Connect Trigger and Echo by a resistor of 4.7 kOhm.
 *  Some old HY-SRF05 modules of mine cannot be converted by adding a 4.7 kOhm resistor,
 *  since the output signal going low triggers the next measurement. But they work with removing the 10 kOhm pull up resistors and adding 10 kOhm.
 *
 * Sensitivity is increased by removing C3 / the low pass part of the 22 kHz Bandpass filter.
 * After this the crosstalking of the output signal will be detected as a low distance. We can avoid this by changing R7 to 0 Ohm.
 *
 *  Module Type                   |   Characteristics     |         3 Pin Mode          | Increase sensitivity
 *  ------------------------------------------------------------------------------------------------------------
 *  3 * 14 pin IC's 2 transistors | C2 below right IC/U2  | 10 kOhm pin 1+2 middle IC   | not needed, because of Max232
 *                                | right IC is Max232    |                             |
 *  3 * 14 pin IC's 2 transistors | Transistor between    |                             | -C2, R11=1.5MOhm, R12=0
 *                                | middle and right IC   |                             |
 *  3 * 14 pin IC's               | R17 below right IC    | 10 kOhm pin 1+2 middle IC   |
 *  1*4 2*8 pin IC's              |                       | 10 kOhm pin 3+4 right IC    | -C4, R7=1.5MOhm, R10=0
 *  HY-SRF05 3 * 14 pin IC's      |                       | 10 kOhm pin 1+2 middle IC   | - bottom left C, R16=1.5MOhm, R15=?
 *
 *  The CS100A module is not very sensitive at short or mid range but can detect up to 3m. Smallest distance is 2 cm.
 *  The amplified analog signal is available at pin 5 and the comparator output at pin 6. There you can see other echoes.
 *  3 Pin mode is difficult since it retriggers itself at distances below 7 cm.
 *
 *  Copyright (C) 2018-2020  Armin Joachimsmeyer
 *  Email: armin.joachimsmeyer@gmail.com
 *
 *  This file is part of Arduino-Utils https://github.com/ArminJo/Arduino-Utils.
 *
 *  Arduino-Utils is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>
#include "HCSR04.h"

//#define DEBUG

uint8_t sTriggerOutPin; // also used as aTriggerOutEchoInPin for 1 pin mode
uint8_t sEchoInPin;

uint8_t sHCSR04Mode = HCSR04_MODE_UNITITIALIZED;

/*
 * @param aEchoInPin - If 0 then assume 1 pin mode
 */
void initUSDistancePins(uint8_t aTriggerOutPin, uint8_t aEchoInPin) {
    sTriggerOutPin = aTriggerOutPin;
    if (aEchoInPin == 0) {
        sHCSR04Mode = HCSR04_MODE_USE_1_PIN;
    } else {
        sEchoInPin = aEchoInPin;
        pinMode(aTriggerOutPin, OUTPUT);
        pinMode(sEchoInPin, INPUT);
        sHCSR04Mode = HCSR04_MODE_USE_2_PINS;
    }
}
/*
 * Using this determines one pin mode
 */
void initUSDistancePin(uint8_t aTriggerOutEchoInPin) {
    sTriggerOutPin = aTriggerOutEchoInPin;
    sHCSR04Mode = HCSR04_MODE_USE_1_PIN;
}

/*
 * Start of standard blocking implementation using pulseInLong() since PulseIn gives wrong (too small) results :-(
 * @return 0 if uninitialized or timeout happened
 */
unsigned int getUSDistance(unsigned int aTimeoutMicros) {
    if (sHCSR04Mode == HCSR04_MODE_UNITITIALIZED) {
        return 0;
    }

// need minimum 10 usec Trigger Pulse
    digitalWrite(sTriggerOutPin, HIGH);

    if (sHCSR04Mode == HCSR04_MODE_USE_1_PIN) {
        // do it AFTER digitalWrite to avoid spurious triggering by just switching pin to output
        pinMode(sTriggerOutPin, OUTPUT);
    }

#ifdef DEBUG
    delayMicroseconds(100); // to see it on scope
#else
    delayMicroseconds(10);
#endif
// falling edge starts measurement after 400/600 microseconds (old/new modules)
    digitalWrite(sTriggerOutPin, LOW);

    uint8_t tEchoInPin;
    if (sHCSR04Mode == HCSR04_MODE_USE_1_PIN) {
        // allow for 20 us low (20 us instead of 10 us also supports the JSN-SR04T) before switching to input which is high because of the modules pullup resistor.
        delayMicroseconds(20);
        pinMode(sTriggerOutPin, INPUT);
        tEchoInPin = sTriggerOutPin;
    } else {
        tEchoInPin = sEchoInPin;
    }

    /*
     * Get echo length.
     * Speed of sound is: 331.5 + (0.6 * TemperatureCelsius).
     * Exact value at 20 degree celsius is 343,46 m/s => 58,23 us per centimeter and 17,17 cm/ms (forth and back)
     * Exact value at 10 degree celsius is 337,54 m/s => 59,25 us per centimeter and 16,877 cm/ms (forth and back)
     * At 20 degree celsius => 50cm gives 2914 us, 2m gives 11655 us
     *
     * Use pulseInLong, this uses micros() as counter, relying on interrupts being enabled, which is not disturbed by (e.g. the 1 ms timer) interrupts.
     * Only thing is that the pulse ends when we are in an interrupt routine, thus prolonging the measured pulse duration.
     * I measured 6 us for the millis() and 14 to 20 us for the Servo signal generating interrupt. This is equivalent to around 1 to 3 mm distance.
     * Alternatively we can use pulseIn() in a noInterrupts() context, but this will effectively stop the millis() timer for duration of pulse / or timeout.
     */
#if ! defined(__AVR__) || defined(TEENSYDUINO) || defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
    noInterrupts();
    unsigned long tUSPulseMicros = pulseIn(tEchoInPin, HIGH, aTimeoutMicros);
    interrupts();
#else
    unsigned long tUSPulseMicros = pulseInLong(tEchoInPin, HIGH, aTimeoutMicros);
#endif
    return tUSPulseMicros;
}

unsigned int getCentimeterFromUSMicroSeconds(unsigned int aDistanceMicros) {
    // The reciprocal of formula in getUSDistanceAsCentiMeterWithCentimeterTimeout()
    return (aDistanceMicros * 100L) / 5825;
}

/*
 * @return  Distance in centimeter @20 degree (time in us/58.25)
 *          0 if timeout or pins are not initialized
 *
 *          timeout of 5825 micros is equivalent to 1 meter
 *          Default timeout of 20000 micro seconds is 3.43 meter
 */
unsigned int getUSDistanceAsCentiMeter(unsigned int aTimeoutMicros) {
    return (getCentimeterFromUSMicroSeconds(getUSDistance(aTimeoutMicros)));
}

// 58,23 us per centimeter (forth and back)
unsigned int getUSDistanceAsCentiMeterWithCentimeterTimeout(unsigned int aTimeoutCentimeter) {
// The reciprocal of formula in getCentimeterFromUSMicroSeconds()
    unsigned int tTimeoutMicros = ((aTimeoutCentimeter * 233L) + 2) / 4; // = * 58.25 (rounded by using +1)
    return getUSDistanceAsCentiMeter(tTimeoutMicros);
}

/*
 * Trigger US sensor as fast as sensible if called in a loop to test US devices.
 * trigger pulse is equivalent to 10 cm and then we wait for 20 ms / 3.43 meter
 */
void testUSSensor(uint16_t aSecondsToTest) {
    for (long i = 0; i < aSecondsToTest * 50; ++i) {
        digitalWrite(sTriggerOutPin, HIGH);
        delayMicroseconds(582); // pulse is as long as echo for 10 cm
        // falling edge starts measurement
        digitalWrite(sTriggerOutPin, LOW);
        delay(20); // wait time for 3,43 meter to let the US pulse vanish
    }
}

/*
 * The NON BLOCKING version only blocks for ca. 12 microseconds for code + generation of trigger pulse
 * Be sure to have the right interrupt vector below.
 * check with: while (!isUSDistanceMeasureFinished()) {<do something> };
 * Result is in sUSDistanceCentimeter;
 */

// Activate the line according to the sEchoInPin if using the non blocking version
// or define it as symbol for the compiler e.g. -DUSE_PIN_CHANGE_INTERRUPT_D0_TO_D7
//#define USE_PIN_CHANGE_INTERRUPT_D0_TO_D7  // using PCINT2_vect - PORT D
//#define USE_PIN_CHANGE_INTERRUPT_D8_TO_D13 // using PCINT0_vect - PORT B - Pin 13 is feedback output
//#define USE_PIN_CHANGE_INTERRUPT_A0_TO_A5  // using PCINT1_vect - PORT C
#if (defined(USE_PIN_CHANGE_INTERRUPT_D0_TO_D7) | defined(USE_PIN_CHANGE_INTERRUPT_D8_TO_D13) | defined(USE_PIN_CHANGE_INTERRUPT_A0_TO_A5))

unsigned int sUSDistanceCentimeter;
volatile unsigned long sUSPulseMicros;

volatile bool sUSValueIsValid = false;
volatile unsigned long sMicrosAtStartOfPulse;
unsigned int sTimeoutMicros;

/*
 * common code for all interrupt handler.
 */
void handlePCInterrupt(uint8_t aPortState) {
    if (aPortState > 0) {
        // start of pulse
        sMicrosAtStartOfPulse = micros();
    } else {
        // end of pulse
        sUSPulseMicros = micros() - sMicrosAtStartOfPulse;
        sUSValueIsValid = true;
    }
#ifdef DEBUG
// for debugging purposes, echo to PIN 13 (do not forget to set it to OUTPUT!)
// digitalWrite(13, aPortState);
#endif
}
#endif // USE_PIN_CHANGE_INTERRUPT_D0_TO_D7 ...

#if defined(USE_PIN_CHANGE_INTERRUPT_D0_TO_D7)
/*
 * pin change interrupt for D0 to D7 here.
 */
ISR (PCINT2_vect) {
// read pin
    uint8_t tPortState = (*portInputRegister(digitalPinToPort(sEchoInPin))) & bit((digitalPinToPCMSKbit(sEchoInPin)));
    handlePCInterrupt(tPortState);
}
#endif

#if defined(USE_PIN_CHANGE_INTERRUPT_D8_TO_D13)
/*
 * pin change interrupt for D8 to D13 here.
 * state of pin is echoed to output 13 for debugging purpose
 */
ISR (PCINT0_vect) {
// check pin
    uint8_t tPortState = (*portInputRegister(digitalPinToPort(sEchoInPin))) & bit((digitalPinToPCMSKbit(sEchoInPin)));
    handlePCInterrupt(tPortState);
}
#endif

#if defined(USE_PIN_CHANGE_INTERRUPT_A0_TO_A5)
/*
 * pin change interrupt for A0 to A5 here.
 * state of pin is echoed to output 13 for debugging purpose
 */
ISR (PCINT1_vect) {
// check pin
    uint8_t tPortState = (*portInputRegister(digitalPinToPort(sEchoInPin))) & bit((digitalPinToPCMSKbit(sEchoInPin)));
    handlePCInterrupt(tPortState);
}
#endif

#if (defined(USE_PIN_CHANGE_INTERRUPT_D0_TO_D7) | defined(USE_PIN_CHANGE_INTERRUPT_D8_TO_D13) | defined(USE_PIN_CHANGE_INTERRUPT_A0_TO_A5))

void startUSDistanceAsCentiMeterWithCentimeterTimeoutNonBlocking(unsigned int aTimeoutCentimeter) {
// need minimum 10 usec Trigger Pulse
    digitalWrite(sTriggerOutPin, HIGH);
    sUSValueIsValid = false;
    sTimeoutMicros = ((aTimeoutCentimeter * 233) + 2) / 4; // = * 58.25 (rounded by using +1)
    *digitalPinToPCMSK(sEchoInPin) |= bit(digitalPinToPCMSKbit(sEchoInPin));// enable pin for pin change interrupt
// the 2 registers exists only once!
    PCICR |= bit(digitalPinToPCICRbit(sEchoInPin));// enable interrupt for the group
    PCIFR |= bit(digitalPinToPCICRbit(sEchoInPin));// clear any outstanding interrupt
    sUSPulseMicros = 0;
    sMicrosAtStartOfPulse = 0;

#ifdef DEBUG
    delay(2); // to see it on scope
#else
    delayMicroseconds(10);
#endif
// falling edge starts measurement and generates first interrupt
    digitalWrite(sTriggerOutPin, LOW);
}

/*
 * Used to check by polling.
 * If ISR interrupts these code, everything is fine, even if we get a timeout and a no null result
 * since we are interested in the result and not in very exact interpreting of the timeout.
 */
bool isUSDistanceMeasureFinished() {
    if (sUSValueIsValid) {
        sUSDistanceCentimeter = getCentimeterFromUSMicroSeconds(sUSPulseMicros);
        return true;
    }

    if (sMicrosAtStartOfPulse != 0) {
        if ((micros() - sMicrosAtStartOfPulse) >= sTimeoutMicros) {
            // Timeout happened, value will be 0
            *digitalPinToPCMSK(sEchoInPin) &= ~(bit(digitalPinToPCMSKbit(sEchoInPin)));// disable pin for pin change interrupt
            return true;
        }
    }
    return false;
}
#endif // USE_PIN_CHANGE_INTERRUPT_D0_TO_D7 ...