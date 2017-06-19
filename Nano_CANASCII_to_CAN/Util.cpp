/*
 * Util.cpp
 * 
 * Copyright 2017 Otto Schreibke <oschreibke@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * The license can be found at https://www.gnu.org/licenses/gpl-3.0.txt
 * 
 * 
 */
// Utility functions

#include "Util.h"

const char hexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

uint8_t Hex2Int(char ch) {
  // convert the input character from a Hex value to Int
  // if the input is not a valid hex character (0..9A..F), 16 will be returned and is an error value

  uint8_t i = 0;
  for (i = 0; i <= 15; i++) {
    if (ch == hexDigits[i]) break;
  }
  //Serial.println(); Serial.print("in Hex2Int. ch = "); Serial.print(ch); Serial.print(", i = "); Serial.println(i);
  return i;
}

char Nybble2Hex(uint8_t nybble) {
	// convert the input nybble to a hex character representation (0..9A..F)
	// if the input is out of range (0..15) return Null (\0)
  if (nybble >= 0 && nybble <= 15)
    return hexDigits[nybble];
  else
    return '\0';
}
