/*  User-level thread system
 *
 */

#include <setjmp.h>
#include <string.h>

#include "auy.h"
//#include "umix.h"
#include "mythreads.h"

#define QUEUESIZE 1000
#define DEBUG 1
static int MyInitThreadsCalled = 0; /* 1 if MyInitThreads called, else 0 */
//static int head = 1;           //?
//static int search_from = 1;    //?
static int current_tid = 0;   //current thread id
static int spawning_tid = 0;  //keep tracking spawn last assignment thread id
typedef struct{
  int q[QUEUESIZE]; //int q[QUEUESIZE-1]; <-提供的源文件这里有错，被坑 =.=|||
  int first;
  int last;
  int pointer;
  int count;
}queue;

static queue tid_queue;

// initialize queue
void init_queue(queue *q)
{
  q->first = 0;
  q->last = QUEUESIZE - 1;
  q->count = 0;
  q->pointer = 0;
}

//insert element to queue
void enqueue(queue *q, int x)
{
  if (q->count >= QUEUESIZE)
  Printf("Warning: queue overflow enqueue x=%d\n",x);
  else {
    q->last = (q->last+1) % QUEUESIZE;
    q->q[ q->last ] = x;
    q->count = q->count + 1;
  }
  //int i;
  //for( i=0;i<QUEUESIZE;i++) Printf("%d ",q->q[i]);
  //Printf("\nfirst:%d,last:%d\n",q->first,q->last);
}

//remove element from queue, always the first element
int dequeue(queue *q)
{
  int x;

  if (q->count <= 0) Printf("Warning: empty queue dequeue.\n");
  else {
    x = q->q[ q->first ];
    q->first = (q->first+1) % QUEUESIZE;
    q->count = q->count - 1;
  }

  return(x);
}

void print_queue(queue *q){
int counter = 0;
 if(DEBUG == 1) Printf("\n");
 
 while( counter < q->count){
  if(DEBUG == 1) Printf("%d ", q->q[q->first + counter]);
  counter++;
 }
 if(DEBUG == 1) Printf("\n");
}

void move_to_queue_head(queue *q, int t){
  int count = 0; 
  int inner_count;
  while(count < q->count){
    if(q->q[q->first + count] == t){
      //Printf("hit on %d", (q->first + count));
      if((q->first + count) == q->first){
        break;
      }
      inner_count = count;
      while((q->first + inner_count) != q->last){
//        if(DEBUG == 1) Printf("replace %d with %d\n");
        q->q[q->first + inner_count] = q->q[q->first + inner_count + 1];
        //print_queue(&tid_queue);
        inner_count += 1;
      }
      if(q->last > 1 && q->first > 0){
        q->last -= 1;
        q->q[q->first - 1] = t;
        q->first -= 1;
      }
    }
    count++;
  }

  
}


// helper method to get last element in queue
int get_queue_last(queue *q)
{
  if(q->count <= 0) Printf("Warning: empty queue dequeue.\n");
  return q->q[ q->last ];
}
// helper method to get the first element in queue
int get_queue_first(queue *q)
{
  if(q->count <= 0) Printf("Warning: empty queue dequeue.\n");
  return q->q[ q->first ];
}
// helpter method to get next element in queue
int get_queue_next(queue *q)
{
int current;
  if(q->count <= 0) Printf("Warning: empty queue dequeue.\n");
   current = q->pointer;
  q->pointer = (q->pointer + 1) % q->count;
  return q->q[current];
}
// helpter method to check whether queue is empty or not
int empty(queue *q)
{
  if (q->count <= 0) return 1;
  else return 0;
}

static struct thread {      /* thread table */
  int valid;      /* 1 if entry is valid, else 0 */
  jmp_buf env;      /* current context */
  jmp_buf clean_env;
  int clean;
  void (*func)();  //void * func; <-提供的源文件这里有错误 =。=||
  int param;
} thread[MAXTHREADS];
#define STACKSIZE 65536   /* maximum size of thread stack */

/*  MyInitThreads () initializes the thread package.  Must be the first
 *  function called by any user program that uses the thread package.
 */
void setStackSpace(int pos)   /*recursively allocate stack space, save each thread start point*/
{  
char s[STACKSIZE];
void (*f)();
int p;
  if(pos < 1){
    thread[0].clean = 0;
    longjmp(thread[0].clean_env, 2);
  }else{
    if(setjmp(thread[pos].clean_env)!=0){
       (*(thread[current_tid].func))(thread[current_tid].param); //注意这里不能用pos代替current_tid,因为pos跳回来后不稳定
       MyExitThread();
    }
    setStackSpace(pos-1); 
  }
}

