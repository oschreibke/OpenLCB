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
