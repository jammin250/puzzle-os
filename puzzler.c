/* The jigsaw puzzle solver */
/*     computer science     */
/*            os            */
/*      multi-threaded      */

#include "puzzler.h"

/* Uncomment for debug mode 
#define DEBUG
*/

void * puzzle_solver( void * );
void find_piece(int row, int column);

int main(int argc, char *argv[])
{
  unsigned short int thread_count;

  if(argc==1){
    thread_count=0;
  }
  else{
    thread_count=(unsigned short int)( atoi(argv[1]) );
  }

#ifdef DEBUG
  printf("Number of threads to solve the problem: %d\n" , thread_count);
#endif

  /* INITIALIZE THE FIRST NODE IN THE LIST */
  head.next = NULL;
  head.prev = NULL;
  head.n=-1;head.e=-1;head.w=-1;head.s=-1;
  head.name="HEAD";
  sem_init(&head.node_lock,0,1);

  /* GLOBAL GRID */
  grid.cells = NULL; //the pointer is null, not too sure on this one...

  /* GET THE PROBLEM FROM STDIN */
#ifdef DEBUG
  printf("Reading Input........");
#endif
  if( ( get_input() ) == 1 ){
    perror("input error. check the file for proper format");
    exit(1);
  }
#ifdef DEBUG
    printf("done\n");
#endif

#ifdef DEBUG
  print_list();
  print_grid();
  printf("N: %d\nE: %d\n S: %d\n W: %d\n", grid.cells[0][0].n, grid.cells[0][0].e, grid.cells[0][0].s, grid.cells[0][0].w);
#endif
  

  // Solve the problem without using threads
  if (thread_count == 0)
  {
    puzzle_solver( (void *)NE );
  }
  else
  {
    int l;
    pthread_attr_t atr;
    pthread_attr_init(&atr);
    pthread_attr_setscope( &atr , PTHREAD_SCOPE_SYSTEM );
    pthread_t tpool[thread_count];

    // create the thread pool
    for ( l = 0; l < thread_count; l++ ){
      direction p = (direction)(l%4);
      pthread_create(   &(tpool[l]), 
                        &atr,
                        puzzle_solver,
                        (void *)p);
    }

    // wait for the threads to finish
    for ( l = 0; l < thread_count; l++ ){
      pthread_join( tpool[l], NULL );
    }

  }

  print_grid();

  release_memory();

  return 0;
}

void * puzzle_solver(void *p)
{
  direction dir = (direction)p;
  int s_row, s_column;

  switch(dir)
  {
    case SE:
    s_row=0;s_column=0;
    break;
    case SW:
    s_row=0;s_column=grid.numcols-1;
    break;
    case NW:
    s_row=grid.numrows-1;s_column=grid.numcols-1;
    break;
    case NE:
    s_row=grid.numrows-1;s_column=0;
    break;
  }

  while
    (
     ( (dir == SE) && (s_row < grid.numrows) ) ||
     ( (dir == SW) && (s_row < grid.numrows) ) ||
     ( (dir == NW) && (s_row >= 0) ) ||
     ( (dir == NE) && (s_row >= 0) ) 
     )
  {

    //if you can get the lock and the piece is not found
    if ( 
     ( sem_trywait( &(grid.cells[s_column][s_row].cell_lock) ) == 0 ) &&
     ( grid.cells[s_column][s_row].n == -1 || grid.cells[s_column][s_row].e == -1 || 
       grid.cells[s_column][s_row].w == -1 || grid.cells[s_column][s_row].s == -1 )
     )
    {

#ifdef DEBUG
      printf("Grid Semaphore Obtained for %dx%d:\t thread-id:%d\n", s_column, s_row, ((int)pthread_self()) % 1000 );
#endif

      //you need two sides to solve the puzzle 
      //spin while you wait for the other thread to fill
      while (
        ( (dir == SE || dir == SW) && s_row!=0 && grid.cells[s_column][s_row-1].s == -1) || 
        ( (dir == NE || dir == NW) && s_row!=grid.numrows-1 && grid.cells[s_column][s_row + 1].n == -1)
        ){}

      find_piece( s_row , s_column );
      sem_post(&(grid.cells[s_column][s_row].cell_lock));
      
      switch(dir)
      {
        case SE:
        if(s_column == grid.numcols-1){
          s_row++;s_column=0;
        }else{
          s_column++;
        }
        break;

        case SW:
        if(s_column == 0){
          s_row++;s_column=grid.numcols-1;
        }else{
          s_column--;
        }
        break;

        case NW:
        if(s_column == 0){
          s_row--;s_column=grid.numcols-1;
        }else{
          s_column--;
        }
        break;

        case NE:
        if(s_column == grid.numcols-1){
          s_row--;s_column=0;
        }else{
          s_column++;
        }
        break;
      }
    }
    else
    {
#ifdef DEBUG
      printf("Grid Semaphore NOT obtained for %dx%d:\t thread-id:%d\n", s_column, s_row, ((int)pthread_self()) % 1000 );
#endif
      switch(dir)
      {
        case SE:
        s_row++;s_column=0;
        break;
        case SW:
        s_row++;s_column=grid.numcols-1;
        break;
        case NW:
        s_row--;s_column=grid.numcols-1;
        break;
        case NE:
        s_row--;s_column=0;
        break;
      }
    }
  }  

#ifdef DEBUG
  printf("Thread %d about to exit\n", ((int)pthread_self()) % 1000);
#endif

  return NULL;
}


