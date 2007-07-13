/*
    Enki - a fast 2D robot simulator
    Copyright (C) 1999-2006 Stephane Magnenat <stephane at magnenat dot net>
    Copyright (C) 2004-2005 Markus Waibel <markus dot waibel at epfl dot ch>
    Copyright (c) 2004-2005 Antoine Beyeler <abeyeler at ab-ware dot com>
    Copyright (C) 2005-2006 Laboratory of Intelligent Systems, EPFL, Lausanne
    Copyright (C) 2006 Laboratory of Robotics Systems, EPFL, Lausanne
    See AUTHORS for details

    This program is free software; the authors of any publication 
    arising from research using this software are asked to add the 
    following reference:
    Enki - a fast 2D robot simulator
    http://lis.epfl.ch/enki
    Stephane Magnenat <stephane at magnenat dot net>,
    Markus Waibel <markus dot waibel at epfl dot ch>
    Laboratory of Intelligent Systems, EPFL, Lausanne.

    You can redistribute this program and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "AsebaMarxbot.h"
#include <common/consts.h>
#include <set>
#include <map>
#include <cassert>
#include <cstring>
#include <algorithm>

/*!	\file Marxbot.cpp
	\brief Implementation of the aseba-enabled marXbot robot
*/

// Implementation of aseba glue code

// map for aseba glue code
typedef std::map<AsebaVMState*, Aseba::Socket*>  VmSocketMap;
static VmSocketMap asebaSocketMaps;

extern "C" void AsebaSendMessage(AsebaVMState *vm, uint16 id, void *data, uint16 size)
{
	Aseba::Socket* socket = asebaSocketMaps[vm];
	assert(socket);
	
	// write message
	socket->write(&size, 2);
	socket->write(&vm->nodeId, 2);
	socket->write(&id, 2);
	socket->write(data, size);
	socket->flush();
}

extern "C" void AsebaSendVariables(AsebaVMState *vm, uint16 start, uint16 length)
{
	Aseba::Socket* socket = asebaSocketMaps[vm];
	assert(socket);
	
	// write message
	uint16 size = length * 2 + 2;
	uint16 id = ASEBA_MESSAGE_VARIABLES;
	socket->write(&size, 2);
	socket->write(&vm->nodeId, 2);
	socket->write(&id, 2);
	socket->write(&start, 2);
	socket->write(vm->variables + start, length * 2);
	socket->flush();
}

void AsebaWriteString(Aseba::Socket *socket, const char *s)
{
	size_t len = strlen(s);
	uint8 lenUint8 = static_cast<uint8>(strlen(s));
	socket->write(&lenUint8, 1);
	socket->write(s, len);
}

extern "C" void AsebaSendDescription(AsebaVMState *vm)
{
	Aseba::Socket* socket = asebaSocketMaps[vm];
	assert(socket);
	
	// write sizes (basic + nodeName + variables)
	uint16 size;
	switch (vm->nodeId)
	{
		case 1: size = 6 + 11 + (8 + 15); break;
		case 2: size = 6 + 12 + (8 + 15); break;
		case 3: size = 6 + 18 + (8 + 20); break;
		case 4: size = 6 + 17 + (6 + 15); break;
		default: assert(false); break;
	}
	socket->write(&size, 2);
	socket->write(&vm->nodeId, 2);
	uint16 id = ASEBA_MESSAGE_DESCRIPTION;
	socket->write(&id, 2);
	
	// write node name
	switch (vm->nodeId)
	{
		case 1: AsebaWriteString(socket, "left motor"); break;
		case 2: AsebaWriteString(socket, "right motor"); break;
		case 3: AsebaWriteString(socket, "proximity sensors"); break;
		case 4: AsebaWriteString(socket, "distance sensors"); break;
		default: assert(false); break;
	}
	
	// write sizes
	socket->write(&vm->bytecodeSize, 2);
	socket->write(&vm->stackSize, 2);
	socket->write(&vm->variablesSize, 2);
	
	switch (vm->nodeId)
	{
		case 1:
		case 2:
		{
			// motors
			size = 3;
			socket->write(&size, 2);
			
			AsebaWriteString(socket, "args");
			size = 32;
			socket->write(&size, 2);
			
			AsebaWriteString(socket, "speed");
			size = 1;
			socket->write(&size, 2);
			
			AsebaWriteString(socket, "odo");
			size = 2;
			socket->write(&size, 2);
		}
		break;
		
		case 3:
		{
			// proximity sensors
			size = 3;
			socket->write(&size, 2);
			
			AsebaWriteString(socket, "args");
			size = 32;
			socket->write(&size, 2);
			
			AsebaWriteString(socket, "bumpers");
			size = 24;
			socket->write(&size, 2);
			
			AsebaWriteString(socket, "ground");
			size = 12;
			socket->write(&size, 2);
		}
		break;
		
		case 4:
		{
			// distance sensors
			size = 2;
			socket->write(&size, 2);
			
			AsebaWriteString(socket, "args");
			size = 32;
			socket->write(&size, 2);
			
			AsebaWriteString(socket, "distances");
			size = 180;
			socket->write(&size, 2);
		}
		break;
		
		default: assert(false);	break;
	}
	
	socket->flush();
}

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	// no native functions for now
}

