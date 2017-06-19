/*
 * OpenLCBCANInterface.h
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

#ifndef OpenLCBCANInterfaceIncluded
#define OpenLCBCANInterfaceIncluded
#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif

#include <mcp_can.h>
#include "OpenLCBMessage.h"


class OpenLCBCANInterface {
    MCP_CAN * can;
    bool canInitialised;
  public:
    OpenLCBCANInterface();
    void begin(MCP_CAN * mcpCanObj, uint8_t intPin);
    bool getInitialised();
    bool receiveMessage(OpenLCBMessage* msg);
    bool sendMessage(OpenLCBMessage* msg);
};



#endif
