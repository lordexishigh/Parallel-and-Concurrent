#define mailbox_c

#include "mailbox.h"

#define NO_MAILBOXES 30

static void *shared_memory = NULL;
static mailbox *freelist = NULL;  /* list of free mailboxes.  */


/*
 *  initialise the data structures of mailbox.  Assign prev to the
 *  mailbox prev field.
 */


static mailbox *mailbox_config (mailbox *mbox, mailbox *prev)
{
  mbox->in=0;
  mbox->out=0;
  mbox->prev = prev;
  mbox->item_available = multiprocessor_initSem (0);
  mbox->space_available = multiprocessor_initSem (MAX_MAILBOX_DATA);
  mbox->mutex = multiprocessor_initSem (1);
  return mbox;
}


/*
 *  init_memory - initialise the shared memory region once.
 *                It also initialises all mailboxes.
 */

static void init_memory (void)
{
  if (shared_memory == NULL)
    {
      mailbox *mbox;
      mailbox *prev = NULL;
      int i;
      _M2_multiprocessor_init ();
      shared_memory = multiprocessor_initSharedMemory
	(NO_MAILBOXES * sizeof (mailbox));
      mbox = shared_memory;
      for (i = 0; i < NO_MAILBOXES; i++)
	prev = mailbox_config (&mbox[i], prev);
      freelist = prev;
    }
}


/*
 *  init - create a single mailbox which can contain a single triple.
 */

mailbox *mailbox_init (void)
{
  mailbox *mbox;

  init_memory ();
  if (freelist == NULL)
    {
      printf ("exhausted mailboxes\n");
      exit (1);
    }
  mbox = freelist;
  freelist = freelist->prev;
  return mbox;
}


/*
 *  kill - return the mailbox to the freelist.  No process must use this
 *         mailbox.
 */

mailbox *mailbox_kill (mailbox *mbox)
{
  mbox->prev = freelist;
  freelist = mbox;
  return NULL;
}


/*
 *  send - send (result, move_no, positions_explored) to the mailbox mbox.
 */

void mailbox_send (mailbox *mbox, int result, int move_no, int positions_explored) 
//a function which populates the data of a mailbox received, with the result of a move, the move number and positions explored which are received as parameters
{
	multiprocessor_wait(mbox->space_available); //checks if the space_available semaphore is available and if it isn't, process waits until the semaphore becomes available. This is to make sure there is enough space and the semaphore is not equal to 0
	multiprocessor_wait(mbox->mutex); //checks if the mutex (or mutual exlusion object) semaphore is available and if it isn't , process waits until the semaphore becomes available. This avoids different processes manipulating the same data
	
	//adds all items received in the function as parameters to the data of the mailbox received
	mbox->data[mbox->in].result = result;
	mbox->data[mbox->in].move_no = move_no; 
	mbox->data[mbox->in].positions_explored = positions_explored;
	mbox->in=(mbox->in+1)%MAX_MAILBOX_DATA;
	multiprocessor_signal(mbox->mutex); //signals the mutex so other processes can enter the shared memory zone,
	multiprocessor_signal(mbox->item_available); //signals the item available semaphore, allowing processes to read from the mailbox as its not empty 
}


/*
 *  rec - receive (result, move_no, positions_explored) from the
 *        mailbox mbox.
 */

void mailbox_rec (mailbox *mbox, int *result, int *move_no, int *positions_explored)
//a function which received a mailbox address, along with 3 integer addresses and will copy the values of the mailbox's data into the interger parameters
{
	multiprocessor_wait(mbox->item_available); //calls the wait for the item available semaphore, checking whether there is an item available to read from
	multiprocessor_wait(mbox->mutex); //calls the wait for the mutex semaphore, checking if there is another process in the shared memory zone
	
	// copy the values of the mailbox's data into the integer parameters
	*result = mbox->data[mbox->out].result; 
	*move_no = mbox->data[mbox->out].move_no;
	*positions_explored = mbox->data[mbox->out].positions_explored;
	mbox->out=(mbox->out+1)%MAX_MAILBOX_DATA;
	multiprocessor_signal(mbox->mutex); //signals the mutex so other processes can enter the shared memory zone,
	multiprocessor_signal(mbox->space_available); //signals the space available semaphore, to make space available for other processes to use

}
