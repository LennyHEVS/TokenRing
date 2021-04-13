#ifndef __MAC_H
#define __MAC_H
#include <stdint.h>
#include "stm32f7xx_hal.h"
#include "main.h"

#define BUFFER_SIZE 4

enum MacStates {Receiving, AwaitingTransmission};
extern enum MacStates mac_state;
extern uint8_t* buffer[BUFFER_SIZE];
extern uint32_t bufferNb;

void PH_DATA_REQUEST(struct queueMsg_t msg);
void LAY_DATA_PUT(struct queueMsg_t msg,osMessageQueueId_t queue_to_send);
void MAC_NEW_TOKEN(void);
#endif //__MAC_H
