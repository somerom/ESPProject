/*
 * fan.cpp
 *
 *  Created on: 5.3.2020
 *      Author: johan
 */

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include "StateMachine.h"
#include <cr_section_macros.h>

#include <cstring>
#include <cstdio>

#include "ModbusMaster.h"
#include "ModbusRegister.h"
#include "DigitalIoPin.h"
#include "LpcUart.h"
#include "ITM_output.h"
#include "I2C.h"


static volatile int counter;
static volatile uint32_t systicks;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief	Handle interrupt from SysTick timer
 * @return	Nothing
 */
void SysTick_Handler(void)
{
	systicks++;
	if(counter > 0) counter--;
}
#ifdef __cplusplus
}
#endif

void Sleep(int ms)
{
	counter = ms;
	while(counter > 0) {
		__WFI();
	}
}

/* this function is required by the modbus library */
uint32_t millis() {
	return systicks;
}

bool Wait(int ms)
{
	bool ended = false;
	counter = ms;
	while(counter > 0);
	return ended = true;

}


void LimitCheck(auto& nr,int max,int min){
	if(nr <= min){
		nr = min;
	}
	if(nr >= max){
		nr = max;
	}
}

bool setFrequency(ModbusMaster& node, uint16_t freq)
{
	int result;
	int ctr;
	bool atSetpoint;
	const int delay = 100;

	ModbusRegister Frequency(&node, 1); // reference 1
	ModbusRegister StatusWord(&node, 3);

	Frequency = freq; // set motor frequency

	printf("Set freq = %d\n", freq/40); // for debugging

	// wait until we reach set point or timeout occurs
	ctr = 0;
	atSetpoint = false;
	do {
		Sleep(delay);
		// read status word
		result = StatusWord;
		// check if we are at setpoint
		if (result >= 0 && (result & 0x0100)) atSetpoint = true;
		ctr++;
	} while(ctr < 20 && !atSetpoint);

	//printf("Elapsed: %d\n", ctr * delay); // for debugging
	return atSetpoint;
}

//alttitudecorrection for 0-120 meter hight
int16_t AlltiduteCorrection(int16_t pressure){
	pressure = (pressure/240)*0.95;
	return pressure;
}
int scale(float base,int nr){
	float i = nr/base;
	return (int) i;
}

int16_t PressureSensor(){
	I2C_config cfg;
	I2C p(cfg);
	uint8_t status[3];
	uint16_t t;
	uint8_t read= 0xF1;
	//if(p.read(0x40, &read, 1));

	p.transaction(0x40, &read, 1,status,3);

	//printf("T; %d",t);
	// left shit and add the secound value to the
	t = ((uint16_t)status[0] << 8) | status[1];

	if((t & 0x8000)!=0) {
		int16_t mask = 0x7FFF;
		t = t & mask;
		t -= 32768;
	}
	return  AlltiduteCorrection((int16_t)t);
}
// Gives the requested frequency according to the pascal value between 0 to 120 = 1.19 ((max value - 1)/100)
//
int PascalToScale(int nr,int base){
	nr+=125;
	int nr2 = scale(base,nr);
	return nr2*200;
}

bool PressureSetting(int expectedpressure,ModbusMaster& node,int16_t& freq){
	bool ready = false;
	int16_t pressure= PressureSensor();
	if(pressure == expectedpressure ){
		ready = true;
		printf("frequency %d\n", freq);
	}else if(expectedpressure<pressure){
		freq-=100;

		printf("frequency %d\n", freq);
	}else if(expectedpressure>pressure){
		freq+=100;
		printf("frequency %d\n", freq);
	}
	LimitCheck(freq,20000,0);
	setFrequency(node, freq);
	printf("Frequency: %4d expected: %d, actuall pressure %d\n",freq,expectedpressure,pressure);
	return ready;
}

/*
int scale(int base,int nr){
	int i = nr/base;
	return i;
}*/


void ManualButtons(ModbusMaster& node,int16_t& freq){
	DigitalIoPin button1(0,9, true, true, true);
	DigitalIoPin button2(0,0, true, true, true);
	DigitalIoPin button3(1,3, true, true, true);
	DigitalIoPin button4(0,10, true, true, true);
	while(1){
		if (button1.read()) {
			while(button1.read()){
				Sleep(100);
				freq +=200;
				LimitCheck(freq,20000,0);
				setFrequency(node, freq);
				int percentage = scale(200,freq);
				printf("freq: %4d precentage: %d\n",freq,percentage);
			}
		}else if (button2.read()) {
			while(button2.read()){
				Sleep(100);
				freq -=200;
				LimitCheck(freq,20000,0);
				setFrequency(node, freq);
				int percentage = scale(200,freq);
				printf("freq: %4d precentage: %d\n",freq,percentage);
			}
		}else if (button3.read()) {
			printf("button3\n");
			break;
		}else if (button4.read()) {
			printf("Going back to menu\n");
			break;
		}
		setFrequency(node, freq);
		int16_t pressure=PressureSensor();
		int percentage = scale(200,freq);
		printf("Freq: %4d %: %d Pressure: %4d\n",freq,percentage,pressure);
	}
}


