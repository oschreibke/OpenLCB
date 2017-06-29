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

#include "OpenLCBCDI.h"

#define Printitem(x) {do {Serial.print(F(#x ": ")); Serial.println(x);} while (0);}
#define PrintXML(x) {do {strcpy(buf, x); Serial.print(buf);} while (0);}

void OpenLCBCDI::ShowItemLengths() {
    char buf[64];


    Printitem(strlen(cdiTagStart));
    Printitem(strlen(cdiTagEnd));
    Printitem(strlen(cdiXmlHeader));
    Printitem(strlen(cdiStart));
    Printitem(strlen(cdiStart2));
    Printitem(strlen(cdiStart3));
    Printitem(strlen(cdiTagIdentification1));
    strcpy(buf, "");
    Printitem(strlen(buf));
    Printitem(strlen(cdiTagIdentification2));
    strcpy(buf, "");
    Printitem(strlen(buf));
    Printitem(strlen(cdiTagIdentification3));
    strcpy(buf, "");
    Printitem(strlen(buf));
    Printitem(strlen(cdiTagIdentification4));
    Printitem(strlen(cdiAdci));
    Printitem(strlen(cdiUserInfo1));
    Printitem(strlen(cdiUserInfo2));
    Printitem(strlen(cdiUserInfo3));
    Printitem(strlen(cdiUserInfo4));
    Printitem(strlen(cdiUserInfo5));
    Printitem(strlen(cdiUserInfo6));    
    Printitem(strlen(cdiSegment253));
    Printitem(strlen(cdiConfiguration));
    Printitem(strlen(cdiSegmentEnd));
    Printitem(strlen(cdiEnd));
}

void OpenLCBCDI::AssembleXML(){
	char buf [64];
	
	PrintXML(cdiXmlHeader);
    PrintXML(cdiStart);
    PrintXML(cdiStart2);
    PrintXML(cdiStart3);
    PrintXML(cdiTagIdentification1);
    strcpy(buf, "O Schreibke");
    Serial.print(buf);
    PrintXML(cdiTagIdentification2);
    strcpy(buf, "Test Node");
    Serial.print(buf);
    PrintXML(cdiTagIdentification3);
    strcpy(buf, "0.1");
    Serial.print(buf);
    PrintXML(cdiTagIdentification4);
    PrintXML(cdiAdci);
    PrintXML(cdiUserInfo1);
    PrintXML(cdiUserInfo2);
    PrintXML(cdiUserInfo3);
    PrintXML(cdiUserInfo4);
    PrintXML(cdiUserInfo5);
    PrintXML(cdiUserInfo6);    
    PrintXML(cdiSegment253);
    PrintXML(cdiConfiguration);
    PrintXML(cdiSegmentEnd);
    PrintXML(cdiEnd);	
	}
