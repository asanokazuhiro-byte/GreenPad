#ifndef _KILIB_KTL_ARRAY_H_
#define _KILIB_KTL_ARRAY_H_
#include "types.h"
#include "memory.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.KTL //@}
//@{
//	Variable-length array for basic types only
//
//	It is very simple to make. You are free to access wherever you like,
//	Add/delete elements only to the end. A bit-by-bit copy
//	I do a lot of eccentric work, so it's okay to do that kind of work.
//	Please do not use it for anything other than molds.
//@}
//=========================================================================

template <typename T, bool clean_on_destroy=true>
class A_WUNUSED storage
{
public:

	//@{
	//	constructor
	//
	//	@param alloc_size
	//		The initial "memory" size to allocate.
	//		Note that this is not the "array's" size.
	//@}
	explicit storage( size_t allocSize )
		: alen_( Max( allocSize, (size_t)1 ) )
		, len_ ( 0 )
		, buf_ ( static_cast<T*>(mem().Alloc(alen_*sizeof(T))) )
		{
			if( !buf_ )
				alen_=0;
		}

	~storage()
		{ if (clean_on_destroy) mem().DeAlloc( buf_, alen_*sizeof(T) ); }

	void Clear()
		{ mem().DeAlloc( buf_, alen_*sizeof(T) ); };

	//@{ Add element to end //@}
	bool Add( const T& obj )
		{
			if( len_ >= alen_ )
				if( !ReAllocate( alen_<<1 ) )
					return false;
			buf_[ len_++ ] = obj;
			return true;
		}

	//@{
	//	Force array size change
	//
	//	Both reduction/enlargement is possible. Unlike constructors,
	//	The maximum index will change based on the specified value.
	//	@param new_size New size.
	//@}
	bool ForceSize( size_t newSize )
		{
			if( newSize > alen_ )
				if( !ReAllocate( newSize ) )
					return false;
			len_ = newSize;
			return true;
		}

public:

	//@{Number of elements //@}
	size_t size() const
		{ return len_; }

	//@{get element //@}
	T& operator[]( size_t i )
		{ return buf_[i]; }

	//@{ Get element (const) //@}
	const T& operator[]( size_t i ) const
		{ return buf_[i]; }

	//@{ Return pointer to start of array //@}
	const T* head() const { return buf_; }

private:

	bool ReAllocate( size_t siz )
		{
			T* newbuf = static_cast<T*>(mem().Alloc( siz*sizeof(T) ));
			if( !newbuf ) return false;
			memmove( newbuf, buf_, len_*sizeof(T) );
			mem().DeAlloc( buf_, alen_*sizeof(T) );
			alen_ = siz;
			buf_ = newbuf;
			return true;
		}

protected:

	size_t alen_;
	size_t len_;
	T*    buf_;
};

//=========================================================================
template<class T, bool free_on_del=true>
struct sstorage
{
	struct ss { size_t len_; T *buf_; };
	enum { SSZ = sizeof(ss) / sizeof(T) };

	sstorage()
	: slen_ ( 0 )
	, alen_ ( SSZ )
	{ }

	~sstorage() { if( free_on_del ) Clear(); }

	void Clear()
	{
		if( alen_ > SSZ )
			ki::mem().DeAlloc( u.s.buf_, alen_*sizeof(T) );
	}

	size_t size() const { return alen_<=SSZ? slen_ : u.s.len_; };

	bool Add( T val )
	{
		size_t prevlen;
		if( alen_ <= SSZ )
		{
			if( slen_ < alen_ )
			{
				u.a[ slen_++ ] = val;
				return true;
			}
			prevlen = slen_;
		} else
			prevlen = u.s.len_;

		if( prevlen >= alen_ )
			if( !ReAllocate( alen_<<1 ) )
				return false;
		u.s.buf_[ u.s.len_++ ] = val;
		return true;
	}

