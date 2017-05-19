/**
* Task_Move.c
*
* Created: 2017-04-20 14:11:27
*  Author: Hadi Deknache && Yurdaer Dalkic
Desiree J�nsson och Jonas Eiselt
*/

#include <asf.h>
#include <FreeRTOS.h>
#include <inttypes.h>
#include "arlo/Arlo.h"
#include "Task_Move.h"
#include "MathFunctions.h"
#include "PwmFunctions.h"
#include "StepCounter_ISR.h"
 
int distance=0;
int angle=0;
int direction=1;
double totalPulses; //Calculate the total pulses needed to move to destination
uint16_t speed = 200; // Set speed for moving the robot
double proportionalError = 0; //P-controller error variable
double referenceValue = 0;
double measurementValue=0;
double controlValue=0; //Variable to store total value for PID-controller
double K=2.5; //Gain for the PID-controller
double totMovement = 0; //Variable to store totalmovement during the transportation
double integral=0;
double derivate=0;
double prevD=0;
double dT=0.1;
double Td=0.053;//0.53;
double Ti=0.215;//2.15;
int32_t sum=0;
int course;
int rotationGain=5;
int rotationSpeed=90;
int wait =0;
int step =0;
int check =0;
Bool newData=false;
extern uint8_t object_counter;
Bool process_running=false;

Bool liftProcessFinished = false;
Bool liftStart = false;

int pulse_counter=0;

extern xTaskHandle xTaskMove;
extern xTaskHandle xTaskCom;
extern xTaskHandle xTaskCoordinate;

// typedef enum {START,STARTGL,STARTLASSE,BEFORE_ROTATE,ROTATE,MOVE,LIFT,NAVI,CLOSE} states;
typedef enum {START,BEFORE_ROTATE,ROTATE,MOVE,COMM,NAVI,CLOSE} states;

states currentState = START;
states nextState;

Bool pick_up_process_finished = false;

