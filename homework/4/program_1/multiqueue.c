#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#include "multiqueue.h"

char * usage = "Usage:\nexecutable <thread_num> <quere_num> <write_times>\n";
char * malloc_error = "malloc error!\n";

int main(int argc, char *argv[]){
  int thread_num = 0;
  int queue_num = 0;
  int write_times = 0;

  int cycleI;

  queue * queues = NULL;

  // Check for correct number of arguments
  if (argc < 4)
  {
    puts(usage);
    return 0;
  }

  // Get parameters
  thread_num = atoi(argv[1]);
  queue_num = atoi(argv[2]);
  write_times = atoi(argv[3]);

  // Set the minimum to 1
  thread_num = (thread_num < 1) ? 1 : thread_num;
  queue_num = (queue_num < 1) ? 1 : queue_num;
  write_times = (write_times < 1) ? 1 : write_times;

  // Set a random seed
  srand((int)GetTime());

  // Print the program parameters.
  printf("Running with P=%d M=%d N=%d\n", thread_num, queue_num, write_times);

  if(NULL == (queues = (queue *)malloc(queue_num * sizeof(queue)))){
    puts(malloc_error);
    exit(-1);
  }

  for(cycleI = 0; cycleI < queue_num; ++ cycleI){
    queue_init(&queues[cycleI], cycleI);
  }

  return 0;
}

void create_threads(pthread_t **ppThreads, int thread_num, thread_func func, 
    thread_param ** pparam, thread_param common_param){
  int cycleI;
  thread_param * parameters;
  
  pthread_t *threads;
  threads = *ppThreads = (pthread_t *)malloc(sizeof( pthread_t ) * thread_num);
  if(NULL == threads){
    puts(malloc_error);
    exit(-1);
  }
  parameters = *pparam = (thread_param *)malloc(sizeof(thread_param) * thread_num);

  // Initialize the parameters
  for (cycleI = 0; cycleI < thread_num; ++cycleI){
    parameters[cycleI] = common_param;
    parameters[cycleI].thread_id = cycleI;
  }

  // ------------------- Create Threads ---------------------
  // Create threads
  for (cycleI = 0; cycleI < thread_num - 1; ++cycleI){
    pthread_create(&threads[cycleI],
    NULL,	// Default attibutes
    func,	///void *(*start_routine)(void *),
    &parameters[cycleI]);
  }

  // Timing: To help other threads complete creation, the
  // master thread will sleep.
  usleep(150000);
  func( &parameters[thread_num - 1] );

  return;
}

void join_threads(pthread_t * threads, int thread_num, thread_param * param){
  int cycleI;
  void *retvalue;

  for (cycleI = 0; cycleI < thread_num - 1; ++cycleI){
    pthread_join(threads[cycleI], &retvalue);
    if (NULL == retvalue){
    }
  }

  free(threads);
  free(param);
  return;
}

// Thread function to manipulate the queue
void * queue_thread(void * p){
  thread_param * param = (thread_param *)p;
  if(NULL == param){
    return NULL;
  }

  queue * queues = param->queues;
  if(NULL == queues){
    return NULL;
  }
  
  int thread_id = param->thread_id;
  int write_times = param->write_times;
  int queue_num = param->queue_num;

  int cycleI = 0;
  for(cycleI = 0; cycleI < write_times; ++cycleI){
    int queue_id = random() % queue_num;
    queue_push(&queues[queue_id], thread_id);
  }

  return((void *)1);
}

double GetTime(void){
  // ------------------- Local Variables ------------------
  struct timeval tp;
  double localtime;

  // ------------------- Get Time -------------------------
  gettimeofday( &tp, NULL );
  localtime = ( double ) tp.tv_usec;
  localtime /= 1e6;
  localtime += ( double ) tp.tv_sec;

  // ------------------- Return Time ----------------------
  return( localtime );
}

// initialize the queue
void queue_init(queue * q, int id){
  q->head = NULL;
  q->tail = NULL;
  q->item_num = 0;
  q->queue_id = id;
}

// push an item to the end of the queue
void queue_push(queue * q, int thread_id){
  queue_item * item = NULL;
  if(NULL == (item = (queue_item *)malloc(sizeof(queue_item)))){
    puts(malloc_error);
    return;
  }

  // initialize the item
  item->thread_id = thread_id;
  item->next = NULL;

  // push the item to the end of the queue
  if(NULL == q->head){
    q->head = item;
    q->tail = item;
    q->item_num = 1;
  }else if(NULL != q->tail){
    q->tail->next = item;
    q->tail = item;
    ++(q->item_num);
  }else{
    q->tail = item;
    ++(q->item_num);
  }
}

// output the queue
void queue_output(queue * q, thread_param param){
  if(NULL == q->head){
    printf("Empty Queue");
    return;
  }

  int cum_sum_first = 0;
  int cum_sum_second = 0;

  queue_item * item = q->head;
  queue_item * next = item;
  while(item){
    next = item->next;
    cum_sum_first += item->thread_write_num;
    cum_sum_second += item->thread_id;
    item = next;
  }

  printf("Queue %d of %d:\n", q->queue_id, param.thread_num);
  printf("N=%d\n", param.write_times);
  printf("%d items in queue\n", q->item_num);
  printf("Cumulative sum of first elements = %d\n", cum_sum_first);
  printf("Cumulative sum of second elements = %d\n", cum_sum_second);
  printf("Last elementof last item = %d\n", q->tail->cum_sum);
  printf("Summary: [Queue %d/%d, N=%d, %d times, Sums = (%d,%d,%d)]\n",
      q->queue_id, param.thread_num, param.write_times, q->item_num,
      cum_sum_first, cum_sum_second, q->tail->cum_sum);
}

// free the queue
void queue_free(queue *q){
  if(NULL == q->head){
    return;
  }
  queue_item * item = q->head;
  queue_item * next = item;
  while(item){
    next = item->next;
    free(item);
    item = next;
  }
  q->head = NULL;
  q->tail = NULL;
  q->item_num = 0;
}

