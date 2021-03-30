//////////////////////////////////////////////////////////////////////////////////
/// \file mac_receiver.c
/// \brief MAC receiver thread
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "mac.h"


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	uint8_t * qPtr;
	osStatus_t retCode;
	
	//------------------------------------------------------------------------------
	for (;;)											// loop until doomsday
	{
    //----------------------------------------------------------------------------
		//	QUEUE READ										
    //----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_macR_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
		qPtr = queueMsg.anyPtr;
		
		if(qPtr[0] == TOKEN_TAG)
		{
			for(uint8_t i = 0;i<16;i++)
			{
				if(i!=gTokenInterface.myAddress)
				{
					gTokenInterface.station_list[i] = qPtr[i+1];
				}
			}
			qPtr[gTokenInterface.myAddress+1] = gTokenInterface.station_list[gTokenInterface.myAddress];
			
			if(mac_state == Receiving)
			{
				queueMsg.type = TOKEN;
				retCode = osMessageQueuePut( 	
				queue_macS_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever); 	
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
			}
			else if(mac_state == AwaitingTransmission)
			{
				
			}
		}
	}
}


