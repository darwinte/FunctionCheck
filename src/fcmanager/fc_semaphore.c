#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "fc_semaphore.h"
#include "fc_tools.h"
#include "fc_global.h"


/*
 * Bakery Algorithm
 * Lesly Lamport
 */

/* initialize a semaphore */
inline void fc_semaphore_init(FC_Semaphore *s)
{
    memset(s, 0, sizeof (*s));
}

/* search the given PID in the PID list, and returns
     the corresponding offset in the 'sem' array */
static int fc_semaphore_find_process(FC_Semaphore *s, int pid)
{
    int i;

    /* change this for a faster access method */
    for (i = 0; (i < s->number_pid) && (i < FC_MAX_SCLIENTS); i++)
    {
        if (s->pids[i] == pid)
            return (i);
    }
    /* full */
    if (i == FC_MAX_SCLIENTS)
    {
        fc_message("table of processes in the semaphore is full.");
        fc_message_fatal(FC_ERR_OTHER, "cannot resume. Sorry.");
    }
    /* not found */
    /* WARNING: this part sould be protected against multiple access */
    s->pids[i] = pid;
    s->number_pid++;
    return (i);
}

/* lock the semaphore */
void fc_semaphore_get(FC_Semaphore volatile*s, unsigned int pid)
{
    int i, process;
    struct timeval tv;

    process = fc_semaphore_find_process((FC_Semaphore*) s, pid);

    if (s->sem[process].number)
    {
        /*
         * Lock a yet locked
         */
        s->number_of_recursive_lock++;
        return;
    }

    s->sem[process].choosing = 1;
    for (i = 0; i < FC_MAX_SCLIENTS; i++)
        if (s->sem[i].number > s->sem[process].number)
            s->sem[process].number = s->sem[i].number;
    s->sem[process].number++;
    s->sem[process].choosing = 0;

    for (i = 0; i < FC_MAX_SCLIENTS; i++)
    {
        while (s->sem[i].choosing)
        {
            tv.tv_sec = 0;
            tv.tv_usec = 1000;
            select(0, NULL, NULL, NULL, &tv);
        }

        while (s->sem[i].number &&
                (
                s->sem[i].number < s->sem[process].number ||
                (
                s->sem[i].number == s->sem[process].number && i < process
                )
                )
                )
        {
            tv.tv_sec = 0;
            tv.tv_usec = 1000;
            select(0, NULL, NULL, NULL, &tv);
        }
    }
}

/* unlock the semaphore */
void fc_semaphore_put(FC_Semaphore volatile*s, unsigned int pid)
{
    int process;

    process = fc_semaphore_find_process((FC_Semaphore*) s, pid);

    if (s->number_of_recursive_lock)
        s->number_of_recursive_lock--;
    else
        s->sem[process].number = 0;
}

/* test if the semaphore is locked */
int fc_semaphore_locked(FC_Semaphore *s)
{
    int i;

    for (i = 0; i < FC_MAX_SCLIENTS; i++)
        if (s->sem[i].number)
            return (1);

    return (0);
}
