#include "stdafx.h"
#include "memory.h"
using namespace ki;

#ifdef _DEBUG
#include "log.h"
#include "kstring.h"
#endif


//=========================================================================

#ifdef __GNUC__
// Operator new/delete for -nostdlib gcc builds (call malloc/free from msvcrt)
void* __cdecl operator new(size_t siz)     { return malloc(siz); }
void  __cdecl operator delete(void* ptr)   { if (ptr) free(ptr); }
void  operator delete(void* ptr, size_t)   { ::operator delete(ptr); }
void  operator delete[](void* ptr)         { ::operator delete(ptr); }
void  operator delete[](void* ptr, size_t) { ::operator delete(ptr); }
void* operator new[](size_t sz)            { return ::operator new(sz); }
#endif // __GNUC__


#ifdef USE_ORIGINAL_MEMMAN
//=========================================================================
//	Allocator for efficient memory management
//=========================================================================

//
// memory block
// My job is to allocate space for "siz bytes * num" at once.
//
// [Next free block index] is stored in the first byte of a free block.
// Using this, it is treated as a unidirectional list that can only be moved in and out of the beginning.
//

struct ki::MemBlock
{
public:
	bool  Construct( ushort siz, ushort num )
	{
		// Secure
		buf_   = (byte *)malloc( siz*num );
		if( !buf_ )
			return false;
		first_ = 0;
		avail_ = num;

		// Linked list initialization
		ushort i=0;
		for( byte *p=buf_; i<num; p+=siz )
			*((ushort*)p) = ++i;
		return true;
	}

	inline void  Destruct()
		{ ::free( buf_ ); /* release */ }

	void* Alloc( ushort siz )
	{
		// memory extraction
		//   (Leave checks such as avail==0 to the upper layer)
		byte* blk = buf_ + siz*first_;
		first_    = *(ushort*)blk;
		--avail_;
		return blk;
	}

	void  DeAlloc( void* ptr, ushort siz )
	{
		// restore memory
		//   (What if a strange pointer was passed to me?)
		byte* blk = static_cast<byte*>(ptr);
		*(ushort*)blk      = first_;
		first_    = static_cast<ushort>((blk-buf_)/siz);
		++avail_;
	}

	inline bool  isAvail()
		{ return (avail_ != 0); } // Is there any space available?
	inline bool  isEmpty( ushort num )
		{ return (avail_ == num); } // Completely empty?
	bool hasThisPtr( const void* ptr, size_t len ) // Pointer to this block?
		{ return ( buf_<=ptr && ptr<buf_+len ); }

private:
	byte* buf_;
	ushort first_, avail_;
};

//-------------------------------------------------------------------------

//
// Fixed size ensure person
// My job is to secure an area of ​​``siz bytes'' each time.
//
// Keep a list of memory blocks and use free blocks
// Respond to memory demands. If there is no space left, create a new MemBlock.
// Make one and add it to the list.
//
// Remember each block that was last memory allocated/released,
// Try to speed up the process by checking that first.
//

bool MemoryManager::FixedSizeMemBlockPool::Construct( ushort siz )
{
	// Secure a little memory block information area
	blocks_ = (MemBlock *)malloc( sizeof(MemBlock) * 4 );
	if( !blocks_ ) return false;

	// Calculate block size etc.
	int npb = BLOCK_SIZ/siz;
	numPerBlock_ = static_cast<ushort>( Min( npb, 65535 ) );
	fixedSize_   = siz;

	// Create only one block
	bool ok = blocks_[0].Construct( fixedSize_, numPerBlock_ );
	if( !ok )
	{
		free( blocks_ ); // free on failure
		return false;
	}
	// All good.
	lastA_            = 0;
	lastDA_           = 0;
	blockNum_         = 1;
	blockNumReserved_ = 4;
	return true;
}

void MemoryManager::FixedSizeMemBlockPool::Destruct()
{
	// free each block
	for( int i=0; i<blockNum_; ++i )
		blocks_[i].Destruct();

	// The memory of the block information holding area is also released.
	free( blocks_ );
	blockNum_ = 0;
}

void* MemoryManager::FixedSizeMemBlockPool::Alloc()
{
	// It would be bad if we didn't check whether lastA_ is Valid here.
	// It might be DeAlloced and gone.

	// To the block from which memory was previously extracted
	// Check if there are still spaces available
	if( !blocks_[lastA_].isAvail() )
	{
		// If not found, perform a linear search starting from the end of the list.
		for( int i=blockNum_;; )
		{
			if( blocks_[--i].isAvail() )
			{
				// Found an empty block!
				lastA_ = i;
				break;
			}
			if( i == 0 )
			{
				// They were all full...
				if( blockNum_ == blockNumReserved_ )
				{
					// Moreover, the work area is full, so expand it.
					MemBlock* nb = (MemBlock *)malloc( sizeof(MemBlock) * blockNum_*2 );
					if( !nb )
						return NULL;
					memmove( nb, blocks_, sizeof(MemBlock)*(blockNum_) );
					free( blocks_ );
					blocks_ = nb;
					blockNumReserved_ *= 2;
				}

				// Build a new block
				bool ok = blocks_[ blockNum_ ].Construct( fixedSize_, numPerBlock_ );
				if( !ok ) return NULL;
				lastA_ = blockNum_++;
				break;
			}
		}
	}
	void *ret = blocks_[lastA_].Alloc( fixedSize_ );
	// Cut out and allocate from block
	return ret;
}

