/** general includes */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

/** personnal includes **/
#include "fc_fifo.h"
#include "fc_semaphore.h"
#include "fc_tools.h"

/** For FC_INIT and FC_LDYN added into FC_FIFO structure. */
/** Both are big-sized structures so they are not passed in
 *  fifo communication buffer, which has fixed-size elements,
 *  but embedded directly into the FC_FIFO control. */
#include "fc_com.h"

/** access for users **/
#define FC_USER_ALL  ((1<<6)&(1<<7)&(1<<8))
#define FC_GROUP_ALL ((1<<3)&(1<<4)&(1<<5))
#define FC_OTHER_ALL ((1<<0)&(1<<1)&(1<<2))
#define FC_ALL_ALL (FC_USER_ALL&FC_GROUP_ALL&FC_OTHER_ALL)

#define MAX_LDYN    256

#define ACQUIRE_LOCK()     {    if (!fifo->singleMode) \
                                      { \
                                            fc_semaphore_get((FC_Semaphore volatile*)(&(fifo->sem)), pid); \
                                      } \
                                  }

#define RELEASE_LOCK()      {   if (!fifo->singleMode) \
                                      { \
                                            fc_semaphore_put((FC_Semaphore volatile*)(&(fifo->sem)), pid); \
                                      } \
                                  }

#define SLEEP()               { \
                                      struct timeval tv; \
                                      tv.tv_sec = 0; \
                                      tv.tv_usec = 10000; \
                                      select(0, NULL, NULL, NULL, &tv); \
                                  }

typedef struct
{
    FC_INIT init;
    FC_LDYN ldyn[MAX_LDYN];
    FC_LDYN *lastLdynPtr;       // pointer to last ldyn[] structure read
    unsigned int numEntry;      // number of entries in data
    unsigned int entrySize;     // size of each entry in data
    unsigned int numData;       // number of available data
    unsigned int readIdx;       // index in data
    unsigned int writeIdx;      // index in data
    unsigned int singleMode;
    unsigned int refCount;      // reference count
    FC_Semaphore sem;
    unsigned char data[0];      // size is numEntry * entrySize
} _fifo_control;

/* create a shared memory buffer */
static int fc_fifo_create_buffer(unsigned int size)
{
    int ret;

    /* try to get a shared memory buffer with the given size */
    ret = shmget(IPC_PRIVATE, size, 0777);

    /* cannot process */
    if (ret < 0)
    {
        fc_message("cannot create shared buffer with size '%d'", size);
        fc_message("error returned: %s", strerror(errno));
        return (ret);
    }

    /* ok */
    fc_debug("shared buffer of size '%d' created (id=%d)", size, ret);
    return (ret);
}

/* connect to an existing FIFO. returns the FIFO or FC_FIFO_NDEF.
   the 'id' is the ID given by 'fc_fifo_create' */
FC_FIFO fc_fifo_connect(unsigned int id)
{
    void *ret;

    /* invalid argument */
    if ((int) id < 0)
    {
        fc_message("invalid shared buffer Id (%d)", id);
        return (NULL);
    }

    /* try to attach the buffer */
    ret = shmat(id, NULL, 0700);
    if ((ret == NULL) || (ret == (void*) - 1))
    {/* cannot process */
        fc_message("cannot attach shared buffer '%d'", id);
        fc_message("error returned: %s", strerror(errno));
        return (NULL);
    }

    /* ok */
    fc_debug("shared buffer '%d' mapped at %p", id, ret);

    /* reference ourself as client */
    ((_fifo_control*)ret)->refCount++;

    return (ret);
}

/* create a new FIFO. returns the FIFO or 'FC_FIFO_NDEF' on error
   'esize' is the size of each element in the buffer, and 'id' is set to
   the ID of the shared buffer, for connections. 'single' is TRUE
   if the FIFO will be used with only one writer, and FALSE if
   many writers can access the FIFO (with fc_fifo_write_single) */
