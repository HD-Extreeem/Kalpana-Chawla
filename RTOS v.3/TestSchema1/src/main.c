/**
* \file
*
*  TaskTest
*
* Author: D�sir�e J�nsson och Jonas Eiselt 2017-04-20
*
*/

#include <asf.h>
<<<<<<< HEAD
=======
#include <inttypes.h>
>>>>>>> master
#include "Task_Move.h"
#include "Task_GetCordinates.h"
#include "Task_UnoComm.h"
#include "consoleFunctions.h"
#include "StepCounter_ISR.h"
#include "PwmFunctions.h"
<<<<<<< HEAD

=======
#include "conf_board.h"
#include "comm/TWIComm.h"
#include "arlo/Arlo.h"

 xTaskHandle xTaskMove=NULL;
 xTaskHandle xTaskCom = NULL;
 xTaskHandle xTaskCoordinate=NULL;
>>>>>>> master

int main (void)
{
	sysclk_init();
	board_init();
	configureConsole();
	attach_interupt();
	PWM_init();
<<<<<<< HEAD
	coordinatesInit();
=======
	
	/* Arlo robot initialization */
	arlo_init();
	coordinatesInit();
	
>>>>>>> master
	/* Print info in terminal Window*/
	printf("-- %s\n\r", BOARD_NAME);
	printf("-- Compiled: %s %s --\n\r", __DATE__, __TIME__);
	
<<<<<<< HEAD
	
	/* Create the task with the third priority the task_Move*/
	if (xTaskCreate(task_move, (const signed char * const) "Move", TASK_MOVE_STACK_SIZE, NULL, TASK_MOVE_STACK_PRIORITY, NULL) != pdPASS) {
=======
	/* Create the task with the third priority the task_Move*/
	if (xTaskCreate(task_move, (const signed char * const) "Move", TASK_MOVE_STACK_SIZE, NULL, TASK_MOVE_STACK_PRIORITY, &xTaskMove) != pdPASS) {
>>>>>>> master
		printf("Failed to test task_Move task\r\n");
	}
	
	// 	/* Create the task with the second priority the task_GetCordinates*/
<<<<<<< HEAD
	// 	if (xTaskCreate(task_getCordinates, (const signed char * const) "�Get", TASK_GET_STACK_SIZE, NULL, TASK_GET_STACK_PRIORITY, NULL) != pdPASS) {
	// 		printf("Failed to test GetCordinates task\r\n");
	// 	}
	
	// 		/* Create the task with the least priority the task task_UnoComm */
	// 	if (xTaskCreate(task_unoComm, (const signed char * const) "UNO", TASK_UNO_STACK_SIZE, NULL, TASK_UNO_STACK_PRIORITY, NULL) != pdPASS) {
	// 		printf("Failed to test UnoComm task\r\n");
	// 	}
=======
	 	if (xTaskCreate(task_getCordinates, (const signed char * const) "�Get", TASK_GET_STACK_SIZE, NULL, TASK_GET_STACK_PRIORITY, &xTaskCoordinate) != pdPASS) {
 		printf("Failed to test GetCordinates task\r\n");
	 	}
	
	// 		/* Create the task with the least priority the task task_UnoComm */
	 	if (xTaskCreate(task_unoComm, (const signed char * const) "UNO", TASK_UNO_STACK_SIZE, NULL, TASK_UNO_STACK_PRIORITY, &xTaskCom) != pdPASS) {
	 		printf("Failed to test UnoComm task\r\n");
	 	}
		 
		 vTaskSuspend(xTaskCom);
		 vTaskSuspend(xTaskCoordinate);
>>>>>>> master
	
	/* Start the FreeRTOS scheduler running all tasks indefinitely*/
	vTaskStartScheduler();
}