Bool drop_off_process_finished = false;
Pick_Up_Status pick_up_status_t=0;
Find_Object_Status find_object_status_t = 0;
Drop_Off_Status drop_off_status_t = 0;
void task_move(void *pvParameters)
{
	portTickType xLastWakeTime;
	const portTickType xTimeIncrement = 100; //Time given for the task to complete work...
	xLastWakeTime = xTaskGetTickCount(); // Initialize the xLastWakeTime variable with the current time.
	while (1)
	{
		vTaskDelayUntil(&xLastWakeTime, xTimeIncrement); // Wait for the next cycle after have finished everything
		
		switch (currentState)
		{
			case START:
			/* If Arlo can pick up all objects without going to box in between pick-ups */
			if (arlo_get_collect_status() == 1)
			{
				updateNextPosition(); // Grönwall&Larz
				
				if (object_counter > 4)
				{
					// Helloooooooooooo
					nextState = CLOSE;
				}
				else
				{
					nextState = BEFORE_ROTATE;
				}
			}
			else 
			{
				updateNextPosition();
				if (object_counter > 7)
				{
					nextState = CLOSE;
				}
				else
				{
					nextState = BEFORE_ROTATE;
				}
			}
			break;
// 			/************************************************************************/
// 			/*                       Start Grönwalls&Larz                           */
// 			/************************************************************************/
// 			case STARTGL:
// 			updateNextPosGL();
// 			nextState = BEFORE_ROTATE;
// 			if (object_counter>=5)
// 			{
// 				nextState=CLOSE;
// 			}
// 			
// 			break;
// 			/************************************************************************/
// 			/*                        Start Lasse Stefanz                           */
// 			/************************************************************************/
// 			case STARTLASSE:
// 			updateNextPosLasse();
// 			nextState = BEFORE_ROTATE;
// 			if (object_counter>=6)
// 			{
// 				nextState=CLOSE;
// 			}
// 			break;
			/************************************************************************/
			/*                              Navigering                              */
			/************************************************************************/
			case NAVI:
			
			// Kollar efter ny data efter 20 pulser
			if (pulse_counter >= 20 || pulse_counter == 0)
			{
				pulse_counter = 0;
				vTaskResume(xTaskCoordinate);
			}
			
			// Kollar ifall ny data finns efter varje 40ms*20pulser=800ms
			if (newData)
			{
				//vTaskSuspend(xTaskCoordinate);
				newData = false;
				updateLastPresent();
				calculateAngleDistance();
				nextState = MOVE;
			}
			
			if ((liftStart && find_object_status_t==0) && (liftStart && drop_off_status_t == 0))
			{
				printf("Startar kommunikationen med UNO!\r\n");
				pick_up_status_t = PICK_UP_RUNNING;
				liftStart=false;
				nextState = COMM;
				vTaskResume(xTaskCom);
			}
			else
			{
				nextState = MOVE;
			}
			break;
			/************************************************************************/
			/*                               MOVE!!!                                */
			/************************************************************************/
			case MOVE:
			printf("MOVE");
			// 			check++;
			// 			if (check==40)
			// 			{
			// 				angle =90;
			// 				referenceValue = referenceValue - angle/2;
			// 			}
			//
			// 			if (check==90)
			// 			{
			// 				angle =-90;
			// 				referenceValue = referenceValue - angle/2;
			// 			}
			totalPulses = (distance*direction/1.45);
			if (totMovement+2>=totalPulses)
			{
				rightWheel(1500);//Stop rightWheel
				leftWheel(1500);//Stop leftWheel
				controlValue=0;
				measurementValue=0;
				totMovement=0;
				proportionalError=0;
				angle=0;
				sum=0;
				speed = 200;
				distance=0;
				reset_Counter();
				liftStart=true;
				nextState = NAVI;
				//wait=0;
			}
			else
			{
				move();
				nextState = NAVI;
			}
			break;
			/************************************************************************/
			/*                         BEFORE-ROTATE                                */
			/************************************************************************/
			case BEFORE_ROTATE:
			printf("BEFORE ROTATE");
			course=1;     // rotation course, 1 to right -1 to left
			if (angle<0)
			{
				course=-1;
			}
			// total number of pulses required for rotation
			totalPulses=((angle*course)/2);
			rightWheel(1500);
			leftWheel(1500);
			reset_Counter();
			nextState = ROTATE;
			break;
			/************************************************************************/
			/*                                ROTATE                                */
			/************************************************************************/
			case ROTATE:
			printf("ROTATE");
			if ((counter_1+counter_2+1) >= totalPulses)
			{
				//  stop wheels
				rightWheel(1500);
				leftWheel(1500);
				reset_Counter();
				angle=0;
				totalPulses=0;
				totMovement=0;
				nextState = NAVI;
			}
			else
			{
				leftWheel(1500 + ( rotationSpeed*course) );
				rightWheel(1500 - ( rotationSpeed*course) );
				nextState = ROTATE;
			}
			break;
			case CLOSE:
			printf("CLOSE");
			nextState = CLOSE;
			while(1);
			break;
			/************************************************************************/
			/*                                 COMM                                 */
			/************************************************************************/
			case COMM:
			
			if (pick_up_process_finished)
			{
				printf("Pick up process finished\r\n");
				//vTaskSuspend(xTaskCom);
				// status = arlo_get_pick_up_status();
				//status=0;
				//flag_i=1;
				process_running=false;
				pick_up_process_finished=false;
				// nextState = STARTGL;
				nextState = START;
			}
			else
			{
				// vTaskResume(xTaskCom);
				nextState = COMM;
			}
			
			break;
			/************************************************************************/
			/*                           SLUTET AV SWITCH CASE                      */
			/************************************************************************/
		}
		currentState = nextState;
	}
}