	bool ForceSize( size_t newSize )
	{
		if( newSize > alen_ )
		{
			// Reallocation needed
			if( !ReAllocate( newSize ) )
				return false; // ERROR
		}
		else if( alen_ <= SSZ )
		{
			// Still in short mode
			slen_= newSize;
			return true; // DONE
		}
//		else if( newSize <= SSZ )
//		{
//			// Switch back to small mode
//			T *ob = u.s.buf_;
//			memmove( u.a, ob, newSize * sizeof(T) );
//			ki::mem().DeAlloc( ob, alen_ * sizeof(T) );
//			slen_ = newSize;
//			alen_ = SSZ;
//			return true;
//		}

		u.s.len_ = newSize; // Set size (long list mode)
		return true;
	}

	bool ReAllocate( size_t siz )
	{
		T* newbuf = static_cast<T*>(ki::mem().Alloc( siz*sizeof(T) ));
		if( !newbuf ) return false;
		memmove( newbuf, alen_ > SSZ? u.s.buf_ : u.a, size()*sizeof(T) );
		if ( alen_ > SSZ )
		{
			memmove( newbuf, u.s.buf_, u.s.len_*sizeof(T) );
			ki::mem().DeAlloc( u.s.buf_, alen_*sizeof(T) );
		}
		else
		{
			memmove( newbuf, u.a, slen_*sizeof(T) );
			u.s.len_ = slen_;
		}

		alen_ = siz;
		u.s.buf_ = newbuf;

		return true;
	}

	T operator [] (size_t i) const
	{
		if (alen_ <= SSZ)
			return u.a[i];
		return u.s.buf_[i];
	}

	T& operator [] (size_t i)
	{
		if (alen_ <= SSZ)
			return u.a[i];
		return u.s.buf_[i];
	}
private:
	size_t slen_ : 3;
	size_t alen_ : sizeof(size_t)*8 - 3;
	union { struct ss s; T a[SSZ]; } u;
};

//=========================================================================
//@{
//	Unidirectional list that can also be used as an object type
//
//	I can hardly do anything. All you can do is add to the end and create your own
//	Sequential access only with iterator.
//@}
//=========================================================================

template <class T>
class A_WUNUSED olist
{
private:

	struct Node {
		explicit Node( const T& obj )
			: obj_ ( obj ), next_( NULL ) {}
		~Node()
			{ delete next_; }
		Node* Add( Node* pN )
			{ return next_==NULL ? next_=pN : next_->Add(pN); }
		T     obj_;
		Node* next_;
	};

public:

	struct iterator {
		explicit iterator( Node* p=NULL ) : ptr_(p)   {}
		T& operator*()                       { return ptr_->obj_; }
		T* operator->() const                { return &ptr_->obj_; }
		bool operator==( const iterator& i ) { return i.ptr_==ptr_; }
		bool operator!=( const iterator& i ) { return i.ptr_!=ptr_; }
		iterator& operator++()    { ptr_=ptr_->next_; return *this; }
	private:
		Node* ptr_;
	};

public:

	//@{ constructor //@}
	olist()
		: top_( NULL ) {}

	//@{ destructor //@}
	~olist()
		{ empty(); }

	//@{ empty //@}
	void empty()
		{ delete top_; top_ = NULL; }

	//@{ beginning //@}
	iterator begin()
		{ return iterator(top_); }

	//@{ trailing //@}
	iterator end()
		{ return iterator(); }

	//@{ Add element to end //@}
	void Add( const T& obj )
		{
			Node* pN = new Node( obj );
			if( !pN ) return;
			(top_ == NULL) ? top_=pN : top_->Add( pN );
		}

	//@{ Delete specified element //@}
	void Del( iterator d )
		{
			if( d != end() )
			{
				Node *p=top_, *q=NULL;
				for( ; p!=NULL; q=p,p=p->next_ )
					if( &p->obj_ == &*d )
						break;
				if( q != NULL )
					q->next_ = p->next_;
				else
					top_ = p->next_;
				p->next_ = NULL;
				delete p;
			}
		}

	//@{ Delete everything after the specified element //@}
	void DelAfter( iterator d )
		{
			if( d != end() )
			{
				if( d == begin() )
				{
					empty();
				}
				else
				{
					Node *p=top_, *q=NULL;
					for( ; p!=NULL; q=p, p=p->next_ )
						if( &p->obj_ == &*d )
							break;
					delete p;
					if( q )
						q->next_ = NULL;
				}
			}
		}

private:

	Node* top_;
	NOCOPY(olist);
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_KTL_ARRAY_H_
