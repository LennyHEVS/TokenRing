#include "mac.h"
enum MacStates mac_state = Receiving;
uint8_t* buffer[BUFFER_SIZE];
uint32_t bufferNb;

//////////////////////////////////////////////////////////////////////////////////
// PHYSICAL DATA REQUEST
//////////////////////////////////////////////////////////////////////////////////
void PH_DATA_REQUEST(struct queueMsg_t msg)
{
	osStatus_t retCode;
	
	msg.type = TO_PHY;
	
	retCode = osMessageQueuePut( 	
					queue_phyS_id,
					&msg,
					osPriorityNormal,
					osWaitForever); 	
	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
}
//////////////////////////////////////////////////////////////////////////////////
// MAC NEW TOKEN
//////////////////////////////////////////////////////////////////////////////////
void MAC_NEW_TOKEN()
{
	struct queueMsg_t msg;		// queue message
	uint8_t * qPtr;
	
	msg.type = TO_PHY;
				
	qPtr = osMemoryPoolAlloc(memPool,osWaitForever);
	qPtr[0] = TOKEN_TAG;
	for(uint8_t i = 0;i<16;i++)
	{
		qPtr[i+1] = gTokenInterface.station_list[i];
	}
	msg.anyPtr = qPtr;
	PH_DATA_REQUEST(msg);
}