void AutomaticButtons(ModbusMaster& node,int16_t& freq,int& expectedpresusre){

	DigitalIoPin button1(0,9, true, true, true);
	DigitalIoPin button2(0,0, true, true, true);
	DigitalIoPin button3(1,3, true, true, true);
	DigitalIoPin button4(0,10, true, true, true);
	int count=0;
	int counterlimit = 100;
	while(1){
		if (button1.read()) {
			while(button1.read()){
				Sleep(100);
				expectedpresusre +=1;
				LimitCheck(expectedpresusre,120,0);
				PressureSetting(expectedpresusre,node,freq);
			}
			printf("button1, %d\n",expectedpresusre);
			count = 0;
		}else if (button2.read()) {
			while(button2.read()){
				Sleep(100);
				expectedpresusre -=1;
				LimitCheck(expectedpresusre,120,0);
				PressureSetting(expectedpresusre,node,freq);
			}
			printf("button2, %d\n",expectedpresusre);
			count=0;
		}else if (button3.read()) {
			printf("button3\n");
			break;
			printf("loop out\n");
		}else if (button4.read()) {
			printf("button4\n");
			break;
			//back to main menu
		}
		count++;
		if(count >= counterlimit){
			printf("message\n");
			count=0;
		}
		printf("count %d",count);
		if(PressureSetting(expectedpresusre,node,freq)){
			count=0;
		}
	}
}

void abbmodbus(){
	ModbusMaster node(2);
	node.begin(9600);

	ModbusRegister ControlWord(&node, 0);
	ModbusRegister StatusWord(&node, 3);
	ModbusRegister OutputFrequency(&node, 102);
	ModbusRegister Current(&node, 103);

	DigitalIoPin button1(0,9, true, true, true);
	DigitalIoPin button2(0,0, true, true, true);
	DigitalIoPin button3(1,3, true, true, true);
	DigitalIoPin button4(0,10, true, true, true);

	StateMachine state;
	int16_t freq= 0;
	int delay = 100;
	ControlWord = 0x0406;
	printf("Status=%04X\n", (int)StatusWord); // for debugging
	Sleep(1000);
	ControlWord = 0x047F;
	printf("Status=%04X\n", (int)StatusWord); // for debugging
	Sleep(1000);
	printf("Start!\n");
	//state.HandelState(StatusWord,ControlWord);

	int expecetedpressure=0;
	while(1){
		while (node.HeartBeat()) {
			if (button1.read()) {
				printf("Manual mode\n");
				ManualButtons(node,freq);
			}else if (button2.read()) {
				printf("Automatic mode\n");
				AutomaticButtons(node,freq,expecetedpressure);
			}
			printf("F=%4d, I=%4d\n", (int) OutputFrequency, (int) Current);
			//printf("Getting inside the function\n");
			//Sleep(delay);
		}
		printf("INACTIVE\n");
	}

}


int main(void)
{

#if defined (__USE_LPCOPEN)
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
	// Set up and initialize all required blocks and
	// functions related to the board hardware
	Board_Init();
	// Set the LED to the state of "On"
	Board_LED_Set(0, true);
#endif
#endif
	LpcPinMap none = {-1, -1}; // unused pin has negative values in it
	LpcPinMap txpin = { 0, 18 }; // transmit pin that goes to debugger's UART->USB converter
	LpcPinMap rxpin = { 0, 13 }; // receive pin that goes to debugger's UART->USB converter
	LpcUartConfig cfg = { LPC_USART0, 115200, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1, false, txpin, rxpin, none, none };
	LpcUart dbgu(cfg);

	/* Set up SWO to PIO1_2 */
	Chip_SWM_MovablePortPinAssign(SWM_SWO_O, 1, 2); // Needed for SWO printf

	/* Enable and setup SysTick Timer at a periodic rate */
	SysTick_Config(SystemCoreClock / 1000);

	Board_LED_Set(0, false);
	Board_LED_Set(1, true);
	printf("Started\n"); // goes to ITM console if retarget_itm.c is included
	dbgu.write("Hello, world\n");
	printf("Hello\n");
	abbmodbus();

	return 1;
}


