/*
 * Nano_OpenLCB_Node.ino
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
 
#include "OpenLCBNode.h"

// the maximum nodes in a segment
#define MAX_ALIASES 48
//#include "OpenLCBAliasRegistry.h"
#include "OpenLCBCANInterface.h"

#include <mcp_can.h>
#define CAN0_INT 2                              // Set INT to pin 2
#define CAN0_CS  10                             // Set CS to pin 10


OpenLCBNode node;
MCP_CAN CAN0(CAN0_CS);
OpenLCBCANInterface canInt;

int looplimit = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(F("Starting..."));

//  node.setNodeId();
  
  // init the can interface
  canInt.begin(&CAN0, CAN0_INT);

  if (canInt.getInitialised()){
	  node.setCanInt(&canInt);

      Serial.println(F("Initialisation complete"));
   } else {
      //while(1){};  // I've fallen over and can't get up (Todo: something intelligent here)
      Serial.println(F("Can initialisation failed"));
  }
}

void loop() {
	if (canInt.getInitialised())
	//    if (looplimit++ < 100)
	        node.loop();
}