void MemoryManager::FixedSizeMemBlockPool::DeAlloc( void* ptr )
{
	// Search for the corresponding block
	const INT_PTR mx=blockNum_-1, ln=fixedSize_*numPerBlock_;
	for( INT_PTR u=lastDA_, d=lastDA_-1;; )
	{
		if( u>=0 )
			if( blocks_[u].hasThisPtr(ptr,ln) )
			{
				lastDA_ = u;
				break;
			}
			else if( u==mx )
			{
				u = -1;
			}
			else
			{
				++u;
			}
		if( d>=0 )
			if( blocks_[d].hasThisPtr(ptr,ln) )
			{
				lastDA_ = d;
				break;
			}
			else
			{
				--d;
			}
	}

	// execute release
	blocks_[lastDA_].DeAlloc( ptr, fixedSize_ );

	// If this deletion leaves the block completely empty
	if( blocks_[lastDA_].isEmpty( numPerBlock_ ) )
	{
		// And if it wasn't the last block
		INT_PTR end = blockNum_-1;
		if( lastDA_ != end )
		{
			// If the backmost one is empty, release it.
			if( blocks_[end].isEmpty( numPerBlock_ ) )
			{
				blocks_[end].Destruct();
				--blockNum_;
				if( lastA_ > --end )
					lastA_ = end;
			}

			// replace with the back
			MemBlock tmp( blocks_[lastDA_] );
			blocks_[lastDA_] = blocks_[end];
			blocks_[end]     = tmp;
		}

		if( blockNum_ > 4 && blockNum_ <= blockNumReserved_ >> 2 )
		{
			// Reduce size of mem pool if less than a quarter of what is needed.
			//LOGGERF( TEXT("Reducing from %d to %d"), blockNumReserved_, blockNum_ );
			MemBlock* nb = (MemBlock *)malloc( sizeof(MemBlock) * blockNum_ );
			if( !nb )
				return;
			memmove( nb, blocks_, sizeof(MemBlock)*(blockNum_) );
			free( blocks_ );
			blocks_ = nb;
			blockNumReserved_ = blockNum_;
		}
	}
}

inline bool MemoryManager::FixedSizeMemBlockPool::isValid()
{
	// Has it already been used?
	return (blockNum_ != 0);
}

//-------------------------------------------------------------------------

//
// top layer
// Transfer processing to FixedSizeMemBlockPool that matches the specified size
//
// The loki implementation also uses a fixed size allocator, if needed.
// I used to dynamically allocate it, but I stopped doing that because it was troublesome. (^^;
// Even if you allocate 64 at the beginning, it doesn't take up that much memory...
//
MemoryManager* MemoryManager::pUniqueInstance_;
MemoryManager::MemoryManager()
{
	// Clear memory pool to ZERO
	#ifndef STACK_MEM_POOLS
	static MemoryManager::FixedSizeMemBlockPool staticpools[ SMALL_MAX/2 ];
	pools_ = staticpools;
	#endif
	#ifdef STACK_MEM_POOLS
	mem00( pools_, /*sizeof(pools_)*/ sizeof(FixedSizeMemBlockPool) * (SMALL_MAX/2) );
	#endif

	// the only instance is me
	pUniqueInstance_ = this;
}

MemoryManager::~MemoryManager()
{
	// Release all built memory pools, Release all built memory pools
	for( int i=0; i<SMALL_MAX/2; ++i )
		if( pools_[i].isValid() )
			pools_[i].Destruct();

//	delete [] pools_;
}

void* A_HOT MemoryManager::Alloc( size_t siz )
{
	siz = siz + (siz&1);

	// If the size is zero or too large
	// Leave it to the default new operator
	uint i = static_cast<uint>( (siz-1)/2 );
	if( i >= SMALL_MAX/2 )
		return malloc( siz );

	// Multi-thread compatible
	AutoLock al(this);

	// If this is your first time allocating memory of this size,
	// Create a memory pool here.
	if( !pools_[i].isValid() )
	{
		bool ok = pools_[i].Construct( static_cast<ushort>(siz) );
		if( !ok ) return NULL;
	}

	// Assign here
	return pools_[i].Alloc();
}

void A_HOT MemoryManager::DeAlloc( void* ptr, size_t siz )
{
	siz = siz + (siz&1);

	// If the size is zero or too large
	// Defer to default delete operator
	uint i = static_cast<uint>( (siz-1)/2 );
	if( i >= SMALL_MAX/2 )
	{
		::free( ptr );
		return; // You can't do return void in VC...
	}

	// Multi-thread compatible
	AutoLock al(this);

	// release here
	pools_[i].DeAlloc( ptr );
}

#else // USE_ORIGINAL_MEMMAN



MemoryManager* MemoryManager::pUniqueInstance_;

MemoryManager::MemoryManager()
{
	// the only instance is me
	pUniqueInstance_ = this;

}

MemoryManager::~MemoryManager()
{
}

void* MemoryManager::Alloc( size_t siz )
{
	return ::malloc(siz);
}
void MemoryManager::DeAlloc( void* ptr, size_t siz )
{
	::free(ptr);
}

#endif
