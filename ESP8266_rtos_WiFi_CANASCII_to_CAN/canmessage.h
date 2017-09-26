#ifndef CANMESSAGE_H
#define CANMESSAGE_H

struct CAN_MESSAGE{
    INT32U id; 
    INT8U  ext;
    INT8U  len; 
    char   dataBytes[8];
};

#endif
