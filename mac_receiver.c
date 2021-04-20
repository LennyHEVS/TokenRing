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
	uint8_t destAddress;
	
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
		
		if(gTokenInterface.connected)
		{
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
					PH_DATA_REQUEST(queueMsg);
					
					//send to lcd the token list
					queueMsg_LCD.type = TOKEN_LIST;
					/**send token list on the LCD**/
					LAY_DATA_PUT(queueMsg_LCD,queue_lcd_id);
					mac_state = Receiving;
				}
				else if(mac_state == AwaitingTransmission)
				{
					//if receive token but awaiting -->
					// send it to sender 
					queueMsg.type = TOKEN;
					LAY_DATA_PUT(queueMsg,queue_macS_id);				
				}
			}else if(queueMsg.type == FROM_PHY)
			{
				/*****************************************
				*NOT A TOKEN
				******************************************/
				// if it is not a token
				//an and because qPtr[0] = 0|AAAA|SSS
				if(qPtr[0]>>3 == gTokenInterface.myAddress)
				{
					/*****************************************
					*SRC ADDR = MY ADDR
					******************************************/
					//Frame DATABACK 
					queueMsg.type = DATABACK;
					queueMsg.addr = gTokenInterface.myAddress;
					queueMsg.sapi = qPtr[0] & 7;//0b0000111;
					LAY_DATA_PUT(queueMsg,queue_macS_id);
				}else{
					destAddress = qPtr[1]>>3;
					if((destAddress == gTokenInterface.myAddress) 
						|| (destAddress == BROADCAST_ADDRESS))
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
						if(crc == (qPtr[qPtr[2] + 3]>>2))
						{
							/*****************************************
							*CHECKSUM VALID
							******************************************/
							//an and because qPtr[0] = 0|AAAA|SSS
							uint8_t Sapi = qPtr[0] & 0x7;//0b00000111
							switch (Sapi){
								case CHAT_SAPI :
									//Modify R to 1
									qPtr[qPtr[2] + 3] |= (1 << READ_BIT_STATUS);
									//Set ACK to 1
									qPtr[qPtr[2] + 3] |= (1 << ACK_BIT_STATUS);
									PH_DATA_REQUEST(queueMsg);
								
									queueMsg_IND.addr = qPtr[0]>>3;
									queueMsg_IND.sapi = CHAT_SAPI;
									queueMsg_IND.anyPtr = osMemoryPoolAlloc(memPool,osWaitForever);
									queueMsg_IND.type = DATA_IND;
									memcpy(queueMsg_IND.anyPtr,qPtr+3,qPtr[2]);
									LAY_DATA_PUT(queueMsg_IND,queue_chatR_id);
									break;
								case TIME_SAPI :
									//Modify R to 1
									qPtr[qPtr[2] + 3] |= (1 << READ_BIT_STATUS);
									//Set ACK to 1
									qPtr[qPtr[2] + 3] |= (1 << ACK_BIT_STATUS);
									PH_DATA_REQUEST(queueMsg);
									
									queueMsg_IND.addr = qPtr[0]>>3;
									queueMsg_IND.sapi = TIME_SAPI;
									queueMsg_IND.anyPtr = osMemoryPoolAlloc(memPool,osWaitForever);
									queueMsg_IND.type = DATA_IND;
									memcpy(queueMsg_IND.anyPtr,qPtr+3,qPtr[2]);
									LAY_DATA_PUT(queueMsg_IND,queue_timeR_id);
									break;
								default:
									//Modify R to 0
									qPtr[qPtr[2] + 3] &= ~(1 << READ_BIT_STATUS);
									//Modify ACK to 0
									qPtr[qPtr[2] + 3] &= ~(1 << ACK_BIT_STATUS);
									PH_DATA_REQUEST(queueMsg);
									break;
							}
							
						}else{
							/*****************************************
							* CHECKSUM NOT VALID
							******************************************/
							//Modify R to 1
							qPtr[qPtr[2] + 3] |= (1 << READ_BIT_STATUS);
							//Set ACK to 0
							qPtr[qPtr[2] + 3] &= ~(1 << ACK_BIT_STATUS);
							
							PH_DATA_REQUEST(queueMsg);
						}					
					}else{
						/*****************************************
						*DEST ADDR != MY ADDR
						******************************************/
						PH_DATA_REQUEST(queueMsg);
					}
				}//if qPtr[1]&(gTokenInterface.myAddress << 3) ==(gTokenInterface.myAddress << 3)
			}//if queueMsg.type == FROM_PHY
		}
		else
		{
			PH_DATA_REQUEST(queueMsg);
		}//if connected
	}//for
}
