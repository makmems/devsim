/***
DEVSIM
Copyright 2013 Devsim LLC

This file is part of DEVSIM.

DEVSIM is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, version 3.

DEVSIM is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with DEVSIM.  If not, see <http://www.gnu.org/licenses/>.
***/

#include "mycondition.hh"
#include "mymutex.hh"
#include <cassert>
#include <cstdio>
#ifdef WIN32
#include "Windows.h"
#else
#include <errno.h>
#include <pthread.h>
#endif

// TODO: "Ensure processes acquire python lock when necessary."
mycondition::mycondition() : cond(NULL)
{
#if WIN32
	cond = new CONDITION_VARIABLE;
	InitializeConditionVariable (reinterpret_cast<CONDITION_VARIABLE *>(cond));
#else
    cond = new pthread_cond_t;
    // Need to define appropriate attributes
    int ret = pthread_cond_init(reinterpret_cast<pthread_cond_t *>(cond), NULL);
    if (ret != 0)
    {
	perror("mycondition::mycondition()");
    }
#endif
}

mycondition::~mycondition()
{
#if WIN32
	///// the API doesn't specify
#else
    pthread_cond_t *p = reinterpret_cast<pthread_cond_t *>(cond);
    int ret = pthread_cond_destroy(p);
    delete p;
//    assert (ret == 0);
    if (ret != 0)
    {
	perror("mycondition::~mycondition()");
    }
#endif
}

#if 0
void mycondition::signal()
{
//// Doesn't really match up
  Tcl_ConditionNotify(cond);
#if 0
    int ret = pthread_cond_signal(&cond);
//    assert (ret == 0);
    if (ret != 0)
    {
	perror("mycondition::signal()");
    }
#endif
}
#endif

void mycondition::broadcast()
{
#if WIN32
    WakeAllConditionVariable(reinterpret_cast<CONDITION_VARIABLE *>(cond));
#else
    int ret = pthread_cond_broadcast(reinterpret_cast<pthread_cond_t *>(cond));
//    assert (ret == 0);
    if (ret != 0)
    {
	perror("mycondition::broadcast()");
    }
#endif
}


void mycondition::wait(mymutex &mutex)
{
#if WIN32
  SleepConditionVariableCS(reinterpret_cast<CONDITION_VARIABLE *>(cond), reinterpret_cast<CRITICAL_SECTION *>(mutex.mutex), INFINITE);
#else
  pthread_cond_wait(reinterpret_cast<pthread_cond_t *>(cond), reinterpret_cast<pthread_mutex_t *>(mutex.mutex));
#endif
}

