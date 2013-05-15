/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file TestMultiBlocks.h
 * @author Cyrille Gaillardet <cgaillardet@toulouse.viveris.com>
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @brief This test check that we can read a file on a channel then
 *        transmit content to lower block, the bottom block transmit it
 *        to the following channel that will forward it to the top and
 *        compare output
 *
 *
 *        file
 *          |
 *  +-------+-----------------------+
 *  | +-----+-----+   +-----------+ |
 *  | |     |     |   |  compare  | |
 *  | |     |     |Top|     |     | |
 *  | |     |     |   |     |     | |
 *  | +-----|-----+   +-----+-----+ |
 *  +-------|---------------+-------+
 *          |               |
 *  +-------+---------------+-------+
 *  | +-----+-----+   +-----+-----+ |
 *  | |     |     |   |     |     | |
 *  | |     |    Middle     |     | |
 *  | |     |     |   |     |     | |
 *  | +-----+-----+   +-----+-----+ |
 *  +-------|-----------------------+
 *          |               |
 *  +-------+---------------+-------+
 *  | +-----|-----+   +-----+-----+ |
 *  | |     |     |   |     |     | |
 *  | |     |    Bottom     |     | |
 *  | |     |     |   |     |     | |
 *  | +-----+-----+   +-----+-----+ |
 *  |       +---------------+       |
 *  +-------------------------------+
 */

#include "TestMultiBlocks.h"

#include "Rt.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <signal.h>
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if ENABLE_TCMALLOC
#include <gperftools/heap-checker.h>
#endif

unsigned char dbgLevel_default = 4;

using std::ostringstream;

static char *read_msg(const MessageEvent *const event, string name, string from)
{
	char *data;
	switch(event->getType())
	{
		case evt_message:
			data = (char *)event->getData();
			data[event->getLength()] = '\0';
			std::cout << "Block " << name << ": " << strlen(data)
			          << " bytes of data received from "
			          << from << " block" << std::endl;
			fflush(stdout);
			break;

		default:
			Rt::reportError(name, pthread_self(), true,
			                "unknown event: %u", event->getType());
			return NULL;

	}
	return data;
}


/*
 * Top Block:
 *  - downward: read file (NetSocketEvent) and transmit it to lower block
 *  - upward: read message from lower block (MessageEvent) and compare to file
 */

TopBlock::TopBlock(const string &name):
	Block(name)
{
}

bool TopBlock::onInit()
{
	this->input_fd = open("TestMultiBlocks.h", O_RDONLY);
	if(this->input_fd < 0)
	{
		//abort test
		Rt::reportError(this->name, pthread_self(), true,
		                "cannot open input file");
	}
	// high priority to be sure to read it before another timer
	this->downward->addFileEvent("top_downward", this->input_fd, 1000);
	return true;
}

bool TopBlock::onDownwardEvent(const RtEvent *const event)
{
	char *data;
	size_t size;
	switch(event->getType())
	{
		case evt_file:
			size = ((NetSocketEvent *)event)->getSize();
			if(size == 0)
			{
				// EOF stop process
				sleep(1);
				std::cout << "EOF: kill process" << std::endl;
				kill(getpid(), SIGTERM);
				break;
			}
			data =  (char *)((NetSocketEvent *)event)->getData();
			std::cout << "Block " << this->name << ": " << strlen(data)
			          << " bytes of data received on net socket" << std::endl;
			fflush(stdout);
			size = strlen(data);
			// keep data in order to compare on the opposite block
			strncpy(this->last_written, data, size + 1);
			// wait in order to receive data on the opposite block and compare it
			// this also allow testing multithreading as this thread is paused
			// while other should handle the data


			// transmit to lower layer
			if(!this->sendDown((void **)&data, size))
			{
				Rt::reportError(this->name, pthread_self(), true,
				                "cannot send data to lower block");
			}
			sleep(1);
			break;

		default:
			Rt::reportError(this->name, pthread_self(), true,
			                "unknown event: %u", event->getType());
			return false;

	}
	return true;
}

bool TopBlock::onUpwardEvent(const RtEvent *const event)
{
	char *data = read_msg((MessageEvent *)event, this->name, "lower");
	if(!data)
	{
		return false;
	}
	// compare data
	if(strcmp(data, this->last_written))
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "wrong data received '%s' instead of '%s'",
		                data, this->last_written);
		free(data);
		return false;
	}
	std::cout << "LOOP: MATCH" << std::endl;
	free(data);
	return true;
}

