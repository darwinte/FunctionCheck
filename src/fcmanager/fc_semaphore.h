
#ifndef __fc_semaphore_h_
#define __fc_semaphore_h_


/* max number of client process allowed */
#define FC_MAX_SCLIENTS 256

/* structure for the semaphores,
   must be 'unsigned int' aligned. */
typedef struct
{
  int number_of_lock;
  int number_of_recursive_lock;
  struct
  {
    short choosing;
    short number;
  }sem[FC_MAX_SCLIENTS];
  int pids[FC_MAX_SCLIENTS]; /* association PID/offset in 'sem' */
  int number_pid;  /* max value used in 'pids' */
}FC_Semaphore;


/* initialize the semaphore structure */
void fc_semaphore_init(FC_Semaphore *s);

/* get the corresp. semaphore for the given process */
void fc_semaphore_get(FC_Semaphore volatile*s, unsigned int pid);

/* put a semaphore for the given process */
void fc_semaphore_put(FC_Semaphore volatile*s, unsigned int pid);

/* test if the semaphore is locked */
int fc_semaphore_locked(FC_Semaphore *s);


#endif /* __fc_semaphore_h_ */
