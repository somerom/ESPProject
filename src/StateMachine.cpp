/*
 * StateMachine.cpp
 *
 *  Created on: 8.3.2020
 *      Author: johan
 */

#include "StateMachine.h"


StateMachine::~StateMachine() {
	// TODO Auto-generated destructor stub
}



void StateMachine::HandelControlWorld(const Event &e,ModbusRegister& ControlWord){
	switch (e.type) {
	case EventType::switch_inhibited:
		//add the event from the Eventhandler here call it by events_.function();
		// set controll word  Bit 0 = 0
		break;
	case EventType::switch_not_ready:
		//add the event from the Eventhandler here
		// set CW  to xxxx xxxx xxxx x110
		break;
	case EventType::switch_ready:
		//add the event from the Eventhandler here
		//set CW to xxxx xxxx xxxx x111
		break;
	case EventType::ready_operate:
		//add the event from the Eventhandler here
		// bit3 =1
		break;
	case EventType::operation_enalbled:
		//add the event from the Eventhandler here
		//bit 5 =1
		break;
	case EventType::RFG:
		//add the event from the Eventhandler here
		//bit 6=1
		break;
	case EventType::operating:
		//add the event from the Eventhandler here
		break;
	}
}

void StateMachine::HandelState(ModbusRegister& StatusWorld,ModbusRegister& ControlWord){
	// check each state untill it mach the condition and after that change the CW
	//laita jokaisin while looppiin ehdoksi SW bitti compareit and take 1 awey
	while(1/*events_.*/){}
	HandelControlWorld(Event(EventType::switch_inhibited), ControlWord);
	while(1){}
	HandelControlWorld(Event(EventType::switch_not_ready), ControlWord);
	while(1){}
	HandelControlWorld(Event(EventType::switch_ready), ControlWord);
	while(1){}
	HandelControlWorld(Event(EventType::ready_operate), ControlWord);
	// check firt that sw bit 12=1 and then bit 2 =1
	while(1){}
	HandelControlWorld(Event(EventType::operation_enalbled), ControlWord);
	while(1){}
	HandelControlWorld(Event(EventType::RFG), ControlWord);
	while(1){}
	HandelControlWorld(Event(EventType::operating), ControlWord);

}


