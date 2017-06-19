/*
 * OpenLCBAliasRegistry.h
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

#ifndef OpenLCBAliasRegistryIncluded
#define OpenLCBAliasRegistryIncluded

#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif

#ifndef MAX_ALIASES        
    #define MAX_ALIASES 48 // the maximum nodes in a segment
#endif

enum NodeAliasStatus: uint8_t {nodeAliasNotFound, CID1received, CID2received, CID3received, CID4received, RIDreceived};

struct NodeAlias{
    uint16_t alias;
    uint64_t nodeId;
    NodeAliasStatus status;
	};

class OpenLCBAliasRegistry {
	// register the alises in this segment
    uint16_t nodes;
    NodeAlias nodeEntry[MAX_ALIASES];
  public:
    OpenLCBAliasRegistry(void);
    bool add(uint16_t alias, uint64_t nodeId, NodeAliasStatus status);
    uint16_t getAlias(uint64_t nodeId);
    uint64_t getNodeId(uint16_t alias);
    bool JMRIRegistered();
    bool remove(uint16_t alias);
    NodeAliasStatus getStatus(uint16_t alias);    
    bool setStatus(uint16_t alias, NodeAliasStatus status);
    bool setNodeId(uint16_t alias, uint64_t newNodeId);
    
  private:
    uint8_t findAlias(uint16_t alias);  
};

#endif
