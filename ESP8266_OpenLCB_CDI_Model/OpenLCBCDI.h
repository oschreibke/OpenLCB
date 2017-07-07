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

// the fixed information about the node. Usually in PROGMEM.
// This is for SNI, repeated below in the CDI. There is potential for reuse here.
const char Manufacturer[] = "O Schreibke";
const char ModelName[] = "Test Node";
const char HardwareVersion[] = "0.1";
const char SoftwareVersion[] = "0.1";

// The Configuration Description Information. Again in PROGMEM.
// Describes the elements which make up both the configuration information itself and how it is presented in the JMRI configuration panel.
// Does NOT contain any variable data (config values, etc).
// The easiest way to visualise is to use the model and call up the configuration panel in JMRI.
// See also S-9.7.4.1

// According to S-9.7.4.1, the identification section is optional. I didn't try to see if JMRI would use the SNI information to populate this information in the panel.

// Describes Segments 0xFB and 0FD
// Segment 0xFB contains two elements: the user name and user description.
// Segment 0xFD contains a repeating group of 20 entries, which is about all I can fit in the 1K EEPROM - See the notes about padding.
//              Each event entry contains five elements:
//                    - the event id associated with the entry. I don't think it has to be unique, but didn't try it.
//                      Repeating an event identifier would for example enable raising and lowering a barrier and also setting the signal protecting it. 
//                    - A user description for the event.
//                    - The handler type: I've defined both a physical Arduino pin and an I2C device.
//                    - The pin number or I2C address
//                    - The value to be sent 0/1 for pins or the command to send to an i2C device (for example setting a servo to a specific angle)
//                      In case if an input pin or device, receipt of the value would cause the corresponding event id to be generated

const  char cdiXml[] = "<?xml version=\"1.0\"?>\n"
                      "<cdi xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://openlcb.org/schema/cdi/1/1/cdi.xsd\">\n"
						  "<identification>"
							  "<manufacturer>O Schreibke</manufacturer>"
							  "<model>Test Node</model>"
							  "<hardwareVersion>0.1</hardwareVersion>"
							  "<softwareVersion>0.1</softwareVersion>"
						  "</identification>\n"
						  "<acdi/>\n"
						  //space 251 (0xFB) = User Info Description
						  "<segment space=\"251\">"
						      "<name>User Identification</name>"
							  "<description>Lets the user add his own description</description>"
							  "<string size=\"63\"><name>Node Name</name></string>" 
							  "<string size=\"64\"><name>Node Description</name></string>"
						  "</segment>\n" 
						  // space 253 (0xFD) = configuration  
						  "<segment space=\"253\">"
							  "<group replication=\"20\">"
								  "<name>Events</name>"
								  "<description>Each tab is one entry in the event table.</description>"
								  "<repname>Event</repname>"
								  "<group>"
								      "<name>Event</name>"
									  "<eventid><name>I2C or pin command</name>"
										  "<description>When this event arrives, command will be sent to the selected i2c device or pin.</description>"
									  "</eventid>"
									  "<string size=\"37\"><name>Description</name></string>"
									  "<int size=\"1\"><name>Decoder type</name><default>1</default>"
										  "<map>"
											  "<relation><property>1</property><value>I2C output</value></relation>"
											  "<relation><property>2</property><value>I2C input</value></relation>"
											  "<relation><property>3</property><value>Pin output</value></relation>"
											  "<relation><property>4</property><value>Pin input</value></relation>"                      
										  "</map>"
									 "</int>"
									 "<int size=\"1\"><name>I2C Address (8-127) or pin number (D0-D10)</name><min>0</min><max>127</max></int>"
									 "<int size=\"1\"><name>Command</name><min>0</min><max>255</max></int>"
								  "</group>"
							  "</group>"
						  "</segment>\n"
                      "</cdi>\n\0";

class OpenLCBCDI{
	public:
	void ShowItemLengths();
//	void AssembleXML();
	};
