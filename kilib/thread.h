#ifndef _KILIB_THREAD_H_
#define _KILIB_THREAD_H_
#include "types.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.Core //@}
//@{
//	Managing multithreading
//
//	nothing yet
//@}
//=========================================================================


class ThreadManager
{
public:
	//@{ Launch executable object //@}
	void Run( class Runnable& r );

	//@{ Are multiple threads running? //@}
	bool isMT() const;

private:

	ThreadManager();

private:

	int threadNum_;
	static ThreadManager* pUniqueInstance_;

private:

	static DWORD WINAPI ThreadStubFunc(void*);

private:

	friend void APIENTRY Startup();
	friend inline ThreadManager& thd();
	NOCOPY(ThreadManager);
};



//-------------------------------------------------------------------------

//@{ Return only one thread management object //@}
inline ThreadManager& thd()
	{ return *ThreadManager::pUniqueInstance_; }

inline bool ThreadManager::isMT() const
	{ return 0 != threadNum_; }



//=========================================================================
//@{
//	executable object base
//@}
//=========================================================================

class Runnable
{
protected:
	Runnable();
	virtual ~Runnable();

	virtual void StartThread() = 0;
	void PleaseExit();
	bool isExitRequested() const;
	bool isRunning() const;

private:
	void FinalizeThread();
	friend class ThreadManager;
	HANDLE hEvent_;
	HANDLE hThread_;
};



//=========================================================================
//@{
//	Multithread policy 1
//
//	Deriving from this class enables use of the AutoLock class.
//	Passing a pointer to it enters exclusive mode; the destructor releases it.
//	NoLockable::AutoLock is a no-op.
//@}
//=========================================================================

class NoLockable
{
protected:
	struct AutoLock
	{
		explicit AutoLock( NoLockable* host ) {}
	};
};



//=========================================================================
//@{
//	Multithread policy 2
//
//	Deriving from this class enables use of the AutoLock class.
//	Passing a pointer to it enters exclusive mode; the destructor releases it.
//	EzLockable::AutoLock provides fast but incomplete exclusive control for single-threaded use.
//	It becomes unsafe the moment a second thread is spawned.
//@}
//=========================================================================
class EzLockable
{
protected:
	EzLockable()
		{
		#ifdef USE_THREADS
			::InitializeCriticalSection( &csection_ );
		#endif
		}
	~EzLockable()
		{
		#ifdef USE_THREADS
			::DeleteCriticalSection( &csection_ );
		#endif
		}

	struct AutoLock
	{
		AutoLock( EzLockable* host )
		{
		#ifdef USE_THREADS
			if( NULL != (pCs_=(thd().isMT() ? &host->csection_ : NULL)) )
				::EnterCriticalSection( pCs_ );
		#endif
		}
		~AutoLock()
		{
		#ifdef USE_THREADS
			if( pCs_ )
				::LeaveCriticalSection( pCs_ );
		#endif
		}
	private:
		NOCOPY(AutoLock);
		#ifdef USE_THREADS
	    CRITICAL_SECTION* pCs_;
		#endif
	};

private:
	#ifdef USE_THREADS
	CRITICAL_SECTION csection_;
	#ifdef __DMC__
		friend struct EzLockable::AutoLock;
	#else
		friend struct AutoLock;
	#endif
	#endif
};



//=========================================================================
//@{
//	Multithread policy 3
//
//	Deriving from this class allows you to use the AutoLock class.
//	By passing this pointer to this class, it enters an exclusive state, and in the destructor
//	You will be able to get out. Lockable::AutoLock provides complete exclusive control.
//	I will do it. If you want to be sure, be sure to use this class.
//@}
//=========================================================================
class Lockable
{
protected:
	Lockable()
		{ ::InitializeCriticalSection( &csection_ ); }
	~Lockable()
		{ ::DeleteCriticalSection( &csection_ ); }

	struct AutoLock
	{
		AutoLock( Lockable* host )
		{
			pCs_ = &host->csection_;
			::EnterCriticalSection( pCs_ );
		}
		~AutoLock()
		{
			::LeaveCriticalSection( pCs_ );
		}
	private:
		NOCOPY(AutoLock);
	    CRITICAL_SECTION* pCs_;
	};
	friend struct AutoLock;

private:
	CRITICAL_SECTION csection_;
};


//=========================================================================

}      // namespace ki
#endif // _KILIB_THREAD_H_