void MyInitThreads ()
{
  int i;
  int setjum_ret;
  char s[STACKSIZE];
  void (*f)();
  int p;

  if (MyInitThreadsCalled) {                /* run only once */
    Printf ("InitThreads: should be called only once\n");
    Exit (0);
  }

  for (i = 0; i < MAXTHREADS; i++) {      /* init all threads */
    thread[i].valid = 0;
    thread[i].clean = 1;
  }

  init_queue(&tid_queue);                 /* init queue */
  thread[0].valid = 1;               /* the initial thread is 0 */
  MyInitThreadsCalled = 1;
  enqueue(&tid_queue, 0);
  current_tid = 0;
  spawning_tid = 0;


  if((setjum_ret=setjmp(thread[0].clean_env))==0){
    //if(DEBUG) Printf("Thread0:0\n");
    memcpy(thread[0].env,thread[0].clean_env, sizeof(jmp_buf));
    setStackSpace(MAXTHREADS);
  }else if(setjum_ret==2){
    //if(DEBUG)  Printf("Thread0:2\n"); 
  }else if(setjum_ret==1){
    //if(DEBUG)  Printf("Thread0:1\n");//can't get in
    (*(thread[0].func))(thread[0].param);
    MyExitThread();
  }
}

/*  MySpawnThread (func, param) spawns a new thread to execute
 *  func (param), where func is a function with no return value and
 *  param is an integer parameter.  The new thread does not begin
 *  executing until another thread yields to it.
 */

int MySpawnThread (func, param)
  void (*func)();   /* function to be executed */
  int param;    /* integer parameter */
{

  int i;

  if (! MyInitThreadsCalled) {
    Printf ("MySpawnThread: Must call MyInitThreads first\n");
    Exit (0);
  }
  //if(DEBUG>1)   Printf("log:Enter MySpawn\n");

  int j;
  for(i=(spawning_tid+1)%MAXTHREADS,j=0;j<MAXTHREADS && thread[i].valid==1;j++,i=(i+1)%MAXTHREADS); /*find next available thread number*/
  //Printf("i:%d,j:%d\n",i,j);
  if(j>MAXTHREADS){  /*no space for new thread*/
     Printf("MySpawnThread: Reach the MAXTHREADS limit. Unable to spawn new thread.\n");
     return -1;
  }

  //i=1-current_tid;//pa4b
  thread[i].valid = 1;
  thread[i].clean = 1;
  thread[i].func = func; //Printf("Spawn func:%d\n",func);
  thread[i].param = param;
  spawning_tid = i;
  enqueue(&tid_queue, i); 
  memcpy(thread[i].env, thread[i].clean_env, sizeof(jmp_buf));
  //Printf("Spawn: currunt_tid %d， spawn_id %d\n",current_tid,spawn);
  return i;

}


/*  MyYieldThread (t) causes the running thread to yield to thread t.
 *  Returns id of thread that yielded to t (i.e., the thread that called
 *  MyYieldThread), or -1 if t is an invalid id.
 */

int MyYieldThread (t)
  int t;        /* thread being yielded to */
{
  int parent_thread = current_tid;
  if (! MyInitThreadsCalled) {
    Printf ("MyYieldThread: Must call MyInitThreads first\n");
    Exit (0);
  }
  if( t < 0 || t > MAXTHREADS){
    Printf ("YieldThread: %d is not a valid thread ID\n", t);
    return (-1);
  }
  if (! thread[t].valid) {
    Printf ("YieldThread: Thread %d does not exist\n", t);
    return (-1);
  }
  if (parent_thread==t)
     return t;   
  //print_queue(&tid_queue);
  if(setjmp(thread[parent_thread].env)==0){
    //parent move to queue tail.Note: if parent is not valid, then yeild function is call by MyExit->MySpawn-> this MyYield.
     if(thread[parent_thread].valid==1){
       move_to_queue_head(&tid_queue,parent_thread);
       dequeue(&tid_queue);
       enqueue(&tid_queue,parent_thread);
     }
    current_tid=t;
    //Printf("Yeild: parentid: %d,currunt_tid:%d\n",parent_thread,t);
    longjmp(thread[t].env,1);
  }
  return parent_thread;
}

/*  MyGetThread () returns id of currently running thread.
 */

int MyGetThread ()
{
  if (! MyInitThreadsCalled) {
    Printf ("MyGetThread: Must call MyInitThreads first\n");
    Exit (0);
  }
  return current_tid;
}

/*  MySchedThread () causes the running thread to simply give up the
 *  CPU and allow another thread to be scheduled.  Selecting which
 *  thread to run is determined here.  Note that the same thread may
 *  be chosen (as will be the case if there are no other threads).
 */

void MySchedThread ()
{ int tid;
  if (! MyInitThreadsCalled) {
    Printf ("MySchedThread: Must call MyInitThreads first\n");
    Exit (0);
  }

  if(empty(&tid_queue)) Exit(0);
  else {
    tid = get_queue_first(&tid_queue);
    //Printf("Sched :%d\n",tid);
    //print_queue(&tid_queue);
    MyYieldThread (tid);
  }

}

/*  MyExitThread () causes the currently running thread to exit.
 */

void MyExitThread ()
{
  if (! MyInitThreadsCalled) {
    Printf ("MyExitThread: Must call MyInitThreads first\n");
    Exit (0);
  }
  if(DEBUG>1)Printf("log:Enter MyExit\n");
  int t = current_tid;
  thread[t].valid = 0;
  thread[t].clean = 0;
  move_to_queue_head(&tid_queue,t); 
  dequeue(&tid_queue);
  //Printf("thread xit:%d\n",t);
  //print_queue(&tid_queue);
  MySchedThread();
}