TopBlock::~TopBlock()
{
	close(this->input_fd);
}

/*
 * Middle Block:
 *  - downward/upward: read message (MessageEvent) and transmit to following block
 */

MiddleBlock::MiddleBlock(const string &name):
	Block(name)
{
}

bool MiddleBlock::onInit()
{
	return true;
}

bool MiddleBlock::onUpwardEvent(const RtEvent *const event)
{
	char *data = read_msg((MessageEvent *)event, this->name, "lower");
	if(!data)
	{
		return false;
	}

	// transmit to upper layer
	if(!this->sendUp((void **)&data, strlen(data)))
	{
		Rt::reportError(this->name, pthread_self(), true, "cannot send data to upper block");
	}
	return true;
}

bool MiddleBlock::onDownwardEvent(const RtEvent *const event)
{
	char *data = read_msg((MessageEvent *)event, this->name, "upper");
	if(!data)
	{
		return false;
	}

	// transmit to lower layer
	if(!this->sendDown((void **)(&data), strlen(data)))
	{
		Rt::reportError(this->name, pthread_self(), true, "cannot send data to lower block");
	}
	return true;
}

MiddleBlock::~MiddleBlock()
{
}

/*
 * Bottom Block:
 *  - downward: read message from upper block (MessageEvent) and write on pipe
 *  - upward: read on pipe (NetSocketEvent) and transmit it to upper block
 */

BottomBlock::BottomBlock(const string &name):
	Block(name)
{
}

bool BottomBlock::onInit()
{
	int32_t pipefd[2];

	if(pipe(pipefd) != 0)
	{
		std::cerr << "error when opening pipe between upward and "
		          << "downward channels" << std::endl;
		return false;
	}
	this->input_fd = pipefd[0];
	this->output_fd = pipefd[1];

	// high priority to be sure to read it before another timer
	this->upward->addFileEvent("bottom_upward", this->input_fd, 1000, 2);
	return true;
}

bool BottomBlock::onDownwardEvent(const RtEvent *const event)
{
	int res = 0;
	char *data = read_msg((MessageEvent *)event, this->name, "upper");
	if(!data)
	{
		return false;
	}

	// write on pipe for opposite channel
	res = write(this->output_fd,
	            data, strlen(data));
	if(res == -1)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "cannot write on pipe");
		free(data);
		return false;
	}
	free(data);
	return true;
}

bool BottomBlock::onUpwardEvent(const RtEvent *const event)
{
	char *data;
	size_t size;
	switch(event->getType())
	{
		case evt_file:
			size = ((NetSocketEvent *)event)->getSize();
			data = (char *)((NetSocketEvent *)event)->getData();;
			std::cout << "Block " << this->name << ": " << strlen(data)
			          << " bytes of data received on net socket" << std::endl;
			fflush(stdout);

			// transmit to upper layer
			if(!this->sendUp((void **)&data, strlen(data)))
			{
				Rt::reportError(this->name, pthread_self(), true, "cannot send data to upper block");
			}
			break;

		default:
			Rt::reportError(this->name, pthread_self(), true,
			                "unknown event %u", event->getType());
			return false;

	}
	return true;
}

BottomBlock::~BottomBlock()
{
	close(this->output_fd);
	// input fd closed in NetSocketEvent
}


int main(int argc, char **argv)
{
	int ret = 0;
#if ENABLE_TCMALLOC
HeapLeakChecker heap_checker("test_multi_blocks");
{
#endif
	Block *top;
	Block *middle;
	Block *bottom;
	string error;

	std::cout << "Launch test" << std::endl;

	top = Rt::createBlock<TopBlock, TopBlock::Upward,
	                      TopBlock::Downward>("top");

	middle = Rt::createBlock<MiddleBlock, MiddleBlock::Upward,
	                         MiddleBlock::Downward>("middle", top);

	bottom = Rt::createBlock<BottomBlock, BottomBlock::Upward,
	                         BottomBlock::Downward>("bottom", middle);

	std::cout << "Start loop, please wait..." << std::endl;
	if(!Rt::run(true))
	{
		ret = 1;
		std::cerr << "Unable to run" << std::endl;
	}
	else
	{
		std::cout << "Successfull" << std::endl;
	}

#if ENABLE_TCMALLOC
}
if(!heap_checker.NoLeaks()) assert(NULL == "heap memory leak");
#endif

	return ret;
}
