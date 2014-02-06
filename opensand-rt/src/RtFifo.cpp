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
 * @file   RtFifo.cpp
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The fifo and signaling pipres for opensand-rt
 *         intra-block messages
 */

#include "Rt.h"
#include "RtFifo.h"

#include <opensand_conf/uti_debug.h>

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <errno.h>

#define DEFAULT_FIFO_SIZE 3

RtFifo::RtFifo():
	fifo(),
	max_size(DEFAULT_FIFO_SIZE)
{
}

RtFifo::~RtFifo()
{
	close(this->r_sig_pipe);
	close(this->w_sig_pipe);

/*	while(!this->fifo.empty())
	{
		this->fifo.pop();
	}
	delete msg.data*/

	pthread_mutex_destroy(&(this->fifo_mutex));
	sem_destroy(&(this->fifo_size_sem));
}

bool RtFifo::init()
{
	int32_t pipefd[2];
	pthread_mutexattr_t mutex_attr;
	int ret;

	UTI_DEBUG("Initialize fifo\n");

	ret = pthread_mutexattr_init(&mutex_attr);
	if(ret != 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to initialize mutex attributes [%d: %s]\n",
		                ret, strerror(ret));
		return false;
	}
	/* choose mutexes depending on what you need:
	    - PTHREAD_MUTEX_ERRORCHECK for library validation
	    - PTHREAD_MUTEX_NORMAL for fast mutex */
	ret = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
	if(ret != 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to set mutex attributes [%d: %s]\n",
		                ret, strerror(ret));
		goto error;
	}
	
	ret = pthread_mutex_init(&(this->fifo_mutex), &mutex_attr);
	if(ret != 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to initialize mutex on FIFO[%d: %s]\n",
		                ret, strerror(ret));
		goto error;
	}
	ret = sem_init(&(this->fifo_size_sem), 0, this->max_size);
	if(ret != 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to initialize semaphore on FIFO [%d: %s]\n",
		                ret, strerror(ret));
		goto error;
	}

	if(pipe(pipefd) != 0)
	{
		goto error;
	}
	this->r_sig_pipe = pipefd[0];
	this->w_sig_pipe = pipefd[1];

	pthread_mutexattr_destroy(&mutex_attr);
	return true;

error:
	pthread_mutexattr_destroy(&mutex_attr);
	return false;
}


bool RtFifo::push(void *data, size_t size, uint8_t type)
{
	fd_set wset;
	bool status = false;
	rt_msg_t msg;
	int ret;
	
	UTI_DEBUG_L3("push message in fifo\n");

	// we need a semaphore here to block while fifo is full
	ret = sem_wait(&(this->fifo_size_sem));
	if(ret != 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to lock mutex for FIFO full [%d: %s]\n",
		                ret, strerror(ret));
		return false;
	}

	// lock mutex on fifo
	ret = pthread_mutex_lock(&(this->fifo_mutex));
	if(ret != 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to lock mutex on FIFO [%d: %s]\n",
		                ret, strerror(ret));
		return false;
	}

	//assert(this->fifo.size() < this->max_size);
	if(this->fifo.size() >= this->max_size)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Size is greater than maximum size (%u > %u), "
		                "this should not happend\n",
		                this->fifo.size(), this->max_size);
	}

	msg.data = data;
	msg.length = size;
	msg.type = type;
	this->fifo.push(msg);
	FD_ZERO(&wset);
	FD_SET(this->w_sig_pipe, &wset);
	if(select(this->w_sig_pipe + 1, NULL, &wset, NULL, NULL) < 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Select failed on pipe [%d: %s]\n",
		                errno, strerror(errno));
		goto error;
	}
	if(write(this->w_sig_pipe, MAGIC_WORD, strlen(MAGIC_WORD)) != strlen(MAGIC_WORD))
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to write on pipe\n");
		goto error;
	}
	status = true;

error:
	// unlock mutex on fifo
	if(pthread_mutex_unlock(&(this->fifo_mutex)) != 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to unlock mutex on FIFO\n");
		status = false;
	}
	return status;

}

bool RtFifo::pop(rt_msg_t &elem)
{
	bool status = true;
	
	// lock mutex on fifo
	if(pthread_mutex_lock(&(this->fifo_mutex)) != 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to lock mutex on FIFO\n");
		return false;
	}

	// assert(!this->fifo.empty());
	if(this->fifo.empty())
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Fifo is already empty, this should not happend\n");
		status = false;
	}
	else
	{
		// get element in queue
		elem = this->fifo.front();

		// remove element from queue
		this->fifo.pop();
	}

	// unlock mutex on fifo
	if(pthread_mutex_unlock(&(this->fifo_mutex)) != 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to unlock mutex on FIFO\n");
		status = false;
	}

	// fifo has empty space, we can unlock it
	if(sem_post(&(this->fifo_size_sem)) != 0)
	{
		Rt::reportError("fifo", pthread_self(), false,
		                "Failed to unlock mutex for FIFO full\n");
		status = false;
	}
	return status;
}


