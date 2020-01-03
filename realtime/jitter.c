// Copyright (C) Cort Dougan, 2003
// Cort Dougan <cort@fsmlabs.com>


#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
/*
* User-level doesn't have some of these utilities.
* In fact, POSIX entirely ignores the idea of SMP or
* any utility routines for comparing/setting
* timespec values.
* -- Cort <cort@fsmlabs.com>
*/

#ifndef CONFIG_RTL

#define RTL_CPUS_MAX 4

#define rtl_num_cpus() (RTL_CPUS_MAX)
#define NSECS_PER_SEC 1000000000

#define timespec_normalize(t) { if ((t) ->tv_nsec >= NSECS_PER_SEC) { (t) ->tv_nsec -= NSECS_PER_SEC; (t) ->tv_sec++; } else if ((t) ->tv_nsec < 0) { (t) ->tv_nsec += NSECS_PER_SEC; (t) ->tv_sec -- ; }}

#define timespec_sub(t1, t2) do { (t1) ->tv_nsec -= (t2) ->tv_nsec; (t1) ->tv_sec  -= (t2) ->tv_sec; timespec_normalize(t1); } while (0)

#define timespec_add_ns(t,n) do { (t) ->tv_nsec += (n); timespec_normalize(t); } while (0)

#define timespec_lt(t1, t2) ((t1) ->tv_sec < (t2) ->tv_sec || ((t1) ->tv_sec == (t2) ->tv_sec && (t1)->tv_nsec < (t2) ->tv_nsec))
#define timespec_nz(t) ((t) ->tv_sec != 0 || (t) ->tv_nsec != 0)
#endif

pthread_t thread[RTL_CPUS_MAX], print_thread;
struct timespec worst[RTL_CPUS_MAX];
sem_t irqsem;
struct timespec abs_start;
/* in nanoseconds */
#define PERIOD (1000*1000)

void *print_code(void *t)
{ 
   int i;
   while ( 1 ) { 
      /* wait for a thread to signal us */
      sem_wait( &irqsem );
      for ( i = 0 ; i < rtl_num_cpus() ; i++ ) { 
         printf( "CPU%d: %3d.%01d us ", i,
            worst[i].tv_nsec/1000,
            (worst[i].tv_nsec/100)%10 );
      } printf("\n");
   }
   return NULL;
}

void *thread_code(void *t)
{ 
   struct timespec next, cur;
   int cpu = (int)t;
   /* zero it out */
   worst[cpu].tv_nsec = worst[cpu].tv_sec = 0;
   /* Get the current time and the start time that the main() function
    * setup for us so that all the threads are synchronized. Then, add
    * a per-cpu skew to them so the threads don't end up becoming runnable
    * at the same time and creating unnecessary resource contention.
    */
   next = abs_start;
   timespec_add_ns( &next, (PERIOD/rtl_num_cpus())*cpu );
   clock_gettime( CLOCK_REALTIME, &cur );
   while ( timespec_lt( &next, &cur ) )
      timespec_add_ns( &next, PERIOD );
   while ( 1 ) { 
      /* set the period so that we're running at PERIOD */
      timespec_add_ns( &next, PERIOD );
      /* sleep */
      clock_nanosleep( CLOCK_REALTIME, TIMER_ABSTIME, &next, NULL);
      /* compute the error between now and when
       * we expected to return from the sleep
       */
      clock_gettime( CLOCK_REALTIME, &cur );
      timespec_sub( &cur, &next );
      /* if this is the first run, set the "worst" value */
      if ( !timespec_nz(&worst[cpu]) ) { 
         worst[cpu] = cur;
         sem_post( &irqsem );
      }
      /* if this is the worst we have seen so far, print it */
      if ( timespec_lt( &worst[cpu], &cur ) ) { 
         worst[cpu] = cur;
         sem_post( &irqsem );
      } 
   }
   return NULL;
}

int main(void)
{ 
   int i;
   pthread_attr_t attr;
   struct sched_param sched_param;

   /* initialize the semaphore */
   sem_init( &irqsem, 1, 0 );

   /* get the current time that the threads can base their scheduling on */
   clock_gettime( CLOCK_REALTIME, &abs_start );

   /*
    * Start the thread that prints the timing values.
    * We set the thread priority very low to make sure that it does
    * not interfere with the threads that are doing the actual timing
    */
   pthread_attr_init( &attr );
   sched_param.sched_priority = sched_get_priority_min(SCHED_OTHER);
   pthread_attr_setschedparam( &attr, &sched_param );
   pthread_create( &print_thread, &attr, print_code, (void *)0 );
   
   /* create the threads to do the timing */
   for ( i = 0; i < rtl_num_cpus(); i++ ) { 
      /* initialize the thread attributes and set the CPU to run on */
      pthread_attr_init( &attr );

#ifdef CONFIG_RTL
      /* Linux does not allow us to set the CPU to run on */
      pthread_attr_setcpu_np( &attr, i );
#endif

      sched_param.sched_priority = sched_get_priority_max(SCHED_OTHER);
      pthread_attr_setschedparam( &attr, &sched_param );
      pthread_create( &thread[i], &attr, thread_code, (void *)i );
   }

   /* wait for the thread to exit or for the user
    * to signal us asynchronously (with ^c or some such) to exit.
    */
#ifdef CONFIG_RTL
   rtl_main_wait();

   /* cancel the threads */
   for ( i = 0 ; i < rtl_num_cpus() ; i++ )
      pthread_cancel( thread[i] );
   /* cancel the print thread */
   pthread_cancel( print_thread );
#endif

   /* join the threads */
   for ( i = 0 ; i < rtl_num_cpus() ; i++ )
      pthread_join( thread[i], NULL );
   /* join the print thread */
   pthread_join( print_thread, NULL );
   return 0;
}
