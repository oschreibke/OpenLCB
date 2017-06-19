/*
 * util.cpp
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

/*
 * Static class to expand the printing facilitits of the Arduino systen
 */
 
#include "util.h"

// print a 64-bit integer in Hex 
void util::print64BitHex(uint64_t val) {

    for (int8_t i = 7; i >= 0; i--) {
        util::print8BitHex((uint8_t) (val >> (8*i)) & 0xFFULL);
    }
}

// Print an eight bit integer as two Hex digits  
void util::print8BitHex(uint8_t val) {

    if (val < 0x10) {
        Serial.print(F("0"));
    }
    Serial.print(val, HEX);
}