FC_FIFO fc_fifo_create(unsigned int elements, unsigned int esize, unsigned int *id, unsigned int single)
{
    int ret;
    FC_FIFO mem;

    if (elements <= 4)
    {
        /* default size */
        elements = 32 * 1024;
    }

    /* get a shared buffer */
    ret = fc_fifo_create_buffer(elements * esize + sizeof (_fifo_control));
    *id = ret;
    /* error */
    if (ret < 0)
    {
        return (NULL);
    }

    /* attach the buffer */
    mem = fc_fifo_connect(ret);
    /* error */
    if (mem == NULL)
    {
        return (NULL);
    }

    memset(&((_fifo_control*)mem)->init, 0, sizeof(((_fifo_control*)mem)->init));
    memset(&((_fifo_control*)mem)->ldyn, 0, sizeof(((_fifo_control*)mem)->ldyn));

    /* initialize the control part of the buffer */
    ((_fifo_control*) mem)->numEntry = elements;
    ((_fifo_control*) mem)->entrySize = esize;
    ((_fifo_control*) mem)->numData = 0;
    ((_fifo_control*) mem)->readIdx = 0;
    ((_fifo_control*) mem)->writeIdx = 0;
    ((_fifo_control*) mem)->singleMode = single;
    ((_fifo_control*)mem)->refCount = 1;
    fc_semaphore_init(&(((_fifo_control*) mem)->sem));

    /* ok */
    return (mem);
}

/* deconnect the given FIFO. once done, no more access to the FIFO
   is possible.
   delete flag if shm has no more reference. */
int fc_fifo_close(FC_FIFO buffer, unsigned int shmid, unsigned int delete)
{
    unsigned int ref;
    volatile _fifo_control *fifo = (volatile _fifo_control *)buffer;

    ref = fifo->refCount;
    if (ref == 0)
    {
        fc_message("Closing fifo with reference count of 0! (must be 1 minimum).\n");
    }
    else
    {
        /* remove our reference */
        ref = --fifo->refCount;
    }

    /* detach the buffer */
    shmdt((const void*)fifo);

    /* delete shared memory */
    if (delete && ref == 0)
        shmctl(shmid, IPC_RMID, NULL);

    return (1);
}

/* Return direct pointer to data instead of copying to user buffer.
   Returns FALSE if EOF is reached before the end of the read.
   No lock is being acquired when modifying FIFO state.
   This function is not thread-safe. */
void* fc_fifo_read_single(FC_FIFO buffer, unsigned int size, unsigned int pid)
{
    volatile _fifo_control *fifo = (volatile _fifo_control *)buffer;

    /* check buffer size */
    if (size > fifo->entrySize)
    {
        fc_message("read %d bytes from buffer is greater than FIFO element size (%d bytes).\n", size, fifo->entrySize);
        return (0);
    }

    for(;;)
    {
        ACQUIRE_LOCK();

        /* check if data is available for reading */
        if ( fifo->numData == 0 )
        {
            /* alone already */
            if (fifo->refCount <= 1)
            {
                // There is a race condition in the check above: fifo->numData == 0.
                // Since the read and compare to zero operation is not atomic,
                // it is possible after we read the value and during compare,
                // then fc_fifo_write_single_done() increment it (e.g., write data to fifo)
                // then process writing to fifo exit causing refCount to be 1.
                // Here we do a second check to make sure there is really no more data left.

                SLEEP();

                if (fifo->numData > 0)
                {
                    break;
                }

                // Really no more data,
                // return NULL.
                RELEASE_LOCK();
                return 0;
            }

            RELEASE_LOCK();

            SLEEP();
        }
        else
        {
            break;
        }
    }

    return (void*)( fifo->data + fifo->readIdx * fifo->entrySize );
}

/* Return direct pointer to data instead of copying to user buffer.
   This function is not thread-safe. */
void fc_fifo_read_single_done(FC_FIFO buffer, unsigned int pid)
{
    volatile _fifo_control *fifo = (volatile _fifo_control *)buffer;

    // operation to numData must be atomic since it
    // is incremented and decremented by at least 2 separate process
    // (fcmanager and process using libfc.so) even in single mode.
    //fifo->numData--;
    __sync_fetch_and_sub(&fifo->numData, 1);

    if ( fifo->readIdx >= (fifo->numEntry-1) )
    {
        fifo->readIdx = 0;
    }
    else
    {
        fifo->readIdx++;
    }

    RELEASE_LOCK();
}

