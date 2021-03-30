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
#include "ext_led.h"


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	struct queueMsg_t queueMsg_LCD;		// queue message
	void* buffer[4];		// message buffer
	uint32_t bufferPtr;
	uint8_t * qPtr;
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
				
				break;
			case TOKEN:
				queueMsg.type = TO_PHY;
				retCode = osMessageQueuePut( 	
				queue_phyS_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever); 	
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				
				queueMsg_LCD.type = TOKEN_LIST;
				retCode = osMessageQueuePut( 	
				queue_lcd_id,
				&queueMsg_LCD,
				osPriorityNormal,
				osWaitForever); 	
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				break;
			case NEW_TOKEN:
				if(mac_state == Receiving)
				{
					queueMsg.type = TOKEN;
				
					qPtr = osMemoryPoolAlloc(memPool,osWaitForever);
					qPtr[0] = TOKEN_TAG;
					for(uint8_t i = 0;i<16;i++)
					{
						qPtr[i+1] = gTokenInterface.station_list[i];
					}
					queueMsg.anyPtr = qPtr;
					retCode = osMessageQueuePut( 	
					queue_phyS_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever); 	
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				}
				break;
			case DATABACK:
				
				break;
			case START:
				gTokenInterface.station_list[gTokenInterface.myAddress] |= (1 << CHAT_SAPI) + (1 << TIME_SAPI);
				break;
			case STOP:
				gTokenInterface.station_list[gTokenInterface.myAddress] &= ~(1 << CHAT_SAPI);
				break;
			default:break;
		}
	}
}


