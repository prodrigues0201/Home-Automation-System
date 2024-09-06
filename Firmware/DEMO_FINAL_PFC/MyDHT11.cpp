/**
 * @file MyDHT11.cpp
 * @brief Implementation of a custom DHT11 sensor library.
 * 
 * This file provides the implementation of the functions declared in MyDHT11.h.
 */

#include "MyDHT11.h"

/**
 * @brief Constructs a new MyDHT11 object.
 * 
 * Initializes the DHT11 sensor by setting the provided GPIO pin as input.
 * 
 * @param pin The GPIO pin where the DHT11 sensor is connected.
 */
MyDHT11::MyDHT11(uint8_t pin) : _pin(pin), _temperature(0), _humidity(0) {
    pinMode(_pin, INPUT);
}

/**
 * @brief Reads temperature and humidity from the DHT11 sensor.
 * 
 * This function communicates with the DHT11 sensor and attempts to read 
 * both temperature and humidity data. It includes timing-sensitive operations
 * to extract data bits from the sensor.
 * 
 * @return bool Returns true if the reading was successful, false otherwise.
 */
bool MyDHT11::read() {
    uint8_t bits[5] = {0};   ///< Array to store the bits from the sensor.
    uint8_t cnt = 7;         ///< Counter for bit position.
    uint8_t idx = 0;         ///< Index for the bits array.

    // Prepare to read from the sensor
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    delay(18);
    digitalWrite(_pin, HIGH);
    delayMicroseconds(40);
    pinMode(_pin, INPUT_PULLUP);

    unsigned int loopCnt = 10000;
    while (digitalRead(_pin) == LOW) {
        if (loopCnt-- == 0) return false;
    }
    
    loopCnt = 30000;
    while (digitalRead(_pin) == HIGH) {
        if (loopCnt-- == 0) return false;
    }

    // Reading the 40-bit data from the sensor
    for (int i = 0; i < 40; i++) {
        loopCnt = 10000;
        while (digitalRead(_pin) == LOW) {
            if (loopCnt-- == 0) return false;
        }

        unsigned long t = micros();
        while (digitalRead(_pin) == HIGH) {
            if (loopCnt-- == 0) return false;
        }

        if ((micros() - t) > 50) {
            bits[idx] |= (1 << cnt);
        }

        if (cnt == 0) {
            cnt = 7;
            idx++;
        } else {
            cnt--;
        }
    }

    _humidity = bits[0];    ///< The humidity value read from the sensor.
    _temperature = bits[2]; ///< The temperature value read from the sensor.

    // Checksum validation
    if (bits[4] == ((bits[0] + bits[2]) & 0xFF)) {
        return true;
    } else {
        Serial.println("Checksum validation failed.");
        Serial.print("Calculated checksum: ");
        Serial.println((bits[0] + bits[2]) & 0xFF);
        Serial.print("Received checksum: ");
        Serial.println(bits[4]);
        return false;
    }
}

/**
 * @brief Returns the last successful temperature reading.
 * 
 * @return int The temperature in degrees Celsius.
 */
int MyDHT11::getTemperature() const {
    return _temperature;
}

/**
 * @brief Returns the last successful humidity reading.
 * 
 * @return int The humidity in percentage.
 */
int MyDHT11::getHumidity() const {
    return _humidity;
}

/**
 * @brief Waits for the sensor to reach a specific state (HIGH or LOW) within a timeout.
 * 
 * This function is used for timing-sensitive operations during communication with the sensor.
 * 
 * @param state The expected state (HIGH or LOW) to wait for.
 * @param timeout The maximum time to wait for the state in microseconds.
 * @return bool True if the expected state was detected, false otherwise.
 */
bool MyDHT11::waitForState(uint8_t state, unsigned long timeout) {
    unsigned long startTime = micros();
    while (digitalRead(_pin) != state) {
        if (micros() - startTime > timeout) {
            return false;
        }
    }
    return true;
}
