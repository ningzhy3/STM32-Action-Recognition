/* 	File: 		main.c
* 	Author: 	Zhiyuan Ning
*/

/* system imports */
#include <mbed.h>
#include <math.h>
#include <USBSerial.h>

/* user imports */
#include "LIS3DSH.h"

/* USBSerial library for serial terminal */
USBSerial serial(0x1f00, 0x2012, 0x0001, false);

/* LIS3DSH Library for accelerometer  - using SPI*/
LIS3DSH acc(PA_7, SPI_MISO, SPI_SCK, PE_3);

/* LED output */
DigitalOut ledOut(LED1);
DigitalOut ledOut2(LED4);
DigitalOut ledOut3(LED5);
DigitalOut ledOut4(LED6);

// button input
DigitalIn ledOut_button(USER_BUTTON);

/* state type definition
four start and four end of exercise */
typedef enum state_t
{
	lying,
	sitting,
	pushup_down,
	pushup_up,
	squats_up,
	squats_down,
	jump_up,
	jump_down
} state_t;

int main()
{
	int16_t X, Y;				// not really used
	int16_t zAccel = 0;			// acceleration in z (raw)
	float g_z = 0;				// acceleration in z (g force)
	float angle = 0;			// angle relative to z axis
	const float PI = 3.1415926; // pi
	int16_t exercise_times = 5;

	//count the times of each exercise
	int16_t situps = 0;
	int16_t pushup = 0;
	int16_t squats = 0;
	int16_t jump = 0;
	state_t state = lying;

	/**** Filter Parameters  ****/
	const uint8_t N = 20;	   // filter length
	float ringbuf[N];		   // sample buffer
	uint8_t ringbuf_index = 0; // index to insert sample

	/**  ringbug = {0, 15000, 0, 0, 0, 0, 0}  **/
	/*				   ^				       */
	/*				   |					   */
	/*			 ringbuf_index	  			   */

	/* check detection of the accelerometer */
	while (acc.Detect() != 1)
	{
		printf("Could not detect Accelerometer\n\r");
		wait_ms(200);
	}
	// set the led on
	ledOut = 1;
	ledOut2 = 1;
	ledOut3 = 1;
	ledOut4 = 1;
	while (1)
	{
		// press the button and reset the count of four exercises
		if (ledOut_button.read() == 1)
		{
			situps = 0;
			pushup = 0;
			squats = 0;
			jump = 0;
		}

		/* read data from the accelerometer */
		acc.ReadData(&X, &Y, &zAccel);

		/* normalise to 1g */
		g_z = (float)zAccel / 17694.0;

		/* insert in to circular buffer */
		ringbuf[ringbuf_index++] = g_z;

		/* at the end of the buffer, wrap around to the beginning */
		if (ringbuf_index >= N)
		{
			ringbuf_index = 0;
		}

		/********** START of filtering ********************/

		float g_z_filt = 0;

		/* add all samples */
		for (uint8_t i = 0; i < N; i++)
		{
			g_z_filt += ringbuf[i];
		}

		/* divide by number of samples */
		g_z_filt /= (float)N;

		/* restrict to 1g (acceleration not decoupled from orientation) */
		if (g_z_filt > 1)
		{
			g_z_filt = 1;
		}

		/********** END of filtering **********************/

		/* compute angle in degrees */
		angle = 180 * acos(g_z_filt) / PI;

		switch (state)
		{
		case lying: // lying down

			if (Y < -20000) // if Y < -2000 the state will turn to jump_up
				state = jump_up;

			else if (Y > 0) // if Y >0 the state will turn to jump_down
				state = jump_down;
			else if (angle > 45 && angle < 70) // the state will turn to sitting
			{
				state = sitting;
			}

			else if (angle > 1135 && angle < 150) //the state will turn to pushup_up
				state = pushup_up;
			else if (angle > 150)
				state = pushup_down;
			else if (angle < 135 && angle > 120) //the state will turn to squats_down
				state = squats_down;
			else if (angle < 100 && angle > 80) //the state will turn to squats_up
				state = squats_up;
			break;

		case sitting: // sitting up

			if (Y < -20000)
				state = jump_up;
			else if (Y > 0)
				state = jump_down;

			else if (angle > 0 && angle < 30)
			{
				state = lying;
				situps++;
			}
			else if (angle > 135 && angle < 150)
				state = pushup_up;
			else if (angle > 150)
				state = pushup_down;
			else if (angle < 135 && angle > 120)
				state = squats_down;
			else if (angle < 100 && angle > 80)
				state = squats_up;
			break;

		case pushup_down: // pushup down to the floor

			if (Y < -20000)
				state = jump_up;
			else if (Y > 0)
				state = jump_down;

			else if (angle > 45 && angle < 70)
			{
				state = sitting;
			}

			else if (angle > 135 && angle < 150)
				state = pushup_up;
			else if (angle < 135 && angle > 120)
				state = squats_down;
			else if (angle < 100 && angle > 80)
				state = squats_up;

			break;

		case pushup_up: // push up

			if (Y < -20000)
				state = jump_up;
			else if (Y > 0)
				state = jump_down;

			else if (angle > 45 && angle < 70)
			{
				state = sitting;
			}
			else if (angle > 0 && angle < 30)
				state = lying;

			else if (angle > 150)
			{
				pushup++;
				state = pushup_down;
			}
			else if (angle < 135 && angle > 120)
				state = squats_down;
			else if (angle < 100 && angle > 80)
				state = squats_up;

			break;

		case squats_up: // squats up means standing

			if (Y < -20000)
				state = jump_up;
			else if (Y > 0)
				state = jump_down;

			else if (angle > 45 && angle < 70)
				state = sitting;
			else if (angle > 0 && angle < 30)
				state = lying;
			else if (angle > 135 && angle < 150)
				state = pushup_up;
			else if (angle > 150)
				state = pushup_down;
			else if (angle < 135 && angle > 120)
				state = squats_down;
			break;

		case squats_down: // squats down means squats

			if (Y < -20000)
				state = jump_up;
			else if (Y > 0)
				state = jump_down;

			else if (angle > 45 && angle < 70)
				state = sitting;
			else if (angle > 0 && angle < 30)
				state = lying;
			else if (angle > 135 && angle < 150)
				state = pushup_up;
			else if (angle > 150)
				state = pushup_down;

			else if (angle < 100 && angle > 80)
			{
				squats++;
				state = squats_up;
			}
			break;

		case jump_up: // when jump to the air

			if (Y > -20000)
			{
				jump++;
				state = jump_down;
			}

			else if (angle > 45 && angle < 70)
				state = sitting;
			else if (angle > 0 && angle < 30)
				state = lying;
			else if (angle > 135 && angle < 150)
				state = pushup_up;
			else if (angle > 150)
				state = pushup_down;
			else if (angle < 135 && angle > 120)
				state = squats_down;

		case jump_down: // when fall to the floor

			if (Y < -20000)
				state = jump_up;

			else if (angle > 45 && angle < 70)
				state = sitting;
			else if (angle > 0 && angle < 30)
				state = lying;
			else if (angle > 135 && angle < 150)
				state = pushup_up;
			else if (angle > 150)
				state = pushup_down;
			else if (angle < 135 && angle > 120)
				state = squats_down;

		default:
			printf("Zhiyuan made a mistake\n\r");
		}
		// if one of the exercise count up to the exercise_times that we set, the led is on
		if (situps >= exercise_times)
			ledOut = 0;
		else
			ledOut = 1;

		if (pushup >= exercise_times)
			ledOut2 = 0;
		else
			ledOut2 = 1;

		if (squats >= exercise_times)
			ledOut3 = 0;
		else
			ledOut3 = 1;

		if (jump >= exercise_times * 2)
			ledOut4 = 0;
		else
			ledOut4 = 1;

		// monitor the counts
		serial.printf("angle: %.2f situps %d pushup %d squats %d jump %d \r\n", angle, situps, pushup, squats, jump);
		// serial.printf("X: %d Y: %d Z: %d \r\n", X, Y ,zAccel);
		wait_ms(10);
	}
}
