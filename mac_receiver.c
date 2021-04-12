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
	struct queueMsg_t queueMsg_LCD;		// queue message for the LCD
<<<<<<< HEAD
=======
	
>>>>>>> dev-lenny
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
		osMessageQueueId_t queue_to_send = 0;
		
		if(qPtr[0] == TOKEN_TAG)
		{
			/*****************************************
			*IT IS A TOKEN
			******************************************/
			for(uint8_t i = 0;i<16;i++)
			{
				if(i!=gTokenInterface.myAddress)
				{
					gTokenInterface.station_list[i] = qPtr[i+1];
				}
			}
			qPtr[gTokenInterface.myAddress+1] = gTokenInterface.station_list[gTokenInterface.myAddress];
			
			queueMsg_LCD.type = TOKEN_LIST;
			retCode = osMessageQueuePut( 	
			queue_lcd_id,
			&queueMsg_LCD,
			osPriorityNormal,
			osWaitForever); 	
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
			
			
			if(mac_state == AwaitingTransmission)
			{
				//Update Lcd and send the TOKEN to next station
				queueMsg.type = TO_PHY;
				queue_to_send = queue_phyS_id;
				
				//send to lcd the token list
				queueMsg_LCD.type = TOKEN_LIST;
				retCode = osMessageQueuePut( 	
				queue_lcd_id,
				&queueMsg_LCD,
				osPriorityNormal,
				osWaitForever); 	
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				mac_state = Receiving;				
			}
			else
			{
<<<<<<< HEAD
				//if receive token but awaiting -->
				// send it to sender 
				queueMsg.type = TOKEN;
				queue_to_send = queue_macS_id;				
=======
				//----------------------------------------------------------------------------
				// MEMORY RELEASE	(received token : mac layer style)
				//----------------------------------------------------------------------------
				retCode = osMemoryPoolFree(memPool,queueMsg.anyPtr);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				
				MAC_NEW_TOKEN();
>>>>>>> dev-lenny
			}
			
			/**SEND THE MESSAGE **/
			retCode = osMessageQueuePut( 	
			queue_to_send,
			&queueMsg,
			osPriorityNormal,
			osWaitForever); 	
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
		}else if(queueMsg.type == FROM_PHY)
		{
			/*****************************************
			*NOT A TOKEN
			******************************************/
			// if it is not a token
			//an and because qPtr[0] = 0|AAAA|SSS
			if(qPtr[0]&(gTokenInterface.myAddress << 3) == (gTokenInterface.myAddress << 3))
			{
				/*****************************************
				*SRC ADDR = MY ADDR
				******************************************/
				//Frame DATABACK 
				queueMsg.type = DATABACK;
				queueMsg.addr = gTokenInterface.myAddress;
				queueMsg.sapi = qPtr[0] & 7;//0b0000111;
				queue_to_send = queue_macS_id;
				
				retCode = osMessageQueuePut( 	
				queue_macS_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever); 	
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);					
			}else{
				if(qPtr[1]&(gTokenInterface.myAddress << 3) ==(gTokenInterface.myAddress << 3))
				{
					/*****************************************
					*DEST ADDR = MY ADDR
					******************************************/
					uint32_t crc = qPtr[0] + qPtr[1] + qPtr[2];
					for(uint8_t i = 0;i< qPtr[2] ;i++)
					{
						crc += qPtr[i+3];
					}
					//We keep only the 6-LSB
					crc &= 0x3F;//0b111111;
					if(crc == (qPtr[qPtr[2] + 3] & 0x3F))
					{
						/*****************************************
						*CHECKSUM VALID
						******************************************/
						
					}else{
						/*****************************************
						* CHECKSUM NOT VALID
						******************************************/
						//Modify R to 1
						qPtr[qPtr[2] + 3] |= (1 << READ_BIT_STATUS) ;
						//Set ACK to 0
						qPtr[qPtr[2] + 3] &= ~(1 << ACK_BIT_STATUS);
						
						queueMsg.type = TO_PHY;
						queue_to_send = queue_phyS_id;
					}					
				}else{
					/*****************************************
					*DEST ADDR != MY ADDR
					******************************************/
					queueMsg.type = TO_PHY;		
					queue_to_send = queue_phyS_id;
				}
				/**SEND THE MESSAGE **/
				retCode = osMessageQueuePut( 	
				queue_to_send,
				&queueMsg,
				osPriorityNormal,
				osWaitForever); 	
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);			
			}//if qPtr[1]&(gTokenInterface.myAddress << 3) ==(gTokenInterface.myAddress << 3)
		}//if queueMsg.type == FROM_PHY
	}//for
}


