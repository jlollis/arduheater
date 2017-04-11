/**
 * Arduheater - Heat controller for astronomy usage
 * Copyright (C) 2016-2017 João Brázio [joao@brazio.org]
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "arduheater.h"

dht22::dht22()
  : sensor(DHT22_WARMUP_TIME, DHT22_SLEEP_TIME, DHT22_REFRESH_TIME) {;}

bool dht22::hwupdate() {
  m_state = SENSOR_BUSY;

  // send the start signal and switch into receive mode
  pinMode(AMBIENT_PIN, OUTPUT);
  digitalWrite(AMBIENT_PIN, LOW);

  utils::delay(1);

  digitalWrite(AMBIENT_PIN, HIGH);
  pinMode(AMBIENT_PIN, INPUT);

  // TODO: stop using pulsein
  if (! pulseIn(AMBIENT_PIN, LOW, 115)) {
    // return timeout if no data is received
    sys.sensor &= ~AMBIENT_SENSOR_READY;
    m_state = SENSOR_TIMEOUT;
    return false;
  }

  uint16_t d = 0, h = 0, t = 0;

  // process the incoming data
  for(uint8_t i = 0; i < 40; i++) {
    d <<= 1;

    // TODO: stop using pulsein
    if (pulseIn(AMBIENT_PIN, HIGH, 200) > 30) { d |= 1; }

    switch (i) {
      case 15: h = d;
        break;

      case 31: t = d;
        d = 0;
    }
  }

  // checksum validation
  if ((byte) (((byte) h) + (h >> 8) + ((byte) t) + (t >> 8)) != d) {
    sys.sensor &= ~AMBIENT_SENSOR_READY;
    m_state = SENSOR_ERROR;
    return false;
  }
/*
  // cache the received humidity data using a low pass filter
  const float oldh = m_cache[1]();
  m_cache[1] += 0.2F * oldh + 0.8 * (((int16_t) h) * 0.1);

  if (t & 0x8000) { t = -((int16_t) (t & 0x7FFF)); }

  // cache the received temperature data using a low pass filter
  const float oldt = m_cache[0]();
  m_cache[0] += 0.2F * oldt + 0.8 * (((int16_t) t) * 0.1);
*/

  // cache the received humidity data
  m_cache[1] += ((int16_t) h) * 0.1;

  if (t & 0x8000) { t = -((int16_t) (t & 0x7FFF)); }

  // cache the received temperature data using a low pass filter
  m_cache[0] += ((int16_t) t) * 0.1;

  // update the global status for the ambient sensor
  if (t && h) { sys.sensor |= AMBIENT_SENSOR_READY; }
  else { sys.sensor &= ~AMBIENT_SENSOR_READY; }

  return true;
}