extern "C" void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason)
{
	std::cerr << "\nFatal error: ";
	switch (vm->nodeId)
	{
		case 1: std::cerr << "left motor module"; break;
		case 2:	std::cerr << "right motor module"; break;
		case 3: std::cerr << "proximity sensors module"; break;
		case 4: std::cerr << "distance sensors module"; break;
		default: std::cerr << "unknown module"; break;
	}
	std::cerr << " has produced exception: ";
	switch (reason)
	{
		case ASEBA_ASSERT_UNKNOWN: std::cerr << "undefined"; break;
		case ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR: std::cerr << "unknown binary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BYTECODE: std::cerr << "unknown bytecode"; break;
		case ASEBA_ASSERT_STACK_OVERFLOW: std::cerr << "stack overflow"; break;
		case ASEBA_ASSERT_STACK_UNDERFLOW: std::cerr << "stack underflow"; break;
		case ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS: std::cerr << "out of variables bounds"; break;
		case ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "out of bytecode bounds"; break;
		case ASEBA_ASSERT_STEP_OUT_OF_RUN: std::cerr << "step out of run"; break;
		case ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "breakpoint out of bytecode bounds"; break;
		default: std::cerr << "unknown exception"; break;
	}
	std::cerr << ".\npc = " << vm->pc << ", sp = " << vm->sp;
	std::cerr << "\nResetting VM" << std::endl;
	assert(false);
	AsebaVMInit(vm, vm->nodeId);
}

namespace Enki
{
	AsebaMarxbot::Module::Module()
	{
		bytecode.resize(512);
		vm.bytecode = &bytecode[0];
		vm.bytecodeSize = bytecode.size();
		
		stack.resize(64);
		vm.stack = &stack[0];
		vm.stackSize = stack.size();
		
		amountOfTimerEventInQueue = 0;
	}
	
	AsebaMarxbot::AsebaMarxbot(const std::string &host, unsigned short port) 
	{
		// connect
		connect(host, port);
		
		// setup modules specific data
		leftMotor.vm.variables = reinterpret_cast<sint16 *>(&leftMotorVariables);
		leftMotor.vm.variablesSize = sizeof(leftMotorVariables) / sizeof(sint16);
		modules.push_back(&leftMotor);
		
		rightMotor.vm.variables = reinterpret_cast<sint16 *>(&rightMotorVariables);
		rightMotor.vm.variablesSize = sizeof(rightMotorVariables) / sizeof(sint16);
		modules.push_back(&rightMotor);
		
		proximitySensors.vm.variables = reinterpret_cast<sint16 *>(&proximitySensorVariables);
		proximitySensors.vm.variablesSize = sizeof(proximitySensorVariables) / sizeof(sint16);
		modules.push_back(&proximitySensors);
		
		distanceSensors.vm.variables = reinterpret_cast<sint16 *>(&distanceSensorVariables);
		distanceSensors.vm.variablesSize = sizeof(distanceSensorVariables) / sizeof(sint16);
		modules.push_back(&distanceSensors);
		
		// fill map
		asebaSocketMaps[&leftMotor.vm] = this;
		asebaSocketMaps[&rightMotor.vm] = this;
		asebaSocketMaps[&proximitySensors.vm] = this;
		asebaSocketMaps[&distanceSensors.vm] = this;
		
		// init VM
		AsebaVMInit(&leftMotor.vm, 1);
		AsebaVMInit(&rightMotor.vm, 2);
		AsebaVMInit(&proximitySensors.vm, 3);
		AsebaVMInit(&distanceSensors.vm, 4);
	}
	
	AsebaMarxbot::~AsebaMarxbot()
	{
		// clean map
		asebaSocketMaps.erase(&leftMotor.vm);
		asebaSocketMaps.erase(&rightMotor.vm);
		asebaSocketMaps.erase(&proximitySensors.vm);
		asebaSocketMaps.erase(&distanceSensors.vm);
	}
	