/* write 'size' bytes to 'fifo', from 'buffer', for the process 'ID' */
void* fc_fifo_write_single(FC_FIFO buffer, unsigned int size, unsigned int pid)
{
    volatile _fifo_control *fifo = (volatile _fifo_control *)buffer;

    /* check buffer size */
    if (size > fifo->entrySize)
    {
        fc_message("write %d bytes to buffer is greater than FIFO element size (%d bytes).\n", size, fifo->entrySize);
        return (0);
    }

    for(;;)
    {
        ACQUIRE_LOCK();

        /* check if data is available for reading */
        if ( fifo->numData == fifo->numEntry )
        {
            /* alone already */
            if (fifo->refCount <= 1)
            {
                RELEASE_LOCK();
                return 0;
            }

            RELEASE_LOCK();

            SLEEP();
        }
        else
        {
            break;
        }
    }

    return (void*)( fifo->data + fifo->writeIdx * fifo->entrySize );
}

void fc_fifo_write_single_done(FC_FIFO buffer, unsigned int pid)
{
    volatile _fifo_control *fifo = (volatile _fifo_control *)buffer;

    // operation to numData must be atomic since it
    // is incremented and decremented by at least 2 separate process
    // (fcmanager and process using libfc.so) even in single mode.
    //fifo->numData++;
    __sync_fetch_and_add(&fifo->numData, 1);

    if ( fifo->writeIdx >= (fifo->numEntry-1) )
    {
        fifo->writeIdx = 0;
    }
    else
    {
        fifo->writeIdx++;
    }

    RELEASE_LOCK();
}

int fc_fifo_read_init(FC_FIFO buffer, void* init, unsigned int size, unsigned int pid)
{
    volatile _fifo_control *fifo = (volatile _fifo_control *)buffer;

    if (size != sizeof(FC_INIT))
    {
        fc_message("invalid read FC_INIT structure size\n");
        return 0;
    }

    ACQUIRE_LOCK();

    memcpy(init, (void*)&(fifo->init), sizeof(fifo->init));

    RELEASE_LOCK();

    return 1;
}

int fc_fifo_write_init(FC_FIFO buffer, void* init, unsigned int size, unsigned int pid)
{
    volatile _fifo_control *fifo = (volatile _fifo_control *)buffer;

    if (size != sizeof(FC_INIT))
    {
        fc_message("invalid write FC_INIT structure size\n");
        return 0;
    }

    ACQUIRE_LOCK();

    memcpy((void*)&(fifo->init), init, sizeof(fifo->init));

    RELEASE_LOCK();

    return 1;
}

int fc_fifo_read_ldyn(FC_FIFO buffer, void* ldyn, unsigned int size, unsigned int pid)
{
    volatile _fifo_control *fifo = (volatile _fifo_control *)buffer;

    if (size != sizeof(FC_LDYN))
    {
        fc_message("invalid FC_LDYN structure size\n");
        return 0;
    }

    ACQUIRE_LOCK();

    if ( fifo->lastLdynPtr == 0 )
    {
        fifo->lastLdynPtr = (FC_LDYN*)&(fifo->ldyn);
    }

    if ( fifo->lastLdynPtr == &(fifo->lastLdynPtr[MAX_LDYN-1]) )
    {
        fc_message("all FC_LDYN structures are read already.\n");
    }

    if ( (*(fifo->lastLdynPtr)).addr == 0 &&
          (*(fifo->lastLdynPtr)).name[0] == '\0' )
    {
        memset(ldyn, 0, sizeof(FC_LDYN));
    }
    else
    {
        memcpy(ldyn, fifo->lastLdynPtr, sizeof(FC_LDYN));
        fifo->lastLdynPtr++;
    }

    RELEASE_LOCK();

    return 1;
}

int fc_fifo_write_ldyn(FC_FIFO buffer, void* ldyn, unsigned int size, unsigned int pid)
{
    volatile _fifo_control *fifo = (volatile _fifo_control *)buffer;

    int ctr;

    if (size != sizeof(FC_LDYN))
    {
        fc_message("invalid FC_LDYN structure size\n");
        return 0;
    }

    ACQUIRE_LOCK();

    /* find available empty slot */
    for (ctr = 0; ctr < (MAX_LDYN-1); ctr++)
    {
        if ( fifo->ldyn[ctr].addr     == 0 &&
              fifo->ldyn[ctr].name[0] == '\0' )
        {
            memcpy((void*)&(fifo->ldyn[ctr]), ldyn, sizeof(fifo->ldyn[0]));
            break;
        }
    }

    if (ctr == (MAX_LDYN-1))
    {
        fc_message("maximum FC_LDYN structures (%d) reached!\n", MAX_LDYN-1);
    }

    RELEASE_LOCK();

    return 1;
}

