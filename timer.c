#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

volatile int time_has_come =0;
time_t cur_time;
struct itimerval interval;

void init_timer(void);
void timerHandler(int );
int startTimer(unsigned int);



/****************timer.c **********/
void init_timer( )
{
   signal(SIGALRM, timerHandler);

   return;

}

void timerHandler(int signal)
{
   time_has_come =1;
   cur_time = time(NULL);
   printf("!!!!!!! %s\n", ctime(&cur_time));
   return;
}


int startTimer( unsigned int seconds)
{
   interval.it_interval.tv_sec = 0;
   interval.it_interval.tv_usec = 0;
   interval.it_value.tv_sec = seconds;
   interval.it_value.tv_usec = 0;
   setitimer(ITIMER_REAL, &interval, NULL);
   //setitimer(ITIMER_VIRTUAL, &interval, NULL);

   cur_time = time(NULL);
   printf("start time =%s\n", ctime(&cur_time));


   return 0;
}

