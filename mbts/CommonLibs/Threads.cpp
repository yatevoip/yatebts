/*
* Copyright 2008 Free Software Foundation, Inc.
*
*
* This software is distributed under the terms of the GNU Affero Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/



#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#include "Threads.h"
#include "Timeval.h"


using namespace std;




Mutex gStreamLock;		///< Global lock to control access to cout and cerr.

void lockCout()
{
	gStreamLock.lock();
	Timeval entryTime;
	cout << entryTime << " " << pthread_self() << ": ";
}


void unlockCout()
{
	cout << dec << endl << flush;
	gStreamLock.unlock();
}


void lockCerr()
{
	gStreamLock.lock();
	Timeval entryTime;
	cerr << entryTime << " " << pthread_self() << ": ";
}

void unlockCerr()
{
	cerr << dec << endl << flush;
	gStreamLock.unlock();
}







Mutex::Mutex()
{
	bool res;
	res = pthread_mutexattr_init(&mAttribs);
	assert(!res);
	res = pthread_mutexattr_settype(&mAttribs,PTHREAD_MUTEX_RECURSIVE);
	assert(!res);
	res = pthread_mutex_init(&mMutex,&mAttribs);
	assert(!res);
}


Mutex::~Mutex()
{
	pthread_mutex_destroy(&mMutex);
	bool res = pthread_mutexattr_destroy(&mAttribs);
	assert(!res);
}




/** Block for the signal up to the cancellation timeout. */
void Signal::wait(Mutex& wMutex, unsigned timeout) const
{
	Timeval then(timeout);
	struct timespec waitTime = then.timespec();
	pthread_cond_timedwait(&mSignal,&wMutex.mMutex,&waitTime);
}

void Thread::start(void *(*task)(void*), void *arg, const char* name, ...)
{
#ifdef PR_SET_NAME
	class NamedTask
	{
		void *(*mTask)(void*);
		void *mArg;
		char mName[16];
	public:
		inline NamedTask(void *(*task)(void*), void *arg, const char* name, va_list ap)
			: mTask(task), mArg(arg)
		{
			vsnprintf(mName, 15, name, ap);
			mName[15] = '\0';
		}

		static void* run(void* arg)
		{
			NamedTask* t = static_cast<NamedTask*>(arg);
			void *(*task)(void*) = t->mTask;
			arg = t->mArg;
			prctl(PR_SET_NAME, (unsigned long)t->mName, 0, 0, 0);
			delete t;
			return task(arg);
		}
	};
#endif

	assert(mThread==((pthread_t)0));
	bool res;
	// (pat) Moved initialization to constructor to avoid crash in destructor.
	//res = pthread_attr_init(&mAttrib);
	//assert(!res);
	res = pthread_attr_setstacksize(&mAttrib, mStackSize);
	assert(!res);
#ifdef PR_SET_NAME
	if (name && *name) {
		va_list ap;
		va_start(ap, name);
		NamedTask* t = new NamedTask(task, arg, name, ap);
		va_end(ap);
		res = pthread_create(&mThread, &mAttrib, NamedTask::run, t);
	}
	else
#endif
		res = pthread_create(&mThread, &mAttrib, task, arg);
	assert(!res);
}



// vim: ts=4 sw=4
