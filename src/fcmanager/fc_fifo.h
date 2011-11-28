/*


*/

#ifndef __fc_fifo_h_
#define __fc_fifo_h_


#include <stdio.h>
#include <stdlib.h>


/* the FIFO type */
#define FC_FIFO void volatile *
#define FC_FIFO_NDEF NULL


/* create a new FIFO. returns the FIFO or 'FC_FIFO_NDEF' on error
   'esize' is the size of each element in buffer, and 'id' is set to
   the ID of the shared buffer, for connections. 'single' is TRUE
   if the FIFO will be used with only one writer, and FALSE if
   many writers can access the FIFO (with fc_fifo_write) */
FC_FIFO fc_fifo_create(unsigned int elements, unsigned int esize, unsigned int *id, unsigned int single);

/* connect to an existing FIFO. returns the FIFO or FC_FIFO_NDEF.
   the 'id' is the ID given by 'fc_fifo_create' */
FC_FIFO fc_fifo_connect(unsigned int id);

/* deconnect the given FIFO. once done, no more access to the FIFO
   is possible.
   delete flag if shm has no more reference. */
int fc_fifo_close(FC_FIFO fifo, unsigned int shmid, unsigned int delete);

/* return pointer to data available in FIFO */
void* fc_fifo_read_single(FC_FIFO fifo, unsigned int size, unsigned int ID);

/* marked data returned by fc_fifo_read_single() as done */
void fc_fifo_read_single_done(FC_FIFO fifo, unsigned int ID);

/* return pointer to data space available in FIFO */
void* fc_fifo_write_single(FC_FIFO fifo, unsigned int size, unsigned int ID);

/* marked data returned by fc_fifo_write_single() as done */
void fc_fifo_write_single_done(FC_FIFO fifo, unsigned int ID);

/* read FC_INIT embedded in FC_FIFO structure */
int fc_fifo_read_init(FC_FIFO fifo, void* init, unsigned int size, unsigned int ID);

/* write FC_INIT embedded in FC_FIFO structure */
int fc_fifo_write_init(FC_FIFO fifo, void* init, unsigned int size, unsigned int ID);

/* read FC_LDYN embedded in FC_FIFO structure */
int fc_fifo_read_ldyn(FC_FIFO fifo, void* ldyn, unsigned int size, unsigned int ID);

/* write FC_LDYN embedded in FC_FIFO structure */
int fc_fifo_write_ldyn(FC_FIFO fifo, void* ldyn, unsigned int size, unsigned int ID);

#endif /* __fc_fifo_h_ */