void move (void)
{
	if (distance < 0)
	{
		direction =-1;
	}
	else
	{
		direction =1;
	}

	totMovement = ((counter_1+counter_2)/2);
	pulse_counter = pulse_counter + 1;
	measurementValue = (counter_2-counter_1);// Calculates the error differences
	proportionalError = (referenceValue - measurementValue); // Calculates p-controller gain
	if ((referenceValue > 0) && (proportionalError < 0))
	{
		referenceValue = 0;
		proportionalError=0;
		sum=0;
		prevD=0;
		counter_1 = (counter_1+counter_2)/2;
		counter_2=counter_1;
	}
	if ((referenceValue < 0) && (proportionalError > 0))
	{
		referenceValue = 0;
		proportionalError=0;
		sum=0;
		prevD=0;
		counter_1 = (counter_1+counter_2)/2;
		counter_2=counter_1;
	}
	sum = (sum + prevD);
	integral= (sum * (dT/Ti));
	derivate = ((Td/dT) * (proportionalError-prevD));
	controlValue =(K*(proportionalError+integral+ derivate)); //PID
	prevD=proportionalError;
	//	Check if almost reached the destination to slow down and make a smoother brake
	if (((totMovement/totalPulses) >= 0.90) || ((totMovement/totalPulses) <= 0.05))
	{
		speed = 130;
	}
	//Else same speed set
	else
	{
		speed = 200;
	}
	if (controlValue>70)
	{
		controlValue=70;
	}
	else if (controlValue<-70)
	{
		controlValue=-70;
	}
	
	rightWheel(1500 + ((speed+controlValue)*direction));//New speed for rightWheel
	leftWheel( 1500 + ((speed-controlValue)*direction));//New speed for leftWheel
	
}

void coordinatesInit (void)
{
	int16_t object_buffer[8] = {0};
	arlo_get_object_positions(object_buffer);
	
	coord.presentX = 0;
	coord.presentY = 0;
	coord.lastX = 0;
	coord.lastY = -100;

	coord.sock[0] = object_buffer[0];
	coord.sock[1] = object_buffer[1];

	coord.cube[0] = object_buffer[2];
	coord.cube[1] = object_buffer[3];

	coord.glass[0] = object_buffer[4];
	coord.glass[1] = object_buffer[5];

	coord.box[0] = 0;
	coord.box[1] = 0;
}

/* Comment here */
void updateNextPosition()
{
	if (arlo_get_collect_status() == 1)
	{
		if (object_counter == 1)
		{
			printf("Driving to sock!\r\n");
			coord.targetX=coord.sock[0];
			coord.targetY=coord.sock[1];
			calculateAngleDistance();
		}	
		else if (object_counter == 2)
		{
			printf("Driving to cube!\r\n");
			updateLastPresent();
			coord.presentX=coord.sock[0];
			coord.presentY=coord.sock[1];
			coord.targetX=coord.cube[0];
			coord.targetY=coord.cube[1];
			calculateAngleDistance();
		}
		else if (object_counter == 3)
		{
			printf("Driving to glass!\r\n");
			updateLastPresent();
			coord.presentX=coord.cube[0];
			coord.presentY=coord.cube[1];
			coord.targetX=coord.glass[0];
			coord.targetY=coord.glass[1];
			calculateAngleDistance();
		}
		else if(object_counter == 4)
		{
			printf("Driving to box!\r\n");
			updateLastPresent();
			coord.presentX=coord.glass[0];
			coord.presentY=coord.glass[1];
			coord.targetX=coord.box[0];
			coord.targetY=coord.box[1];
			calculateAngleDistance();
		}
		printf("I counter %d \r\n",object_counter);
		object_counter++;
	}
	else 
	{
		if (object_counter == 1)
		{
			coord.targetX=coord.sock[0];
			coord.targetY=coord.sock[1];
			calculateAngleDistance();
		}
		else if (object_counter == 2)
		{
			updateLastPresent();
			coord.presentX=coord.sock[0];
			coord.presentY=coord.sock[1];
			coord.targetX=coord.box[0];
			coord.targetY=coord.box[1];
			calculateAngleDistance();
		}
		else if(object_counter == 3)
		{
			updateLastPresent();
			coord.presentX=coord.box[0];
			coord.presentY=coord.box[1];
			coord.targetX=coord.cube[0];
			coord.targetY=coord.cube[1];
			calculateAngleDistance();
		}
		else if (object_counter == 4)
		{
			updateLastPresent();
			coord.presentX=coord.cube[0];
			coord.presentY=coord.cube[1];
			coord.targetX=coord.box[0];
			coord.targetY=coord.box[1];
			calculateAngleDistance();
		}
		else if (object_counter == 5)
		{
			updateLastPresent();
			coord.presentX=coord.box[0];
			coord.presentY=coord.box[1];
			coord.targetX=coord.glass[0];
			coord.targetY=coord.glass[1];
			calculateAngleDistance();
		}
		else if (object_counter == 6)
		{
			updateLastPresent();
			coord.presentX=coord.glass[0];
			coord.presentY=coord.glass[1];
			coord.targetX=coord.box[0];
			coord.targetY=coord.box[1];
			calculateAngleDistance();
		}
		
		printf("\nObject counter Lasse: %d\r\n",object_counter);
		object_counter++;
	}
}


