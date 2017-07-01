/*
 * OpenLCBCDI.h
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


#ifndef OpenLCBCDIIncluded
#define OpenLCBCDIIncluded
#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif
#endif


//  Strings (max length = 64 (datagram)
//                                      0        1         2         3         4         5         6
//                                      1234567890123456789012345678901234567890123456789012345678901234
/*
const  char cdiXmlHeader[] =          {"<?xml version=\"1.0\"?>\n"};
const  char cdiStart[] =              {"<cdi xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""};
const  char cdiStart2[] =             {" xsi:noNamespaceSchemaLocation="};
const  char cdiStart3[] =             {"\"http://openlcb.org/schema/cdi/1/1/cdi.xsd\">\n"};
const  char cdiTagIdentification1[] = {"<identification><manufacturer>"};
const  char cdiTagIdentification2[] = {"</manufacturer><model>"};
const  char cdiTagIdentification3[] = {"</model><hardwareVersion>"};
const  char cdiTagIdentification4[] = {"</hardwareVersion></identification>\n"};
const  char cdiAdci[] =               {"<acdi/>\n"};
//space 251 (0xFB) = User Info
const  char cdiUserInfo1[] =          {"<segment space=\"251\"><name>User Identification</name>"};
const  char cdiUserInfo2[] =          {"<description>Lets the user add his own description</description>"};
const  char cdiUserInfo3[] =          {"<string size=\"21\"><name>Version</name></string>"};
const  char cdiUserInfo4[] =          {"<string size=\"63\"><name>Node Name</name></string>"}; 
const  char cdiUserInfo5[] =          {"<string size=\"64\"><name>Node Description</name></string>"};
const  char cdiUserInfo6[] =          {"</segment>\n"}; 
// space 253 (0xFD) = configuration  
const  char cdiSegment253[] =         {"<segment space=\"253\">"};
const  char cdiConfiguration[] =      {""};  
const  char cdiSegmentEnd[] =         {"</segment>\n"};
const  char cdiEnd[] =                {"</cdi>\n\0"};
*/
const char Manufacturer[] = "O Schreibke";
const char ModelName[] = "Test Node";
const char HardwareVersion[] = "0.1";
const char SoftwareVersion[] = "0.1";
const char UserName[] = "my first Node";
const char UserDescription[] = "first node for cdi"; 


const  char cdiXml[] = "<?xml version=\"1.0\"?>\n"
                      "<cdi xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://openlcb.org/schema/cdi/1/1/cdi.xsd\">\n"
                      "<identification><manufacturer>O Schreibke</manufacturer><model>Test Node</model><hardwareVersion>0.1</hardwareVersion><softwareVersion>0.1</softwareVersion></identification>\n"
                      "<acdi/>\n"
                      //space 251 (0xFB) = User Info Description
                      "<segment space=\"251\"><name>User Identification</name>"
                      "<description>Lets the user add his own description</description>"
                      "<string size=\"63\"><name>Node Name</name></string>" 
                      "<string size=\"64\"><name>Node Description</name></string>"
                      "</segment>\n" 
                      // space 253 (0xFD) = configuration  
                      "<segment space=\"253\">"
                      "<group replication=\"20\">"
                      "<name>channels</name><description>Each channel is one entry in the event table.</description><repname>Channel</repname>"
                      "<group><name>Decoder</name>"
                      "<int size=\"1\"><name>Decoder type</name><default>1</default>"
                      "<map>"
                      "<relation><property>1</property><value>I2C output</value></relation>"
                      "<relation><property>2</property><value>Pin Output</value></relation>"
                      "<relation><property>3</property><value>I2C Sensor</value></relation>"
                      "<relation><property>4</property><value>Pin Sensor</value></relation>"
                      "</map></int>"
                      "<int size=\"1\"><name>I2C Address</name><min>8</min><max>127</max></int>"
                      "<int size=\"1\"><name>Command</name><min>0</min><max>255</max></int>"
                      "</group>"
                      "</group>"
                      "</segment>\n"
                      "</cdi>\n\0";


// dynamic information

class OpenLCBCDI{
	public:
	void ShowItemLengths();
//	void AssembleXML();
	};
