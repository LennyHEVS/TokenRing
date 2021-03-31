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
// PHYSICAL DATA REQUEST
//////////////////////////////////////////////////////////////////////////////////
void PH_DATA_REQUEST(struct queueMsg_t msg)
{
	osStatus_t retCode;
	
	retCode = osMessageQueuePut( 	
					queue_phyS_id,
					&msg,
					osPriorityNormal,
					osWaitForever); 	
	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
}
//////////////////////////////////////////////////////////////////////////////////
// MAC ERROR INDICATION
//////////////////////////////////////////////////////////////////////////////////
void MAC_ERROR_INDICATION(struct queueMsg_t msg)
{
	osStatus_t retCode;
	
	msg.type = MAC_ERROR;
	retCode = osMessageQueuePut( 	
					queue_lcd_id,
					&msg,
					osPriorityNormal,
					osWaitForever); 	
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
// MAC NEW TOKEN
//////////////////////////////////////////////////////////////////////////////////
void MAC_NEW_TOKEN()
{
	struct queueMsg_t msg;		// queue message
	uint8_t * qPtr;
	
	msg.type = TOKEN;
				
	qPtr = osMemoryPoolAlloc(memPool,osWaitForever);
	qPtr[0] = TOKEN_TAG;
	gTokenInterface.station_list[gTokenInterface.myAddress] = (1 << CHAT_SAPI) + (1 << TIME_SAPI);
	for(uint8_t i = 0;i<16;i++)
	{
		qPtr[i+1] = gTokenInterface.station_list[i];
	}
	msg.anyPtr = qPtr;
	PH_DATA_REQUEST(msg);
}
//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	struct queueMsg_t queueMsg_LCD;		// queue message for the LCD
	uint8_t * sentData;	//last data packet sent
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
		switch(mac_state)
		{
			case Receiving:
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
						MAC_NEW_TOKEN();
						break;
					case DATABACK:
						
						break;
					case START:
						MAC_START_REQUEST(CHAT_SAPI);
						break;
					case STOP:
						MAC_STOP_REQUEST(CHAT_SAPI);
						break;
					default:break;
				}
				break;
			case AwaitingTransmission:
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
						MAC_NEW_TOKEN();
						break;
					case DATABACK:
						
						break;
					case START:
						MAC_START_REQUEST(CHAT_SAPI);
						break;
					case STOP:
						MAC_STOP_REQUEST(CHAT_SAPI);
						break;
					default:break;
				}
				break;
		}
	}
}


