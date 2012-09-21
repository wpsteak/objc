//
//  track.m
//  objc
//

#include "objc/runtime.h"
#include "uthash.h"
#include <pthread.h>
#include <unistd.h>


typedef struct {
    Class cls;
    int count;
    UT_hash_handle hh;
} objc_allocations;

static objc_allocations *allocations = NULL;
static pthread_mutex_t allocationLock = PTHREAD_MUTEX_INITIALIZER;
static int track_enabled = 1;
static int track_interval = 5000;


int count_sort(objc_allocations *entrya, objc_allocations *entryb) {
    return entrya->count - entrya->count;
}

static void force_log_allocations()
{
    objc_allocations *entry, *tmp;
    DEBUG_LOG("===================ALLOCATIONS===================");
    HASH_ITER(hh, allocations, entry, tmp)
    {
        if (entry->cls == (Class)0xdeadface)
        {
            DEBUG_LOG("DEADFACE'd objects: %d", entry->count);
        }
        else
        {
            DEBUG_LOG("%d %s ", entry->count, class_getName(entry->cls));
        }
    }
    DEBUG_LOG("=================================================");
}

static void log_allocations()
{
	static int idx = 0;
	if (track_interval > 0 && track_enabled != 0)
	{
		idx = (idx + 1) % track_interval;
		if (idx == 0)
		{
            force_log_allocations();
		}
	}
}

void track_enable(int interval)
{
	track_enabled = 1;
	track_interval = interval;
}

void track_disable()
{
	track_enabled = 0;
}

static Class breakClass = (Class)NULL;
static int minAllocationsToBreakOn = 0;

void break_on_allocations_of_class_above(Class cls, int n) {
    breakClass = cls;
    minAllocationsToBreakOn = n;
}

void break_on_allocation_of_class(Class cls) {
    break_on_allocations_of_class_above(cls, 0);
}

void remove_class_allocation_break() {
    breakClass = (Class)NULL;
}

void maybe_break_on_allocation(Class cls, numAllocs) {
    if (numAllocs >= minAllocationsToBreakOn && cls == breakClass) {
        DEBUG_BREAK();
    }
}

void track_allocation(Class cls)
{
	pthread_mutex_lock(&allocationLock);
	objc_allocations *entry = NULL;
	HASH_FIND_PTR(allocations, &cls, entry);
	if (entry == NULL)
	{
		entry = malloc(sizeof(objc_allocations));
		entry->cls = cls;
		entry->count = 0;
		HASH_ADD_PTR(allocations, cls, entry);
	}
	entry->count = entry->count + 1;
    maybe_break_on_allocation(cls, entry->count);
	log_allocations();
	pthread_mutex_unlock(&allocationLock);
}

void track_deallocation(Class cls)
{
	pthread_mutex_lock(&allocationLock);
	objc_allocations *entry = NULL;
	HASH_FIND_PTR(allocations, &cls, entry);
	if (entry != NULL)
	{
		entry->count = entry->count - 1;
	}
	log_allocations();
	pthread_mutex_unlock(&allocationLock);
}

void track_swizzle(Class from, Class to)
{
	pthread_mutex_lock(&allocationLock);
	objc_allocations *fromEntry = NULL;
	HASH_FIND_PTR(allocations, &from, fromEntry);
	if (fromEntry != NULL)
	{
		objc_allocations *toEntry = NULL;
		HASH_FIND_PTR(allocations, &to, toEntry);
		if (toEntry == NULL)
		{
			toEntry = malloc(sizeof(objc_allocations));
			toEntry->cls = to;
			toEntry->count = 0;
			HASH_ADD_PTR(allocations, cls, toEntry);
		}
		fromEntry->count = fromEntry->count - 1;
		toEntry->count = toEntry->count +1;
	}
	log_allocations();
	pthread_mutex_unlock(&allocationLock);
}
