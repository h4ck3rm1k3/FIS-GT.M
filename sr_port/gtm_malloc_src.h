/****************************************************************
 *								*
 *	Copyright 2001, 2004 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

/* Storage manager for "smaller" pieces of storage. Uses power-of-two
   "buddy" system as described by Knuth. Currently manages pieces of
   size 2K - sizeof(header).

   This include file is included in both gtm_malloc.c and gtm_malloc_dbg.c.
   See the headers of those modules for explanations of how the storage
   manager build is actually accomplished.

   Debugging is controlled via the "gtmdbglvl" environment variable in
   the Unix environment and the GTM$DBGLVL logical in the VMS environment.
   If this variable is set to a non-zero value, the debugging environment
   is enabled. The debugging features turned on will correspond to the bit
   values defined gtmdbglvl.h. Note that this mechanism is versatile enough
   that non-storage-managment debugging is also hooked in here. The
   debugging desired is a mask for the features desired. For example, if the
   value 4 is set, then tracing is enabled. If the value is set to 6, then
   both tracing and statistics are enabled. Because the code is expanded
   twice in a "Pro" build, these debugging features are available even
   in a pro build and can thus be enabled in the field without the need for
   a "debug" version to be installed in order to chase a corruption or other
   problem.
*/

#include "mdef.h"

#include <sys/types.h>
#include <signal.h>
#include <stddef.h>
#include <errno.h>

#include "gtm_stdio.h"
#include "gtm_string.h"
#include "eintr_wrappers.h"
#include "gtmdbglvl.h"
#include "io.h"
#include "iosp.h"
#include "min_max.h"
#include "mdq.h"
#include "error.h"
#include "trans_log_name.h"
#include "gtmmsg.h"
#include "print_exit_stats.h"
#include "gtm_logicals.h"

/* To debug this routine effectively, normally static routines are turned into
   GBLDEFs. */
#ifdef DEBUG
#  define STATICD
#  define STATICR
#else
#  define STATICD static
#  define STATICR static
#endif

/* We are the redefined versions so use real versions in this module */
#undef malloc
#undef free
#include "gtm_stdlib.h"

#ifdef VMS
/* These routines for VMS are AST-safe */
#  define MALLOC(size, addr)						\
{									\
        int	msize, errnum;						\
	void	*maddr;							\
	msize = size;							\
        errnum = lib$get_vm(&msize, &maddr);				\
	if (SS$_NORMAL != errnum)					\
	{								\
		--gtmMallocDepth;					\
		assert(FALSE);						\
		rts_error(VARLSTCNT(4) ERR_VMSMEMORY, 1, msize, errnum);\
	}								\
	addr = (void *)maddr;						\
}
#  define FREE(size, addr)						\
{									\
        int	msize, errnum;						\
	void	*maddr;							\
	msize = size;							\
        maddr = addr;							\
        errnum = lib$free_vm(&msize, &maddr);				\
	if (SS$_NORMAL != errnum)					\
	{								\
		--gtmMallocDepth;					\
		assert(FALSE);						\
		rts_error(VARLSTCNT(3) ERR_FREEMEMORY, 0, errnum);	\
	}								\
}
#  define GTM_MALLOC_REENT
#else
/* These routines for Unix are NOT thread-safe */
#  define MALLOC(size, addr) 						\
{									\
	addr = (void *)malloc(size);					\
	if (NULL == (void *)addr)					\
	{								\
		--gtmMallocDepth;					\
		assert(FALSE);						\
		rts_error(VARLSTCNT(4) ERR_MEMORY, 1, size, errno);	\
	}								\
}
#  define FREE(size, addr) free(addr);
#endif
#ifdef GTM_MALLOC_REENT
#  define GMR_ONLY(statement) statement
#else
#  define GMR_ONLY(statement)
#endif

#ifdef DEBUG
/* States a storage element may be in. Debug version has values less likely to occur naturally
   although the possibilities are limited with only one byte of information. */
enum ElemState {Allocated = 0x42, Free = 0x24};
#else
enum ElemState {Allocated, Free};
#endif

/* Each allocated block has the following structure. The actual address
   returned to the user for 'malloc' and supplied by the user for 'free'
   is actually the storage beginning at the 'userStorage.userStart' area.
   This holds true even for storage that is truely malloc'd. Note that true
   allocated length is kept even in the pro header.
*/
typedef struct storElemStruct
{	/* While the following chars and short are not the best for performance, they enable us
	   to keep the header size to 8 bytes in a pro build. This is important since our minimum
	   allocation size is 16 bytes leaving 8 bytes for data. Also I have not researched what
	   they are, there are a bunch of 8 byte allocates in GT.M that if we were to go to a 16
	   byte header would make the minimum block size 32 bytes thus doubling the storage
	   requirements for these small blocks. SE 03/2002
	*/
	signed char	queueIndex;			/* Index into TwoTable for this size of element */
	unsigned char	state;				/* State of this block */
	unsigned short	extHdrOffset;			/* For MAXTWO sized elements: offset to the
							   header that describes the extent */
	int		realLen;			/* Real (total) length of allocation */
#ifdef DEBUG
	struct	storElemStruct	*fPtr;			/* Next storage element on free/allocated queue */
	struct	storElemStruct	*bPtr;			/* Previous storage element on free/allocated queue */
	unsigned char	*allocatedBy;			/* Who allocated storage */
	int		allocLen;			/* Requested length of allocation */
	unsigned int	smTn;				/* Storage management transaction number allocated at */
	unsigned char	headMarker[4];			/* Header that should not be modified during usage */
	union
	{
		struct storElemStruct *deferFreeNext;	/* Pointer to next deferred free block */
		unsigned char	userStart;		/* First byte of user useable storage */
	} userStorage;
#else
	union						/* In production mode, the links are used only when element is free */
	{
		struct storElemStruct *deferFreeNext;	/* Pointer to next deferred free block */
		struct					/* Free block information */
		{
			struct	storElemStruct	*fPtr;	/* Next storage element on free queue */
			struct	storElemStruct	*bPtr;	/* Previous storage element on free queue */
		} links;
		unsigned char	userStart;		/* First byte of user useable storage */
	} userStorage;
#endif
} storElem;

/* At the end of each block is this header which is used to track when all of the elements that
   a block of real allocated storage was broken into have become free. At that point, we can return
   the chunk to the OS.
*/
typedef struct storExtHdrStruct
{
	struct
	{
		struct storExtHdrStruct	*fl, *bl;	/* In case we need to visit the entire list */
	} links;
	unsigned char	*extentStart;			/* First byte of real extent (not aligned) */
	storElem	*elemStart;			/* Start of array of MAXTWO elements */
	int		elemsAllocd;			/* MAXTWO sized element count. When 0 this block is free */
} storExtHdr;

#define MAXTWO 2048
/* How many "MAXTWO" elements to allocate at one time. This minimizes the waste since our subblocks must
   be aligned on a suitable power of two boundary for the buddy-system to work properly
*/
#define ELEMS_PER_EXTENT 16

