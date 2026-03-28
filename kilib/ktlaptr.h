#ifndef _KILIB_KTL_APTR_H_
#define _KILIB_KTL_APTR_H_
#include "types.h"
#ifdef _MSC_VER
#pragma warning( disable : 4284 ) // Warning: -> return type is limp
#pragma warning( disable : 4150 ) // Warning: Definition of delete is confusing
#endif
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.KTL //@}
//@{
//	automatic pointer
//
//	I think it behaves roughly the same as std::unique_ptr within the scope of my expectations...
//	Hurray for the best wheel invention!
//@}
//=========================================================================

template<class T>
class A_WUNUSED uptr
{
public:

	//@{ constructor //@}
	explicit uptr( T* p = NULL )
		: obj_( p ) {}

	//@{ destructor //@}
	~uptr()
		{ delete obj_; }

	//@{ Set/Reset object, (delete the old) //@}
	void reset( T *ptr = NULL )
		{
		#ifdef _DEBUG
			if( obj_ && obj_ == ptr )
				MessageBox(NULL, TEXT("uptr::reset(): trying to reset an object to itself!"), NULL, 0);
		#endif
			T *old = obj_;
			obj_ = ptr;
			delete old;
		}

public:

	//@{ indirect operator //@}
	T& operator*() const
		{ return *obj_; }

	//@{ member reference //@}
	T* operator->() const
		{ return obj_; }

	//@{ Get pointer //@}
	T* get() const
		{ return obj_; }

	//@{Release ownership //@}
	T* release()
		{
			T* ptr = obj_;
			obj_ = NULL;
			return ptr;
		}

	//@{ Is it valid? //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

private:
	NOCOPY(uptr);
	T* obj_;
};



//=========================================================================
//@{
//	Automatic pointer (array version)
//@}
//=========================================================================

template<class T>
class A_WUNUSED aarr
{
public:

	//@{ constructor //@}
	explicit aarr( size_t sz )
		: obj_( (T *)malloc( sizeof(T) * sz ) ) {}
	aarr(): obj_( NULL ) {} ;

	//@{ destructor //@}
	~aarr()
		{ free( obj_ ); }

	//@{ Ownership transfer For some reason bcc doesn't work, so I added const //@}
	aarr( const aarr<T>& r )
		: obj_ ( const_cast<aarr<T>&>(r).release() ) {}

	//@{ Ownership transfer //@}
	aarr<T>& operator=( aarr<T>& r )
		{
			if( obj_ != r.obj_ )
			{
				free( obj_ );
				obj_ = r.release();
			}
			return *this;
		}

public:

	//@{ Get pointer //@}
	T* get() const
		{ return obj_; }

	//@{Release ownership //@}
	T* release()
		{
			T* ptr = obj_;
			obj_ = NULL;
			return ptr;
		}

	//@{ Is it valid? //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

public:

	//@{Array element access //@}
	T& operator[]( int i ) const
		{ return obj_[i]; }

private:

	T* obj_;
};



//=========================================================================
//@{
//	Deletion rights exclusive pointer
//
//	"Resources are acquired in the constructor and released in the destructor"
//	If you can be thorough, don't use this, use const auto_ptr without hesitation
//	Should. Should be, but if you use this in the member initialization list, VC++'s
//	I don't want to get scolded by the compiler, so I just used this constructor
//	Because it is initialized within the function...(^^;
//@}
//=========================================================================
#if 0
template<class T>
class dptr
{
public:

	//@{ constructor //@}
	explicit dptr( T* p = NULL )
		: obj_( p ) {}

	//@{ destructor //@}
	~dptr()
		{ delete obj_; }

	//@{ own new object; Delete the old one //@}
	void operator=( T* p )
		{
			delete obj_; // Delete the old one
			obj_ = p;
		}

	//@{ Is it valid? //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

public:

	//@{ indirect operator //@}
	T& operator*() const
		{ return *obj_; }

	//@{ member reference //@}
	T* operator->() const
		{ return obj_; }

	//@{ Get pointer //@}
	T* get() const
		{ return obj_; }

private:

	T* obj_;

private:

	NOCOPY(dptr<T>);
};



//=========================================================================
//@{
//	Deletion right exclusive pointer (array version)
//@}
//=========================================================================

template<class T>
class darr
{
public:

	//@{ constructor //@}
	explicit darr( T* p = NULL )
		: obj_( p ) {}

	//@{ destructor //@}
	~darr()
		{ delete [] obj_; }

	//@{ own new object; Delete the old one //@}
	void operator=( T* p )
		{
			delete [] obj_; // Delete the old one
			obj_ = p;
		}

	//@{ Is it valid? //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

public:

	//@{Array element access //@}
	T& operator[]( int i ) const
		{ return obj_[i]; }

private:

	T* obj_;

private:

	NOCOPY(darr<T>);
};
#endif

//=========================================================================

#ifdef _MSC_VER
#pragma warning( default : 4150 )
#pragma warning( default : 4284 )
#endif
}      // namespace ki
#endif // _KILIB_KTL_APTR_H_
