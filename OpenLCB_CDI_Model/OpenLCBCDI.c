/*
 * OpenLCBCDI.cpp
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

#include "string.h"
#include "OpenLCBCDI.h"

// #define Printitem(x) {do {printf(#x ": "); printf(x); printf("\n");} while (0);}
// #define PrintXML(x) {do {strcpy(buf, x); printf(buf);} while (0);}

extern const  char cdiXml[];

void ShowCdiXmlLength() {
	// Printitem(strlen(cdiXml));
	printf("strlen(cdiXml): %d \n", strlen(cdiXml));
}
