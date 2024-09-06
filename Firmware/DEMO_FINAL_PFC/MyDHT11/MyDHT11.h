/**
 * @file MyDHT11.h
 * @brief Custom library for the DHT11 temperature and humidity sensor.
 * 
 * This library provides an interface to read temperature and humidity data from a DHT11 sensor.
 * 
 * @class MyDHT11
 * @brief A class to interface with a DHT11 sensor for temperature and humidity readings.
 * 
 * This class allows you to read temperature and humidity values from the DHT11 sensor.
 */

#ifndef MYDHT11_H
#define MYDHT11_H

#include "Arduino.h"

class MyDHT11 {
public:
    /**
     * @brief Constructor for MyDHT11.
     * 
     * Initializes the DHT11 sensor with the specified pin.
     * 
     * @param pin The GPIO pin where the DHT11 sensor is connected.
     */
    MyDHT11(uint8_t pin);

    /**
     * @brief Reads data from the DHT11 sensor.
     * 
     * Initiates communication with the DHT11 sensor and retrieves temperature and humidity data.
     * 
     * @return bool True if the reading was successful, false otherwise.
     */
    bool read();

    /**
     * @brief Gets the last temperature reading.
     * 
     * @return int The last successful temperature reading from the sensor.
     */
    int getTemperature() const;

    /**
     * @brief Gets the last humidity reading.
     * 
     * @return int The last successful humidity reading from the sensor.
     */
    int getHumidity() const;

private:
    uint8_t _pin;         ///< The GPIO pin number where the DHT11 sensor is connected.
    int _temperature;     ///< The last successful temperature reading from the sensor.
    int _humidity;        ///< The last successful humidity reading from the sensor.

    /**
     * @brief Waits for a specific state on the data line.
     * 
     * Waits until the DHT11 sensor sends the expected state (HIGH or LOW) on the data line or until a timeout occurs.
     * 
     * @param state The expected state (HIGH or LOW) to wait for.
     * @param timeout The maximum time to wait for the state in microseconds.
     * @return bool True if the state was detected before the timeout, false otherwise.
     */
    bool waitForState(uint8_t state, unsigned long timeout);
};

#endif