	void AsebaMarxbot::step(double dt)
	{
		/*
			Values mapping
			
			motor:
				estimated 3000 == 30 cm/s
			
			encoders:
				16 tick per motor turn
				134 reduction
				6 cm wheel diameter
		*/
		
		// set physical variables
		leftSpeed = static_cast<double>(leftMotorVariables.speed) / 100;
		rightSpeed = static_cast<double>(rightMotorVariables.speed) / 100;
		
		// do motion
		DifferentialWheeled::step(dt);
		
		// get physical variables
		int odoLeft = static_cast<int>((leftOdometry * 16  * 134) / (2 * M_PI));
		leftMotorVariables.odo[0] = odoLeft & 0xffff;
		leftMotorVariables.odo[1] = odoLeft >> 16;
		
		int odoRight = static_cast<int>((rightOdometry * 16  * 134) / (2 * M_PI));
		rightMotorVariables.odo[0] = odoRight & 0xffff;
		rightMotorVariables.odo[1] = odoRight >> 16;
		
		for (size_t i = 0; i < 24; i++)
			proximitySensorVariables.bumpers[i] = static_cast<sint16>(getVirtualBumper(i));
		std::fill(proximitySensorVariables.ground, proximitySensorVariables.ground + 12, 0);
		
		for (size_t i = 0; i < 180; i++)
			distanceSensorVariables.distances[i] = static_cast<sint16>(rotatingDistanceSensor.zbuffer[i]);
		
		// push on timer events
		Event onTimer;
		onTimer.id = ASEBA_EVENT_PERIODIC;
		for (size_t i = 0; i < modules.size(); i++)
		{
			if (modules[i]->amountOfTimerEventInQueue == 0)
			{
				modules[i]->eventsQueue.push_back(onTimer);
				modules[i]->amountOfTimerEventInQueue++;
			}
		}
		
		// process all events
		while (true)
		{
			bool wasActivity = false;
			
			Aseba::NetworkClient::step();
			
			for (size_t i = 0; i < modules.size(); i++)
			{
				// if events in queue and not blocked in a thread, execute new thread
				if (!modules[i]->eventsQueue.empty() && !AsebaVMIsExecutingThread(&modules[i]->vm))
				{
					// copy event to vm
					BaseVariables *vars = reinterpret_cast<BaseVariables*>(modules[i]->vm.variables);
					const Event &event = modules[i]->eventsQueue.front();
					size_t amount = std::min(event.data.size(), sizeof(vars->args) / sizeof(sint16));
					std::copy(event.data.begin(), event.data.begin() + amount, vars->args);
					AsebaVMSetupEvent(&modules[i]->vm, event.id);
					
					// pop event
					if (event.id == ASEBA_EVENT_PERIODIC)
						modules[i]->amountOfTimerEventInQueue--;
					modules[i]->eventsQueue.pop_front();
				}
				
				// try to run, notice if anything was run
				if (AsebaVMRun(&modules[i]->vm))
					wasActivity = true;
			}
			
			if (!wasActivity)
				break;
		}
	}
	
	void AsebaMarxbot::incomingData()
	{
		unsigned short len;
		unsigned short type;
		unsigned short source;
		read(&len, 2);
		read(&source, 2);
		read(&type, 2);
		std::valarray<unsigned char> buffer(static_cast<size_t>(len));
		read(&buffer[0], buffer.size());
		
		signed short *dataPtr = reinterpret_cast<signed short *>(&buffer[0]);
		
		if (type < 0x8000)
		{
			// user type queue
			assert(buffer.size() % 2 == 0);
			
			// create event
			Event event;
			event.id = type;
			
			for (size_t i = 0; i < buffer.size(); i++)
				event.data.push_back(*dataPtr++);
			
			// push event in queus
			for (size_t i = 0; i < modules.size(); i++)
				modules[i]->eventsQueue.push_back(event);
		}
		else
		{
			// debug message
			if (type >= 0xA000)
			{
				// not bootloader
				if (buffer.size() % 2 != 0)
				{
					std::cerr << std::hex << std::showbase;
					std::cerr << "AsebaMarxbot::incomingData() : fatal error: received event: " << type;
					std::cerr << std::dec << std::noshowbase;
					std::cerr << " of size " << buffer.size();
					std::cerr << ", which is not a power of two." << std::endl;
					assert(false);
				}
				for (size_t i = 0; i < modules.size(); i++)
					AsebaVMDebugMessage(&modules[i]->vm, type, reinterpret_cast<uint16 *>(dataPtr), buffer.size() / 2);
			}
		}
	}
	
	void AsebaMarxbot::connectionEstablished()
	{
		// do nothing in addition to what is done by NetworkClient
	}
	
	void AsebaMarxbot::connectionClosed()
	{
		// do nothing in addition to what is done by NetworkClient
	}
}