void updateNextPosGL(void){
	//static uint8_t i = 1;
	if (object_counter==1)
	{
		coord.targetX=coord.sock[0];
		coord.targetY=coord.sock[1];
		calculateAngleDistance();
	}
	
	else if(object_counter==2){
		updateLastPresent();
		coord.presentX=coord.sock[0];
		coord.presentY=coord.sock[1];
		coord.targetX=coord.cube[0];
		coord.targetY=coord.cube[1];
		calculateAngleDistance();
	}
	
	else if(object_counter==3){
		updateLastPresent();
		coord.presentX=coord.cube[0];
		coord.presentY=coord.cube[1];
		coord.targetX=coord.glass[0];
		coord.targetY=coord.glass[1];
		calculateAngleDistance();
	}
	
	else if(object_counter==4){
		updateLastPresent();
		coord.presentX=coord.glass[0];
		coord.presentY=coord.glass[1];
		coord.targetX=coord.box[0];
		coord.targetY=coord.box[1];
		calculateAngleDistance();
	}
	
	printf("I counter %d \r\n", object_counter);
	object_counter++;
}
void updateNextPosLasse(void){
	//static uint8_t i = 1;
	if (object_counter==1)
	{
		coord.targetX=coord.sock[0];
		coord.targetY=coord.sock[1];
		calculateAngleDistance();
	}
	
	else if(object_counter==2){
		updateLastPresent();
		coord.presentX=coord.sock[0];
		coord.presentY=coord.sock[1];
		coord.targetX=coord.box[0];
		coord.targetY=coord.box[1];
		calculateAngleDistance();
	}
	
	else if(object_counter==3){
		updateLastPresent();
		coord.presentX=coord.box[0];
		coord.presentY=coord.box[1];
		coord.targetX=coord.cube[0];
		coord.targetY=coord.cube[1];
		calculateAngleDistance();
	}
	
	else if(object_counter==4){
		updateLastPresent();
		coord.presentX=coord.cube[0];
		coord.presentY=coord.cube[1];
		coord.targetX=coord.box[0];
		coord.targetY=coord.box[1];
		calculateAngleDistance();
	}
	else if(object_counter==5){
		updateLastPresent();
		coord.presentX=coord.box[0];
		coord.presentY=coord.box[1];
		coord.targetX=coord.glass[0];
		coord.targetY=coord.glass[1];
		calculateAngleDistance();
	}
	else if(object_counter==6){
		updateLastPresent();
		coord.presentX=coord.glass[0];
		coord.presentY=coord.glass[1];
		coord.targetX=coord.box[0];
		coord.targetY=coord.box[1];
		calculateAngleDistance();
	}
	else
	{
		printf("\n close now!\r\n");
		nextState = CLOSE;
	}
	printf("\nObject counter: %d\r\n", object_counter);
	object_counter++;
}

void updateLastPresent(void)
{
	coord.lastX=coord.presentX;
	coord.lastY=coord.presentY;
}

void calculateAngleDistance(void){
	printf("NAVI");
	angle = calcluteRotationAngle(coord.lastX,coord.lastY,coord.presentX,coord.presentY,coord.targetX,coord.targetY);
	printf("%d",angle);
	distance = calculateDistance(coord.presentX,coord.presentY,coord.targetX,coord.targetY);
	printf("%d",distance);
}