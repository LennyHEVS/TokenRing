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
	struct queueMsg_t queueMsg_IND;		// queue message for the LCD
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
			
			if(mac_state == Receiving)
			{
				//Update Lcd and send the TOKEN to next station
				queueMsg.type = TO_PHY;
				queue_to_send = queue_phyS_id;
				LAY_DATA_PUT(queueMsg,queue_to_send);
				
				//send to lcd the token list
				queueMsg_LCD.type = TOKEN_LIST;
				/**send token list on the LCD**/
				retCode = LAY_DATA_PUT(queueMsg_LCD,queue_lcd_id);					
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				mac_state = Receiving;				
			}
			else if(mac_state == AwaitingTransmission)
			{
				//if receive token but awaiting -->
				// send it to sender 
				queueMsg.type = TOKEN;
				queue_to_send = queue_macS_id;	
				LAY_DATA_PUT(queueMsg,queue_to_send);				
			}
		}else if(queueMsg.type == FROM_PHY)
		{
			/*****************************************
			*NOT A TOKEN
			******************************************/
			// if it is not a token
			//an and because qPtr[0] = 0|AAAA|SSS
			if(qPtr[0]>>3 == gTokenInterface.myAddress )
			{
				/*****************************************
				*SRC ADDR = MY ADDR
				******************************************/
				//Frame DATABACK 
				queueMsg.type = DATABACK;
				queueMsg.addr = gTokenInterface.myAddress;
				queueMsg.sapi = qPtr[0] & 7;//0b0000111;
				queue_to_send = queue_macS_id;
				LAY_DATA_PUT(queueMsg,queue_to_send);
			}else{
				if(qPtr[1]>>3 == gTokenInterface.myAddress)
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
					if(crc == ((qPtr[qPtr[2] + 3]>>2) & 0x3F))
					{
						/*****************************************
						*CHECKSUM VALID
						******************************************/
						//Modify R to 1
						qPtr[qPtr[2] + 3] |= (1 << READ_BIT_STATUS) ;
						//Set ACK to 1
						qPtr[qPtr[2] + 3] |= (1 << ACK_BIT_STATUS);	
						PH_DATA_REQUEST(queueMsg);
						//an and because qPtr[0] = 0|AAAA|SSS
						uint8_t Sapi = qPtr[0] & 0x8;//0b00000111
						switch (Sapi){
							case CHAT_SAPI :
								queueMsg_IND.anyPtr = osMemoryPoolAlloc(memPool,osWaitForever);
								queueMsg_IND.type = DATA_IND;
								memcpy(queueMsg_IND.anyPtr,qPtr+3,qPtr[2]);
								LAY_DATA_PUT(queueMsg_IND,queue_chatS_id);
								break;
							
							case TIME_SAPI :
								queueMsg_IND.anyPtr = osMemoryPoolAlloc(memPool,osWaitForever);
								queueMsg_IND.type = DATA_IND;
								memcpy(queueMsg_IND.anyPtr,qPtr+3,qPtr[2]);
								LAY_DATA_PUT(queueMsg_IND,queue_timeS_id);
								break;
						}
						
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
						LAY_DATA_PUT(queueMsg,queue_to_send);	
					}					
				}else{
					/*****************************************
					*DEST ADDR != MY ADDR
					******************************************/
					queueMsg.type = TO_PHY;		
					queue_to_send = queue_phyS_id;
					LAY_DATA_PUT(queueMsg,queue_to_send);	
				}
								
			}//if qPtr[1]&(gTokenInterface.myAddress << 3) ==(gTokenInterface.myAddress << 3)
		}//if queueMsg.type == FROM_PHY
	}//for
}