#define MAXDEFERQUEUES 10
#ifdef DEBUG
#  define STOR_EXTENTS_KEEP 1 /* Keep only one extent in debug for maximum testing */
#  define MINTWO 64
#  define MAXINDEX 5
#  define STE_FP(p) p->fPtr
#  define STE_BP(p) p->bPtr
#else
#  define STOR_EXTENTS_KEEP 5
#  define MINTWO 16
#  define MAXINDEX 7
#  define STE_FP(p) p->userStorage.links.fPtr
#  define STE_BP(p) p->userStorage.links.bPtr
#endif
/* Our extent must be aligned on a MAXTWO byte boundary hence we allocate one more extent than
   we actually want so we can be guarranteed usable storage. However if that allocation actually
   starts on a MAXTWO boundary (on guarranteed 8 byte boundary), then we get an extra element.
   Here we define our extent size and provide an initial sanity value for "extent_used". If the
   allocator ever gets this extra block, this field will be increased by the size of one element
   to compensate.
*/
#define EXTENT_SIZE ((MAXTWO * (ELEMS_PER_EXTENT + 1)) + sizeof(storExtHdr))
static unsigned int extent_used = (MAXTWO * ELEMS_PER_EXTENT + sizeof(storExtHdr));
/* Following are values used in queueIndex in a storage element. Note that both
   values must be less than zero for the current code to function correctly. */
#define QUEUE_ANCHOR		-1
#define REAL_MALLOC		-2

/* Define number of malloc and free calls we will keep track of */
#define MAXSMTRACE 128

/* Structure where malloc and free call trace information is kept */
typedef struct
{
	unsigned char	*smAddr;	/* Addr allocated or released */
	unsigned int	smSize;		/* Size allcoated or freed */
	unsigned char	*smCaller;	/* Who called malloc/free */
	unsigned int	smTn;		/* What transaction it was */
} smTraceItem;

#ifdef DEBUG
#  define INCR_CNTR(x) ++x
#  define INCR_SUM(x, y) x += y
#  define DECR_CNTR(x) --x
#  define DECR_SUM(x, y) x -= y
#  define SET_MAX(max, tst) {max = MAX(max, tst);}
#  define SET_ELEM_MAX(qtype, idx) SET_MAX(qtype##ElemMax[idx], qtype##ElemCnt[idx])
#  define TRACE_MALLOC(addr,len) {if (GDL_SmTrace & gtmDebugLevel) \
                                          FPRINTF(stderr,"Malloc at %x of %d bytes from %lx\n", addr, len, CALLERID);}
#  define TRACE_FREE(addr,len)       {if (GDL_SmTrace & gtmDebugLevel) \
                                          FPRINTF(stderr,"Free at %x of %d bytes from %lx\n", addr, len, CALLERID);}
#else
#  define INCR_CNTR(x)
#  define INCR_SUM(x, y)
#  define DECR_CNTR(x)
#  define DECR_SUM(x, y)
#  define SET_MAX(max, tst)
#  define SET_ELEM_MAX(qtype, idx)
#  define TRACE_MALLOC(addr, len)
#  define TRACE_FREE(addr, len)
#endif
#ifdef DEBUG_SM
#  define DEBUGSM(x) (PRINTF x, fflush(stdout))
# else
#  define DEBUGSM(x)
#endif
/* Note we use unsigned char * instead of caddr_t for all references to caller_id so the caller id
   is always 4 bytes. On Tru64, caddr_t is 8 bytes which will throw off the size of our
   storage header in debug mode */
#ifdef GTM_MALLOC_DEBUG
#  define CALLERID (smCallerId)
#else
#  define CALLERID ((unsigned char *)caller_id())
#endif

/* Define "routines" to enqueue and dequeue storage elements. Use define so we don't
   have to depend on each implementation's compiler inlining to get efficient code here */