void find_piece(int row, int column)
{
#ifdef DEBUG
  printf("finding piece %dx%d\n", column, row);
#endif

  int found=0;

  //Get the lock to the head
  if ( sem_wait( &(head.node_lock) ) != 0 ){
#ifdef DEBUG
    printf("Thread ID %d was unable to aquire semaphore for the HEAD of the list.\n", ((int)pthread_self()) % 1000 );
#endif
  }
  else{
#ifdef DEBUG
    printf("Head to the list acquired. thread-id:%d\n", ((int)pthread_self()) % 1000 );
#endif
  }

  node_t *runner=head.next;

  //find the piece 
  while( !found ) 
  {

    // if you hit the end of the list. start again, the piece exists. 
    if (runner == NULL){
#ifdef DEBUG
      printf("unable to find piece. %2dx%2d thread %d STARTING AGAIN\n" , column, row, ((int)pthread_self()) % 1000);
#endif
    }


    // Lock the NODE
    if ( (sem_wait( &(runner->node_lock) ) ) == 0 && (row == 99) ){
#ifdef DEBUG
      printf( "Thread ID %d has aquired the semaphore for piece: %s\n" , ( (int)pthread_self()) % 1000 , runner->name );
#endif
    }  

    // logic to check to see if the current node contains the correct piece
    // as you can see there are many cases and exceptions
    if( 
      (grid.cells[column][row].n == runner->n && grid.cells[column][row].e == runner->e) ||
      (grid.cells[column][row].n == runner->n && grid.cells[column][row].w == runner->w) ||
      (grid.cells[column][row].s == runner->s && grid.cells[column][row].e == runner->e) ||
      (grid.cells[column][row].s == runner->s && grid.cells[column][row].w == runner->w) ||
      (column > 0 && row > 0 && grid.cells[column][row-1].s == runner->n && grid.cells[column-1][row].e == runner->w) ||
      (column < grid.numcols-1 && row > 0 && grid.cells[column][row-1].s == runner->n && grid.cells[column+1][row].w == runner->e) ||
      (column < grid.numcols-1 && row < grid.numrows-1  && grid.cells[column][row+1].n == runner->s && grid.cells[column+1][row].w == runner->e) ||
      (column > 0 && row < grid.numrows-1 && grid.cells[column][row+1].n == runner->s && grid.cells[column-1][row].e == runner->w) ||
      (column !=0 && row==0 && grid.cells[column][row].n == runner->n && grid.cells[column-1][row].e == runner->w ) || //se
      (column == 0 && row!=0 && grid.cells[column][row-1].s == runner->n && grid.cells[column][row].w == runner->w) || //se
      (column != grid.numcols-1 && row==0 && grid.cells[column][row].n == runner->n && grid.cells[column+1][row].w == runner->e) || //sw
      (column == grid.numcols-1 && row!=0 && grid.cells[column][row-1].s == runner->n && grid.cells[column][row].e == runner->e) || //sw
      (column != grid.numcols-1 && row == grid.numrows-1 && grid.cells[column][row].s == runner->s && grid.cells[column+1][row].w == runner->e) || //nw
      (column == grid.numcols-1 && row != grid.numrows -1 && grid.cells[column][row+1].n == runner->s && grid.cells[column][row].e == runner->e) || //nw
      (column !=0 && row==grid.numrows-1 && grid.cells[column][row].s == runner->s && grid.cells[column-1][row].e == runner->w ) || //ne
      (column == 0 && row != grid.numrows-1 && grid.cells[column][row+1].n == runner->s && grid.cells[column][row].w == runner->w ) //ne
      )
{
  found=1;
}
  // Post on the semaphore
else{
  if ( sem_post( &(runner->prev->node_lock) ) == 0 && row==99){
#ifdef DEBUG
    printf("Thread %d has posted for the semaphore for %s\n" , ((int)pthread_self()) % 1000 , runner->prev->name );
#endif
  }
  // move the runner to the next item in the list
  runner=runner->next;
}
}

  // Fill in the piece to the grid
grid.cells[column][row].n = runner->n;
grid.cells[column][row].w = runner->w;
grid.cells[column][row].e = runner->e;
grid.cells[column][row].s = runner->s;
strcpy( grid.cells[column][row].name, runner->name );

#ifdef DEBUG
  printf("%s\tthread-id:%d\n", runner->name, ((int)pthread_self()) % 1000 );
#endif

//Unlink and free the item from the list
runner->prev->next = runner->next;
if ( runner->next != NULL ){
  runner->next->prev = runner->prev;
}
else{
  runner->prev->next == NULL;
}

// Post for the semaphore that was the previous node in the list. 
if ( sem_post( &(runner->prev->node_lock) ) == 0 && row==99){
#ifdef DEBUG
    printf("Thread %d has posted for the semaphore for %s\n" , ((int)pthread_self()) % 1000 , runner->prev->name );
#endif
}

// Free the piece's memory back to the Operating System
free(runner);


}



