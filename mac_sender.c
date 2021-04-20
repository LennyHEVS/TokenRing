//////////////////////////////////////////////////////////////////////////////////
/// \file mac_sender.c
/// \brief MAC sender thread
/// \author Pascal Sartoretti (pascal dot sartoretti at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "mac.h"


//////////////////////////////////////////////////////////////////////////////////
// MAC ERROR INDICATION
//////////////////////////////////////////////////////////////////////////////////
void MAC_ERROR_INDICATION(struct queueMsg_t msg)
{
	osStatus_t retCode;
	
	msg.type = MAC_ERROR;
	sprintf((char*)(msg.anyPtr),"Destination address %d does not respond\0",(((uint8_t*) msg.anyPtr)[1] >> 3) + 1);
	
	retCode = osMessageQueuePut( 	
					queue_lcd_id,
					&msg,
					osPriorityNormal,
					osWaitForever); 	
	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
}
//////////////////////////////////////////////////////////////////////////////////
// MAC DATA REQUEST
//////////////////////////////////////////////////////////////////////////////////
void MAC_DATA_REQUEST(struct queueMsg_t msg)
{
	uint8_t * qPtr;
	uint32_t msgLength;
	uint32_t checksum = 0;
	
	osStatus_t retCode;
	
	if(bufferNb < BUFFER_SIZE)
	{
		bufferNb++;
		
		msgLength = strlen(msg.anyPtr);
		
		qPtr = osMemoryPoolAlloc(memPool,osWaitForever);
		qPtr[0] = (gTokenInterface.myAddress<<3) + msg.sapi;
		qPtr[1] = (msg.addr<<3) + msg.sapi;
		qPtr[2] = msgLength;
		
		memcpy(qPtr+3,msg.anyPtr,msgLength);
		for(uint32_t i=0;i<msgLength+3;i++)
		{
			checksum += qPtr[i];
		}
		
		qPtr[3+msgLength] = checksum<<2;
		
		buffer[bufferNb-1] = qPtr;
	}
	
	mac_state = AwaitingTransmission;
	
	//----------------------------------------------------------------------------
	// MEMORY RELEASE	(received data : mac layer style)
	//----------------------------------------------------------------------------
	retCode = osMemoryPoolFree(memPool,msg.anyPtr);
	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
}
//////////////////////////////////////////////////////////////////////////////////
// MAC START REQUEST
//////////////////////////////////////////////////////////////////////////////////
void MAC_START_REQUEST(uint8_t SAPI)
{
	gTokenInterface.station_list[gTokenInterface.myAddress] |= (1 << SAPI);
}
//////////////////////////////////////////////////////////////////////////////////
// MAC STOP REQUEST
//////////////////////////////////////////////////////////////////////////////////
void MAC_STOP_REQUEST(uint8_t SAPI)
{
	gTokenInterface.station_list[gTokenInterface.myAddress] &= ~(1 << SAPI);
}
//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	uint8_t * qPtr;
	
	uint8_t * sentFrame;	//last data packet sent
	
	uint32_t msgLength;
	
	osStatus_t retCode;
	
	//------------------------------------------------------------------------------
	for (;;)											// loop until doomsday
	{
    //----------------------------------------------------------------------------
		//	QUEUE READ										
    //----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_macS_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
		switch(queueMsg.type)
		{
			case DATA_IND:
				if(gTokenInterface.connected)
				{
					MAC_DATA_REQUEST(queueMsg);
				}
				else
				{
					//----------------------------------------------------------------------------
					// MEMORY RELEASE	(received token : mac layer style)
					//----------------------------------------------------------------------------
					retCode = osMemoryPoolFree(memPool,queueMsg.anyPtr);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				}
				break;
			case TOKEN:
				
				//----------------------------------------------------------------------------
				// MEMORY RELEASE	(received token : mac layer style)
				//----------------------------------------------------------------------------
				retCode = osMemoryPoolFree(memPool,queueMsg.anyPtr);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
			
				//Send data
				sentFrame = osMemoryPoolAlloc(memPool,osWaitForever);
			
				queueMsg.type = TO_PHY;
				queueMsg.anyPtr = buffer[bufferNb-1];
				memcpy(sentFrame,buffer[bufferNb-1],buffer[bufferNb-1][2] + 4);
				PH_DATA_REQUEST(queueMsg);
				bufferNb--;
				break;
			case NEW_TOKEN:
				gTokenInterface.station_list[gTokenInterface.myAddress] = (1 << CHAT_SAPI) + (1 << TIME_SAPI);
				MAC_NEW_TOKEN();
				break;
			case DATABACK:
				qPtr = queueMsg.anyPtr;
				if((qPtr[1]>>3) == BROADCAST_ADDRESS)
				{
					//----------------------------------------------------------------------------
					// MEMORY RELEASE	(received frame : mac layer style)
					//----------------------------------------------------------------------------
					retCode = osMemoryPoolFree(memPool,queueMsg.anyPtr);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
					
					//----------------------------------------------------------------------------
					// MEMORY RELEASE	(sent frame : mac layer style)
					//----------------------------------------------------------------------------
					retCode = osMemoryPoolFree(memPool,sentFrame);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
					
					MAC_NEW_TOKEN();
				}
				else
				{
					msgLength = qPtr[2];
					if((qPtr[3+msgLength] & 0x02) == 0x02) //Check Read
					{
						if((qPtr[3+msgLength] & 0x01) == 0x01)	//Check Ack
						{
							//----------------------------------------------------------------------------
							// MEMORY RELEASE	(received frame : mac layer style)
							//----------------------------------------------------------------------------
							retCode = osMemoryPoolFree(memPool,queueMsg.anyPtr);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
							
							//----------------------------------------------------------------------------
							// MEMORY RELEASE	(sent frame : mac layer style)
							//----------------------------------------------------------------------------
							retCode = osMemoryPoolFree(memPool,sentFrame);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
							
							MAC_NEW_TOKEN();
						}
						else
						{
							//Send data back
							qPtr = osMemoryPoolAlloc(memPool,osWaitForever);
							queueMsg.type = TO_PHY;
							queueMsg.anyPtr = qPtr;
							memcpy(qPtr,sentFrame,sentFrame[2] + 4);
							PH_DATA_REQUEST(queueMsg);
							break;
						}
					}
					else
					{
						MAC_ERROR_INDICATION(queueMsg);
						MAC_NEW_TOKEN();
					}
				}
				if(bufferNb == 0)	//buffer empty ?
				{
					mac_state = Receiving;
				}
				else
				{
					mac_state = AwaitingTransmission;
				}
				break;
			case START:
				MAC_START_REQUEST(CHAT_SAPI);
				break;
			case STOP:
				MAC_STOP_REQUEST(CHAT_SAPI);
				break;
			default:break;
		}
	}
}