#define ENQUEUE_STOR_ELEM(qtype, idx, elem)		\
{							\
	  storElem *qHdr, *fElem;			\
	  qHdr = &qtype##StorElemQs[idx];		\
	  STE_FP(elem) = fElem = STE_FP(qHdr);		\
	  STE_BP(elem) = qHdr;				\
	  STE_FP(qHdr) = STE_BP(fElem) = elem;		\
	  INCR_CNTR(qtype##ElemCnt[idx]);		\
	  SET_ELEM_MAX(qtype, idx);			\
}

#define DEQUEUE_STOR_ELEM(qtype, elem)			\
{ 							\
	  STE_FP(STE_BP(elem)) = STE_FP(elem);		\
	  STE_BP(STE_FP(elem)) = STE_BP(elem);		\
	  DECR_CNTR(qtype##ElemCnt[elem->queueIndex]);	\
}

#define GET_QUEUED_ELEMENT(sizeIndex, uStor, qHdr, sEHdr) \
{							\
	qHdr = &freeStorElemQs[sizeIndex];		\
	uStor = STE_FP(qHdr);	      			/* First element on queue */ \
	if (QUEUE_ANCHOR != uStor->queueIndex)		/* Does element exist? (Does queue point to itself?) */ \
	{						\
		DEQUEUE_STOR_ELEM(free, uStor);		/* It exists, dequeue it for use */ \
		if (MAXINDEX == sizeIndex)		\
		{	/* Allocating a MAXTWO block. Increment use counter for this subblock's block */ \
			sEHdr = (storExtHdr *)((char *)uStor + uStor->extHdrOffset); \
			++sEHdr->elemsAllocd;		\
		}					\
	} else						\
		uStor = findStorElem(sizeIndex);	\
	assert(0 == ((unsigned long)uStor & (TwoTable[sizeIndex] - 1)));	/* Verify alignment */ \
}


#ifdef INT8_SUPPORTED
#  define ChunkSize 8
#  define ChunkType gtm_int64_t
#  define ChunkValue 0xdeadbeefdeadbeefLL
#else
#  define ChunkSize 4
#  define ChunkType int4
#  define ChunkValue 0xdeadbeef
#endif
#define AddrMask (ChunkSize - 1)

#ifdef DEBUG
/* For debug builds, keep track of the last MAXSMTRACE mallocs and frees. */
GBLDEF volatile int smLastMallocIndex;			/* Index to entry of last malloc-er */
GBLDEF volatile int smLastFreeIndex;			/* Index to entry of last free-er */
GBLDEF smTraceItem smMallocs[MAXSMTRACE];		/* Array of recent allocators */
GBLDEF smTraceItem smFrees[MAXSMTRACE];			/* Array of recent releasers */
GBLDEF volatile unsigned int smTn;			/* Storage management (wrappable) transaction number */
#ifndef PRO_BUILD
GBLREF boolean_t gtmdbglvl_inited;			/* gtmDebugLevel has been initialized but only in debug build*/
#endif
#endif

GBLREF	uint4		gtmDebugLevel;			/* Debug level (0 = using default sm module so with
							   a DEBUG build, even level 0 implies basic debugging) */
GBLREF  int		process_exiting;		/* Process is on it's way out */
GBLREF	volatile int4	gtmMallocDepth;			/* Recursion indicator. Volatile so it gets stored immediately */
/* This var allows us to call ourselves but still have callerid info */
GBLREF	unsigned char	*smCallerId;			/* Caller of top level malloc/free */

STATICD	boolean_t	gtmSmInitialized;		/* Initialized indicator */

#define SIZETABLEDIM MAXTWO/MINTWO
STATICD int size2Index[SIZETABLEDIM];

STATICD readonly struct
{
	unsigned char nullHMark[4];
	unsigned char nullStr[1];
	unsigned char nullTMark[4];
} NullStruct = {0xde, 0xad, 0xbe, 0xef, 0x00, 0xde, 0xad, 0xbe, 0xef};

#ifdef DEBUG
/* Arrays allocated with size of MAXINDEX + 2 are sized to hold an extra
   entry for "real malloc" type allocations. */

STATICD readonly uint4 TwoTable[MAXINDEX + 2] = {64, 128, 256, 512, 1024, 2048, 0xFFFFFFFF};	/* Powers of two element sizes */
STATICD readonly unsigned char markerChar[4] = {0xde, 0xad, 0xbe, 0xef};
#else
STATICD readonly uint4 TwoTable[MAXINDEX + 2] = {16, 32, 64, 128, 256, 512, 1024, 2048, 0xFFFFFFFF};
#endif

STATICD storElem	freeStorElemQs[MAXINDEX + 1];	/* Need full element as queue anchor for dbl-linked
							   list since ptrs not at top of element */
STATICD storExtHdr	storExtHdrQ;			/* List of storage blocks we allocate here */
STATICD int		curExtents;			/* Number of current extents */

#ifdef GTM_MALLOC_REENT
STATICD storElem *deferFreeQueues[MAXDEFERQUEUES];	/* Where deferred (nested) frees are queued for later processing */
STATICD boolean_t deferFreeExists;			/* A deferred free is pending on a queue */
#endif

#ifdef DEBUG
STATICD storElem allocStorElemQs[MAXINDEX + 2];		/* The extra element is for queueing "real" malloc'd entries */
#  ifdef INT8_SUPPORTED
STATICD readonly unsigned char backfillMarkC[8] = {0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef};
#  else
STATICD readonly unsigned char backfillMarkC[4] = {0xde, 0xad, 0xbe, 0xef};
#  endif
#endif

#ifdef DEBUG
/* Define variables used to instrument how our algorithm works */
STATICD	int	totalMallocs;				/* Total malloc requests */
STATICD	int	totalFrees;				/* Total free requests */
STATICD	int	totalRmalloc;				/* Total storage currently (real) malloc'd (includes extent blocks) */
STATICD int	totalExtents;				/* Times we allocated more storage */
STATICD int	maxExtents;				/* Highwater mark of extents */
STATICD	int	rmallocMax;				/* Maximum value of totalRmalloc */
STATICD	int	mallocCnt[MAXINDEX + 2];		/* Malloc count satisfied by each queue size */
STATICD	int	freeCnt[MAXINDEX + 2];			/* Free count for element in each queue size */
STATICD	int	elemSplits[MAXINDEX + 2];		/* Times a given queue size block was split */
STATICD	int	elemCombines[MAXINDEX + 2];		/* Times a given queue block was formed by buddies being recombined */
STATICD	int	freeElemCnt[MAXINDEX + 2];		/* Current count of elements on the free queue */
STATICD	int	allocElemCnt[MAXINDEX + 2];		/* Current count of elements on the allocated queue */
STATICD	int	freeElemMax[MAXINDEX + 2];		/* Maximum number of blocks on the free queue */
STATICD	int	allocElemMax[MAXINDEX + 2];		/* Maximum number of blocks on the allocated queue */
GMR_ONLY(STATICD	int	reentMallocs;)		/* Total number of reentrant mallocs made */
GMR_ONLY(STATICD	int	deferFreePending;)	/* Total number of frees that were deferred */
#endif

/* Macro to return an index into the TwoTable for a given size (round up to next power of two)
   Use the size2Index table to get the proper index. This table is indexed by the number of
   storage "blocks" being requested. A storage block is the size of the smallest power of two
   block we can allocate (size MINTWO) */
#ifdef DEBUG
#  define GetSizeIndex(size) (size ? size2Index[(size - 1) / MINTWO] : assert(FALSE))
#else
#  define GetSizeIndex(size) (size2Index[(size - 1) / MINTWO])
#endif

/* Internal prototypes */
STATICR void gtmSmInit(void);
storElem *findStorElem(int sizeIndex);
#ifdef DEBUG
#include "gtm_malloc.h"
void backfill(unsigned char *ptr, int len);
boolean_t backfillChk(unsigned char *ptr, int len);
#else
void *gtm_malloc_dbg(size_t);
void gtm_free_dbg(void *);
#endif

/* Initialize the storage manangement system. Things to initialize:

   - Initialize size2Index table. This table is used to convert a malloc request size
     to a storage queue index.
   - Initialize queue anchor fwd/bkwd pointers to point to queue anchors so we
     build a circular queue. This allows elements to be added and removed without
     end-of-queue special casing. The queue anchor element is easily recognized because
     it's queue index size will be set to a special value.
   - Initialize debug mode. See if gtm_debug_level environment variable is set and
     retrieve it's value if yes. */
STATICR void gtmSmInit(void)
{
	char		*ascNum;
	storElem	*uStor;
	int		i, sizeIndex, testSize, blockSize;

	error_def(ERR_TRNLOGFAIL);
	error_def(ERR_INVDBGLVL);

	/* WARNING!! Since this is early initialization, the following asserts are not well behaved if they do
	   indeed trip. The best that can be hoped for is they give a condition handler exhausted error on
	   GTM startup. Unfortunately, more intelligent responses are somewhat elusive since no output devices
	   are setup nor (potentially) most of the GTM runtime.
	*/
	assert(MINTWO == TwoTable[0]);
#ifndef PRO_BUILD
	/* For a pro build, this var wasn't set so the assert makes no sense so bypass it */
	assert(gtmdbglvl_inited);
#endif
	/* Check that the storage queue offset in a storage element has sufficient reach
	   to cover an extent.
	*/
	assert(((extent_used - sizeof(storExtHdr)) <= ((1 << (sizeof(uStor->extHdrOffset) * 8)) - 1)));

	/* Initialize size table used to get a storage queue index */
	sizeIndex = 0;
	testSize = blockSize = MINTWO;
	for (i = 0; i < SIZETABLEDIM; i++, testSize += blockSize)
	{
		if (testSize > TwoTable[sizeIndex])
			++sizeIndex;
		size2Index[i] = sizeIndex;
	}

	/* Need to initialize the fwd/bck ptrs in the anchors to point to themselves */
	for (uStor = &freeStorElemQs[0], i = 0; i <= MAXINDEX; ++i, ++uStor)
	{
		STE_FP(uStor) = STE_BP(uStor) = uStor;
		uStor->queueIndex = QUEUE_ANCHOR;
	}
#ifdef DEBUG
	for (uStor = &allocStorElemQs[0], i = 0; i <= (MAXINDEX + 1); ++i, ++uStor)
	{
		STE_FP(uStor) = STE_BP(uStor) = uStor;
		uStor->queueIndex = QUEUE_ANCHOR;
	}
#endif
	dqinit(&storExtHdrQ, links);
	gtmSmInitialized = TRUE;
}

/* Recursive routine used to obtain an element on a given size queue. If no
   elements of that size are available, we recursively call ourselves to get
   an element of the next larger queue which we will then split in half to
   get the one we need and place the remainder back on the free queue of its
   new smaller size. If we run out of queues, we obtain a fresh new 'hunk' of
   storage, carve it up into the largest block size we handle and process as
   before. */
storElem *findStorElem(int sizeIndex)
{
	unsigned char	*uStorAlloc;
	storElem	*uStor, *uStor2, *qHdr;
	storExtHdr	*sEHdr;
	int		hdrSize;
	unsigned int	i;

	VMS_ONLY(error_def(ERR_VMSMEMORY);)
	UNIX_ONLY(error_def(ERR_MEMORY);)

	++sizeIndex;
	if (MAXINDEX >= sizeIndex)
	{	/* We have more queues to search */
		GET_QUEUED_ELEMENT(sizeIndex, uStor, qHdr, sEHdr);

		/* We have a larger than necessary element now so break it in half and put
		   the second half on the queue one size smaller than us */
		INCR_CNTR(elemSplits[sizeIndex]);
		--sizeIndex;					/* Dealing now with smaller element queue */
		assert(sizeIndex >= 0 && sizeIndex < MAXINDEX);
		uStor2 = (storElem *)((unsigned long)uStor + TwoTable[sizeIndex]);
		uStor2->state = Free;
		uStor2->queueIndex = sizeIndex;
		assert(0 == ((unsigned long)uStor2 & (TwoTable[sizeIndex] - 1)));	/* Verify alignment */
#ifdef DEBUG
		memcpy(uStor2->headMarker, markerChar, sizeof(markerChar));	/* Put header tag in place */
		/* Backfill entire block being freed so usage of it will cause problems */
		hdrSize = offsetof(storElem, userStorage);			/* Size of storElem header */
		if (GDL_SmBackfill & gtmDebugLevel)
			backfill((unsigned char *)uStor2 + hdrSize, TwoTable[sizeIndex] - hdrSize);
#endif
		ENQUEUE_STOR_ELEM(free, sizeIndex, uStor2);	/* Place on free queue */
	} else
	{	/* Nothing left to search, [real]malloc a new ALIGNED block of storage and put it on our queues */
		++curExtents;
		SET_MAX(maxExtents, curExtents);
		INCR_CNTR(totalExtents);
		/* Allocate size for one more subblock than we want. This guarrantees us that we can put our subblocks
		   on a power of two boundary necessary for buddy alignment
		*/
		MALLOC(EXTENT_SIZE, uStorAlloc);
		uStor2 = (storElem *)uStorAlloc;
		uStor = (storElem *)(((unsigned long)(uStor2) + MAXTWO - 1) & -MAXTWO); /* Make addr "MAXTWO" byte aligned */
		INCR_SUM(totalRmalloc, EXTENT_SIZE);
		SET_MAX(rmallocMax, totalRmalloc);
		sEHdr = (storExtHdr *)((char *)uStor + (ELEMS_PER_EXTENT * MAXTWO));
		DEBUGSM(("debugsm: Allocating extent at 0x%08lx\n", uStor));

		/* If the storage given to us was aligned, we have ELEMS_PER_EXTENT+1 blocks, else we have
		   ELEMS_PER_EXTENT blocks. We won't put the first element on the queue since that block is
		   being returned to be split.
		*/
		if (uStor == uStor2)
		{
			i = 0;		/* The storage was suitably aligned, we get an extra block free */
			sEHdr = (storExtHdr *)((char *)sEHdr + MAXTWO);
			extent_used = EXTENT_SIZE; /* New max for sanity checks */
		} else
			i = 1;		/* The storage was not aligned. Have planned number of blocks with some waste */
		assert(((char *)sEHdr + sizeof(*sEHdr)) <= ((char *)uStorAlloc + EXTENT_SIZE));
		for (uStor2 = uStor; ELEMS_PER_EXTENT > i; ++i)
		{	/* Place all but first entry on the queue */
			uStor2 = (storElem *)((unsigned long)uStor2 + MAXTWO);
			assert(0 == ((unsigned long)uStor2 & (TwoTable[MAXINDEX] - 1)));	/* Verify alignment */
			uStor2->state = Free;
			uStor2->queueIndex = MAXINDEX;
			uStor2->extHdrOffset = (char *)sEHdr - (char *)uStor2;
			assert(extent_used > uStor2->extHdrOffset);
#ifdef DEBUG
			memcpy(uStor2->headMarker, markerChar, sizeof(markerChar));
			/* Backfill entire block on free queue so we can detect trouble
			   with premature usage or overflow from something else */
			hdrSize = offsetof(storElem, userStorage);			/* Size of storElem header */
			if (GDL_SmBackfill & gtmDebugLevel)
				backfill((unsigned char *)uStor2 + hdrSize, TwoTable[MAXINDEX] - hdrSize);
#endif
			ENQUEUE_STOR_ELEM(free, MAXINDEX, uStor2);	/* Place on free queue */
		}
		uStor->extHdrOffset = (char *)sEHdr - (char *)uStor;
		uStor->state = Free;
		sizeIndex = MAXINDEX;
		/* Set up storage block header */
		sEHdr->extentStart = uStorAlloc;
		sEHdr->elemStart = uStor;
		sEHdr->elemsAllocd = 1;
		dqins(&storExtHdrQ, links, sEHdr);
	}

	assert(sizeIndex >= 0 && sizeIndex <= MAXINDEX);

	uStor->queueIndex = sizeIndex;		/* This is now a smaller block */
	return uStor;
}

#ifdef GTM_MALLOC_REENT
/* Routine to process deferred frees in the deferred free queues */
void processDeferredFrees()
{
	int		dqIndex;
	storElem	*uStor, *uStorNext;
	VMS_ONLY(error_def(ERR_FREEMEMORY);)

	assert(0 == gtmMallocDepth);
	do
	{
		deferFreeExists = FALSE;
		/* Run queue in reverse order so we can process the highest index queues first freeing them
		   up that much sooner. This eliminates the problem of index creep. */
		for (dqIndex = MAXDEFERQUEUES - 1; 0 <= dqIndex; --dqIndex)
		{
			/* Check if queue is empty or not once outside of the gtmMallocDepth lock 'cause
			   we don't want to get the lock unless we really need to */
			if (deferFreeQueues[dqIndex])
			{
				gtmMallocDepth = dqIndex + 2;
				uStor = deferFreeQueues[dqIndex];	/* Dequeue entire chain at this location */
				deferFreeQueues[dqIndex] = NULL;
				gtmMallocDepth = 0;
				for (; uStor; uStor = uStorNext)	/* Release all elements on this queue */
				{
					uStorNext = uStor->userStorage.deferFreeNext;
					gtm_free(&uStor->userStorage.userStart);
				}
			}
		}
	} while (deferFreeExists);
}
#endif


/* Obtain free storage of the given size */
void *gtm_malloc(size_t size)
{
	unsigned char	*retVal;
	storElem 	*uStor, *qHdr;
	storExtHdr	*sEHdr;
	size_t		tSize;
	int		sizeIndex, i, hdrSize;
	unsigned char	*trailerMarker;
	boolean_t	reentered;

	VMS_ONLY(error_def(ERR_VMSMEMORY);)
	UNIX_ONLY(error_def(ERR_MEMORY);)
	error_def(ERR_MEMORYRECURSIVE);

#ifndef DEBUG
	/* If we are not expanding for DEBUG, check now if DEBUG has been turned on.
	   If it has, we are in the wrong module Jack. This IF is structured so that
	   if this is the normal (default/optimized) case we will fall into the code
	   and handle the rerouting at the end. */
	if (GDL_None == gtmDebugLevel)
	{
#endif
		/* Note that this if is also structured for maximum fallthru. The else will
		   be near the end of this entry point */
		if (gtmSmInitialized)
		{
			assert(MAXPOSINT4 >= size);			/* Since unsigned, no negative check needed */
			hdrSize = offsetof(storElem, userStorage);	/* Size of storElem header */
			assert((hdrSize + sizeof(markerChar)) < MINTWO);

			++gtmMallocDepth;				/* Nesting depth of memory calls */
			reentered = (1 < gtmMallocDepth);
#ifndef GTM_MALLOC_REENT
			if (reentered)
			{
				--gtmMallocDepth;
				assert(FALSE);
				rts_error(VARLSTCNT(1) ERR_MEMORYRECURSIVE);
			}
#endif

			INCR_CNTR(totalMallocs);
			INCR_CNTR(smTn);

			/* Validate null string not overwritten */
			assert(0 == memcmp(&NullStruct.nullHMark[0], markerChar, sizeof(markerChar)));
			assert(0 == memcmp(&NullStruct.nullTMark[0], markerChar, sizeof(markerChar)));
#ifdef DEBUG
			GMR_ONLY(if (!reentered))
			{	/* Verify the storage chains before we play */
				VERIFY_STORAGE_CHAINS;
			}
#endif

			if (0 != size)
			{
				GMR_ONLY(size = MAX(sizeof(char *), size);)	/* Need room for deferred free next pointer */
				tSize = size + hdrSize;				/* Add in header size */
#ifdef DEBUG
				tSize += sizeof(markerChar);			/* Add in room for trailer label */
				/* If being a storage hog, we want to make sure we have plenty of room for
				   filler. For strings up to MAXTWO in length, we pad with an additional 50%
				   of storage with a minimum of 32 bytes and a maximum of 256 bytes. For larger
				   strings, we pad with 256 bytes. Since selecting GDL_SmStorHog also turns on
				   GDL_SmBackfill and GDL_SmChkAllocBackfill, this padding will be backfilled and
				   checked during allocate storage validation calls. */
				if (GDL_SmStorHog & gtmDebugLevel)
				{
					if (MAXTWO >= size)
						tSize += MIN(MAX(size / 2, 32), 256);
					else
						tSize += 256;
				}
#endif
				if (MAXTWO >= tSize GMR_ONLY(&& !reentered))
				{	/* Use our memory manager for smaller pieces */
					sizeIndex = GetSizeIndex(tSize);		/* Get index to size we need */
					assert(sizeIndex >= 0 && sizeIndex <= MAXINDEX);
					GET_QUEUED_ELEMENT(sizeIndex, uStor, qHdr, sEHdr);
					uStor->realLen = TwoTable[sizeIndex];
				} else
				{	/* Use regular malloc to obtain the piece */
					MALLOC(tSize, uStor);
					INCR_SUM(totalRmalloc, tSize);
					SET_MAX(rmallocMax, totalRmalloc);

					uStor->queueIndex = REAL_MALLOC;
					uStor->realLen = tSize;
#ifdef DEBUG
					sizeIndex = MAXINDEX + 1;	/* Just so the ENQUEUE below has a queue since
									   we usually use -1 as the "real" queueindex
									   for malloc'd storage */
#endif
				}
				INCR_CNTR(mallocCnt[sizeIndex]);
				uStor->state = Allocated;
#ifdef DEBUG
				/* Fill in extra debugging fields in header */
				uStor->allocatedBy = CALLERID;				/* Who allocated us */
				uStor->allocLen = size;					/* User requested size */
				memcpy(uStor->headMarker, markerChar, sizeof(markerChar));
				trailerMarker = (unsigned char *)&uStor->userStorage.userStart + size;	/* Where to put trailer */
				memcpy(trailerMarker, markerChar, sizeof(markerChar));	/* Small trailer */
				if (GDL_SmBackfill & gtmDebugLevel)
				{	/* Use backfill method of after-allocation metadata */
					backfill(trailerMarker + sizeof(markerChar),
						uStor->realLen - size - hdrSize - sizeof(markerChar));
				}

				uStor->smTn = smTn;					/* transaction number */
				GMR_ONLY(if (!reentered))
				{
					ENQUEUE_STOR_ELEM(alloc, sizeIndex, uStor);
				}
#ifdef GTM_MALLOC_REENT
				else
				{	/* Reentrant allocates cannot be put on our allocated queue -- sorry too dangerous */
					uStor->fPtr = uStor->bPtr = NULL;
					INCR_CNTR(allocElemCnt[sizeIndex]);
					INCR_CNTR(reentMallocs);
				}
#endif
#endif
				retVal = &uStor->userStorage.userStart;
				assert(((long)retVal & (long)-8) == (long)retVal);	/* Assert we have an 8 byte boundary */
			} else	/* size was 0 */
			{
				retVal = &NullStruct.nullStr[0];
			}
#ifdef DEBUG
			/* Record this transaction in debugging history */
			++smLastMallocIndex;
			if (MAXSMTRACE <= smLastMallocIndex)
				smLastMallocIndex = 0;
			smMallocs[smLastMallocIndex].smAddr = retVal;
			smMallocs[smLastMallocIndex].smSize = size;
			smMallocs[smLastMallocIndex].smCaller = CALLERID;
			smMallocs[smLastMallocIndex].smTn = smTn;
#endif
			TRACE_MALLOC(retVal, size);

			--gtmMallocDepth;
#ifdef GTM_MALLOC_REENT
			/* Check on deferred frees */
			if (0 == gtmMallocDepth && deferFreeExists)
				processDeferredFrees();
#endif
			return retVal;
		} else  /* Storage mgmt has not been initialized */
		{
			gtmSmInit();
			/* Reinvoke gtm_malloc now that we are initialized. Note that this one time (the first
			   call to malloc), we will not record the proper caller id in the storage header or in
			   the traceback table. The caller will show up as gtm_malloc(). However, all subsequent
			   calls will be correct.
			*/
			return (void *)gtm_malloc(size);
		}
#ifndef DEBUG
	} else
	{	/* We have a non-DEBUG module but debugging is turned on so redirect the call to the appropriate module */
		smCallerId = (unsigned char *)caller_id();
		return (void *)gtm_malloc_dbg(size);
	}
#endif
}

/* Release the free storage at the given address */
void gtm_free(void *addr)
{
	storElem 	*uStor, *buddyElem;
	storExtHdr	*sEHdr;
	unsigned char	*trailerMarker;
	int 		sizeIndex, hdrSize, saveIndex, dqIndex, saveSize, freedElemCnt;

	error_def(ERR_MEMORYRECURSIVE);
	VMS_ONLY(error_def(ERR_VMSMEMORY);)
	VMS_ONLY(error_def(ERR_FREEMEMORY);)
	UNIX_ONLY(error_def(ERR_MEMORY);)

#ifndef DEBUG
	/* If we are not expanding for DEBUG, check now if DEBUG has been turned on.
	   If it has, we are in the wrong module Jack. This IF is structured so that
	   if this is the normal (optimized) case we will fall into the code and
	   handle the rerouting at the end. */
	if (GDL_None == gtmDebugLevel)
	{
#endif
		if (!gtmSmInitialized)	/* Storage must be init'd before can free anything */
			GTMASSERT;
		if (process_exiting)	/* If we are exiting, don't bother with frees. Process destruction can do it */
			return;

		++gtmMallocDepth;	/* Recursion indicator */

#ifdef GTM_MALLOC_REENT
		/* If we are attempting to do a reentrant free, we will instead put the free on a queue to be released
		   at a later time. Ironically, since we cannot be sure of any queues of available blocks, we have to
		   malloc a small block to carry this info which we will free with the main storage */
		if (1 < gtmMallocDepth)
		{
			if ((unsigned char *)addr != &NullStruct.nullStr[0])
			{
				dqIndex = gtmMallocDepth - 2;		/* 0 origin index into defer queues */
				if (MAXDEFERQUEUES <= dqIndex)		/* Can't run out of queues */
					GTMASSERT;
				hdrSize = offsetof(storElem, userStorage);
				uStor = (storElem *)((unsigned long)addr - hdrSize);		/* Backup ptr to element header */
				uStor->userStorage.deferFreeNext = deferFreeQueues[dqIndex];
				deferFreeQueues[dqIndex] = uStor;
				deferFreeExists = TRUE;
				INCR_CNTR(deferFreePending);
			}
			--gtmMallocDepth;
			return;
		}
#else
		if (1 < gtmMallocDepth)
		{
			--gtmMallocDepth;
			assert(FALSE);
			rts_error(VARLSTCNT(1) ERR_MEMORYRECURSIVE);
		}
#endif
		INCR_CNTR(smTn);	/* Bump the transaction number */

		/* Validate null string not overwritten */
		assert(0 == memcmp(&NullStruct.nullHMark[0], markerChar, sizeof(markerChar)));
		assert(0 == memcmp(&NullStruct.nullTMark[0], markerChar, sizeof(markerChar)));
#ifdef DEBUG
		/* verify chains before we attempt dequeue */
		VERIFY_STORAGE_CHAINS;
#endif
		INCR_CNTR(totalFrees);
		if ((unsigned char *)addr != &NullStruct.nullStr[0])
		{
			hdrSize = offsetof(storElem, userStorage);
			uStor = (storElem *)((unsigned long)addr - hdrSize);		/* Backup ptr to element header */
			sizeIndex = uStor->queueIndex;
#ifdef DEBUG
			TRACE_FREE(addr, uStor->allocLen);
			saveSize = uStor->allocLen;
			/* Extra checking for debugging. Note that these sanity checks are only done in debug
			   mode. The thinking is that we will bypass the checks in the general case for speed but
			   if we really need to chase a storage related problem, we should switch to the debug version
			   in the field to turn on these and other checks.
			*/
			assert(Allocated == uStor->state);
			assert(0 == memcmp(uStor->headMarker, markerChar, sizeof(markerChar)));
			trailerMarker = (unsigned char *)&uStor->userStorage.userStart + uStor->allocLen;/* Where trailer was put */
			assert(0 == memcmp(trailerMarker, markerChar, sizeof(markerChar)));
			if (GDL_SmChkAllocBackfill & gtmDebugLevel)
			{	/* Use backfill check method for after-allocation metadata */
				assert(backfillChk(trailerMarker + sizeof(markerChar),
					uStor->realLen - uStor->allocLen - hdrSize - sizeof(markerChar)));
			}

			/* Remove element from allocated queue unless element is from a reentered malloc call. In that case, just
			   manipulate the counters */
			if (NULL != uStor->fPtr)
			{
				if (0 <= uStor->queueIndex)
				{
					DEQUEUE_STOR_ELEM(alloc, uStor);
				} else
				{	/* shenanigans so that counts are maintained properly in debug mode */
					saveIndex = uStor->queueIndex;
					uStor->queueIndex = MAXINDEX + 1;
					DEQUEUE_STOR_ELEM(alloc, uStor);
					uStor->queueIndex = saveIndex;
				}
			} else
				DECR_CNTR(allocElemCnt[((0 <= uStor->queueIndex) ? uStor->queueIndex : MAXINDEX + 1)]);
#endif
			if (sizeIndex >= 0)
			{	/* We can put the storage back on one of our simple queues */
				assert(0 == ((unsigned long)uStor & (TwoTable[sizeIndex] - 1)));	/* Verify alignment */
				assert(sizeIndex >= 0 && sizeIndex <= MAXINDEX);
				uStor->state = Free;
				INCR_CNTR(freeCnt[sizeIndex]);

			        /* First, if there are larger queues than this one, see if it has a buddy that it can
				   combine with */
				while (sizeIndex < MAXINDEX)
				{
					buddyElem = (storElem *)((unsigned long)uStor ^ TwoTable[sizeIndex]);/* Address of buddy */
					assert(0 == ((unsigned long)buddyElem & (TwoTable[sizeIndex] - 1)));/* Verify alignment */
					assert(buddyElem->state == Allocated || buddyElem->state == Free);
					assert(buddyElem->queueIndex >= 0 && buddyElem->queueIndex <= sizeIndex);
					if (buddyElem->state == Allocated || buddyElem->queueIndex != sizeIndex)
						/* All possible combines done */
						break;

					/* Remove buddy from its queue and make a larger element for a larger queue */
					DEQUEUE_STOR_ELEM(free, buddyElem);
					if (buddyElem < uStor)		/* Pick lower address buddy for top of new bigger block */
						uStor = buddyElem;
					++sizeIndex;
					assert(sizeIndex >= 0 && sizeIndex <= MAXINDEX);
					INCR_CNTR(elemCombines[sizeIndex]);
					uStor->queueIndex = sizeIndex;
				}
#ifdef DEBUG
				/* Backfill entire block being freed so usage of it will cause problems */
				if (GDL_SmBackfill & gtmDebugLevel)
					backfill((unsigned char *)uStor + hdrSize, TwoTable[sizeIndex] - hdrSize);
#endif
				ENQUEUE_STOR_ELEM(free, sizeIndex, uStor);
				if (MAXINDEX == sizeIndex)
				{	/* Freeing/Coagulating a MAXTWO block. Decrement use counter for this element's block */
					sEHdr = (storExtHdr *)((char *)uStor + uStor->extHdrOffset);
					--sEHdr->elemsAllocd;
					assert(0 <= sEHdr->elemsAllocd);
					/* Check for an extent being ripe for return to the system. Requirements are:
					     1) All subblocks must be free (elemsAllocd == 0).
					     2) There must be more than STOR_EXTENTS_KEEP extents already allocated.
					   If these conditions are met, we will dequeue each individual element from
					   it's queue and release the entire extent in a (real) free.
					*/
					if (STOR_EXTENTS_KEEP < curExtents && 0 == sEHdr->elemsAllocd)
					{	/* Release this extent */
						DEBUGSM(("debugsm: Extent being freed from 0x%08lx\n", sEHdr->elemStart));
						DEBUG_ONLY(freedElemCnt = 0);
						for (uStor = sEHdr->elemStart;
						     (char *)uStor < (char *)sEHdr;
						     uStor = (storElem *)((char *)uStor + MAXTWO))
						{
							DEBUG_ONLY(++freedElemCnt);
							assert(Free == uStor->state);
							assert(MAXINDEX == uStor->queueIndex);
							DEQUEUE_STOR_ELEM(free, uStor);
							DEBUGSM(("debugsm: ... element removed from free q 0x%08lx\n", uStor));
						}
						assert(ELEMS_PER_EXTENT <= freedElemCnt);	/* one loop to free them all */
						assert((char *)uStor == (char *)sEHdr);
						dqdel(sEHdr, links);
						FREE(EXTENT_SIZE, sEHdr->extentStart);
						--curExtents;
						assert(curExtents);
					}
				}

			} else
			{
				TRACE_FREE(addr, 0);
				DEBUG_ONLY(saveSize = 0);
				assert(REAL_MALLOC == sizeIndex);		/* Better be a real malloc type block */
				INCR_CNTR(freeCnt[MAXINDEX + 1]);		/* Count free of malloc */
				DECR_SUM(totalRmalloc, uStor->realLen);
				FREE(uStor->realLen, uStor);
			}
		}
#ifdef DEBUG
		/* Make trace entry for this free */
		++smLastFreeIndex;
		if (MAXSMTRACE <= smLastFreeIndex)
			smLastFreeIndex = 0;
		smFrees[smLastFreeIndex].smAddr = addr;
		smFrees[smLastFreeIndex].smSize = saveSize;
		smFrees[smLastFreeIndex].smCaller = CALLERID;
		smFrees[smLastFreeIndex].smTn = smTn;
#endif
		--gtmMallocDepth;
#ifdef GTM_MALLOC_REENT
		/* Check on deferred frees */
		if (0 == gtmMallocDepth && deferFreeExists)
			processDeferredFrees();
#endif
#ifndef DEBUG
	} else
	{	/* If not a debug module and debugging is enabled, reroute call to
		   the debugging version. */
		smCallerId = (unsigned char *)caller_id();
		gtm_free_dbg(addr);
	}
#endif
}


#ifdef DEBUG
/* Backfill the requested area with marker text. We do this by doing single byte
   stores up to the point where we can do aligned stores of the native register
   length. Then fill the area as much as possible and finish up potentially with
   a few single byte unaligned bytes at the end. */
void backfill(unsigned char *ptr, int len)
{
	unsigned char	*c;
	ChunkType	*chunkPtr;
	int		unalgnLen, chunkCnt;

	assert(0 <= len);
	if (0 != len)
	{
		/* Process unaligned portion first */
		unalgnLen = (unsigned long)ptr & AddrMask;	/* Past an alignment point */
		if (unalgnLen)
		{
			unalgnLen = ChunkSize - unalgnLen;	/* How far to go to get to alignment point */
			unalgnLen = MIN(unalgnLen, len);	/* Make sure not going too far */
			c = backfillMarkC;
			len -= unalgnLen;
			do
			{
				*ptr++ = *c++;
				--unalgnLen;
			} while(unalgnLen);
		}

		/* Now, do aligned portion */
		assert(0 == ((unsigned long)ptr & AddrMask));	/* Verify aligned */
		chunkCnt = len / ChunkSize;
		chunkPtr = (ChunkType *)ptr;
		while (chunkCnt--)
		{
			*chunkPtr++ = ChunkValue;
			len -= sizeof(ChunkType);
		}

		/* Do remaining unaligned portion if any */
		if (len)
		{
			ptr = (unsigned char *)chunkPtr;
			c = backfillMarkC;
			do
			{
				*ptr++ = *c++;
				--len;
			} while(len);
		}
	}
}

/*  ** still under ifdef DEBUG ** */
/* Check the given backfilled area that it was filled in exactly as
   the above backfill routine would have filled it in. Again, do any
   unaligned single chars first, then aligned native length areas,
   then any stragler unaligned chars */
boolean_t backfillChk(unsigned char *ptr, int len)
{
	unsigned char	*c;
	ChunkType	*chunkPtr;
	int		unalgnLen, chunkCnt;

	if (0 != len)
	{
		/* Process unaligned portion first */
		unalgnLen = (unsigned long)ptr & AddrMask;	/* Past an alignment point */
		if (unalgnLen)
		{
			unalgnLen = ChunkSize - unalgnLen;	/* How far to go to get to alignment point */
			unalgnLen = MIN(unalgnLen, len);	/* Make sure not going too far */
			c = backfillMarkC;
			len -= unalgnLen;
			do
			{
				if (*ptr++ == *c++)
					--unalgnLen;
				else
					return FALSE;
			} while(unalgnLen);
		}

		/* Now, do aligned portion */
		assert(0 == ((unsigned long)ptr & AddrMask));	/* Verify aligned */
		chunkCnt = len / ChunkSize;
		chunkPtr = (ChunkType *)ptr;
		while (chunkCnt--)
		{
			if (*chunkPtr++ == ChunkValue)
				len -= sizeof(ChunkType);
			else
				return FALSE;
		}

		/* Do remaining unaligned portion if any */
		if (len)
		{
			ptr = (unsigned char *)chunkPtr;
			c = backfillMarkC;
			do
			{
				if (*ptr++ == *c++)
					--len;
				else
					return FALSE;
			} while(len);
		}
	}
	return TRUE;
}


/*  ** still under ifdef DEBUG ** */
/* Routine to run the free storage chains to verify that everything is in the correct place */
void verifyFreeStorage(void)
{
	storElem	*eHdr, *uStor;
	uint4		i;
	int		hdrSize;

	hdrSize = offsetof(storElem, userStorage);
	/* Looping for each free queue */
	for (eHdr = &freeStorElemQs[0], i = 0; i <= MAXINDEX; ++i, ++eHdr)
	{
		for (uStor = STE_FP(eHdr); uStor->queueIndex != QUEUE_ANCHOR; uStor = STE_FP(uStor))
		{
			assert(0 == ((unsigned long)uStor & (TwoTable[i] - 1)));			/* Verify alignment */
			assert(Free == uStor->state);							/* Verify state */
			assert(0 == memcmp(uStor->headMarker, markerChar, sizeof(markerChar)));		/* Verify metadata marker */
			assert(MAXINDEX != i || extent_used > uStor->extHdrOffset);
			if (GDL_SmChkFreeBackfill & gtmDebugLevel)
			{	/* Use backfill check method for verifying freed storage is untouched */
				assert(backfillChk((unsigned char *)uStor + hdrSize, TwoTable[i] - hdrSize));
			}
		}
	}
}


/*  ** still under ifdef DEBUG ** */
/* Routine to run the allocated chains to verify that the markers are all still in place */
void verifyAllocatedStorage(void)
{
	storElem	*eHdr, *uStor;
	unsigned char	*trailerMarker;
	uint4		i;
	int		hdrSize;

	hdrSize = offsetof(storElem, userStorage);
	/* Looping for MAXINDEX+1 will check the real-malloc'd chains too */
	for (eHdr = &allocStorElemQs[0], i = 0; i <= (MAXINDEX + 1); ++i, ++eHdr)
	{
		for (uStor = STE_FP(eHdr); uStor->queueIndex != QUEUE_ANCHOR; uStor = STE_FP(uStor))
		{
			if (i != MAXINDEX + 1)							/* If not verifying real mallocs,*/
				assert(0 == ((unsigned long)uStor & (TwoTable[i] - 1)));	/* .. verify alignment */
			assert(Allocated == uStor->state);					/* Verify state */
			assert(0 == memcmp(uStor->headMarker, markerChar, sizeof(markerChar)));	/* Verify metadata markers */
			trailerMarker = (unsigned char *)&uStor->userStorage.userStart+uStor->allocLen;/* Where  trailer was put */
			assert(0 == memcmp(trailerMarker, markerChar, sizeof(markerChar)));
			assert(MAXINDEX != i || extent_used > uStor->extHdrOffset);
			if (GDL_SmChkAllocBackfill & gtmDebugLevel)
			{	/* Use backfill check method for after-allocation metadata */
				assert(backfillChk(trailerMarker + sizeof(markerChar),
					uStor->realLen - uStor->allocLen - hdrSize - sizeof(markerChar)));
			}
		}
	}
}


/*  ** still under ifdef DEBUG ** */
/* Routine to print the end-of-process info -- either allocation statistics or malloc trace dump.
   Note that the use of FPRINTF here instead of util_out_print is historical. The output was at one
   time going to stdout and util_out_print goes to stderr. If necessary or desired, these could easily
   be changed to use util_out_print instead of FPRINTF
*/
void printMallocInfo(void)
{
	storElem	*eHdr, *uStor;
	int 		i, j;

	if (GDL_SmStats & gtmDebugLevel)
	{
		FPRINTF(stderr,"\nMalloc small storage performance:\n");
		FPRINTF(stderr,
			"Total mallocs: %d, total frees: %d, total extents: %d, total rmalloc bytes: %d, max rmalloc bytes: %d\n",
			totalMallocs, totalFrees, totalExtents, totalRmalloc, rmallocMax);
		FPRINTF(stderr,
			"Maximum extents: %d, Current extents: %d, Released extents: %d\n", maxExtents, curExtents,
			(totalExtents - curExtents));
		GMR_ONLY(FPRINTF(stderr,"Total reentrant mallocs: %d, total deferred frees: %d\n", reentMallocs, deferFreePending);)
		FPRINTF(stderr,"\nQueueSize   Mallocs     Frees    Splits  Combines    CurCnt    MaxCnt    CurCnt    MaxCnt\n");
		FPRINTF(stderr,  "                                                      Free      Free      Alloc     Alloc\n");
		FPRINTF(stderr,  "-----------------------------------------------------------------------------------------\n");
		{
			for (i = 0; i <= MAXINDEX + 1; ++i)
			{
				FPRINTF(stderr,
					"%9d %9d %9d %9d %9d %9d %9d %9d %9d\n", TwoTable[i], mallocCnt[i], freeCnt[i],
					elemSplits[i], elemCombines[i], freeElemCnt[i], freeElemMax[i],
					allocElemCnt[i], allocElemMax[i]);
			}
		}
	}
	if (GDL_SmDumpTrace & gtmDebugLevel)
	{
		FPRINTF(stderr,"\nMalloc Storage Traceback:\n");
		FPRINTF(stderr,"TransNumber   AllocAddr        Size   CallerAddr\n");
		FPRINTF(stderr,"------------------------------------------------\n");
		for (i = 0,j = smLastMallocIndex; i < MAXSMTRACE; ++i,--j)/* Loop through entire table, start with last elem used */
		{
			if (0 > j)					   /* Wrap as necessary */
				j = MAXSMTRACE - 1;
			if (0 != smMallocs[j].smTn)
				FPRINTF(stderr,"%9d    0x%08lx  %10d   0x%08lx\n", smMallocs[j].smTn, smMallocs[j].smAddr,
					smMallocs[j].smSize, smMallocs[j].smCaller);
		}
		FPRINTF(stderr,"\n\nFree Storage Traceback:\n");
		FPRINTF(stderr,"TransNumber    FreeAddr        Size   CallerAddr\n");
		for (i = 0, j = smLastFreeIndex; i < MAXSMTRACE; ++i, --j)/* Loop through entire table, start with last elem used */
		{
			if (0 > j)					  /* Wrap as necessary */
				j = MAXSMTRACE - 1;
			if (0 != smFrees[j].smTn)
				FPRINTF(stderr,"%9d    0x%08lx  %10d   0x%08lx\n", smFrees[j].smTn, smFrees[j].smAddr,
					smFrees[j].smSize, smFrees[j].smCaller);
		}
		FPRINTF(stderr,"\n");
	}
	if (GDL_SmDump & gtmDebugLevel)
	{
		FPRINTF(stderr,"\nMalloc Storage Dump:\n");
		FPRINTF(stderr,"Malloc Addr      Alloc From     Malloc Size\n");
		FPRINTF(stderr,"-------------------------------------------\n");
		/* Looping for each allocated queue */
		for (eHdr = &allocStorElemQs[0], i = 0; i <= (MAXINDEX + 1); ++i, ++eHdr)
		{
			for (uStor = STE_FP(eHdr); uStor->queueIndex != QUEUE_ANCHOR; uStor = STE_FP(uStor))
			{
				FPRINTF(stderr, "0x%08lx       0x%08lx      %10d\n", &uStor->userStorage.userStart,
					uStor->allocatedBy, uStor->allocLen);
			}
		}
	}

}
#endif
