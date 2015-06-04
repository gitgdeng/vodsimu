/*
 * gdeng, 2006.12 for vod server simuator testing
 *
 * the code was build on Linux pc to simulator the vod server, which will
 * talk to the vod cilent runnin gon set-top-box
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <sys/select.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>
#include <server.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>

//#define _BSD_SOURCE
#define inaddrr(x) (*(struct in_addr *) &ifr->x[sizeof sa.sin_port])
#define IFRSIZE   ((int)(size * sizeof (struct ifreq)))

typedef void (*sighandler_t)(int);

extern uint16_t MS_SERVER_PORT;
extern uint16_t VOD_SERVER_PORT;
extern int msclientfd[FD_SETSIZE];
extern int vodclientfd[FD_SETSIZE];

extern int startTimer (unsigned int);
extern void init_timer(void );

extern int mainMS(void);
extern int mainVOD(void);

sighandler_t sigint(void);
void display_menu(void);
int  display_sub_menu(void);
void set_vod_server_ip_address(void);
void set_simulator_port_number(void);
void display_ip_address(void);
void  display_vod_streaming_type(void);


/* global variables */
rtsp_server_modes_t  menu_choice;
rtsp_testcase_t  sub_choice;
char ms_server_ip_addr[5][20];
char ms_server_if_name[5][20];
char vod_server_ip_addr[20];

int  vod_stream_transport_type;

/******************************** main() **************/
int main (int argc, char **argv)
{

   pthread_t		vod_thread_id;

   bzero(ms_server_ip_addr, 5*20);
   bzero(ms_server_if_name, 5*20);
   bzero(vod_server_ip_addr, 20);

   display_ip_address();
   set_vod_server_ip_address ( );
   set_simulator_port_number ( );

   display_vod_streaming_type();

   display_menu();
   //printf("menu_choice is %d\n", menu_choice);



//===========================================
   /* init a timer */
   init_timer();

   signal(SIGINT, (sighandler_t)sigint);
   //startTimer(5);


   /* create a thread for VOD server */
   if (menu_choice == ONE_MS_SERVER_AND_ONE_VOD_SERVER)
   {
      if (pthread_create(&vod_thread_id, NULL, (void *)mainVOD, (void *)0) != 0)
      {
	   printf ("Cannot create new thread.\n");
           exit(1);
      }
     
   }

   /* start MS server */
   mainMS();

  return 0;
}
   


sighandler_t sigint()

{  
   int i;

   for (i =0 ; i < FD_SETSIZE; i++) {
 	if (msclientfd[i] != -1)
        {
          close(msclientfd[i]);
	  msclientfd[i] = -1;
        }

 	if (vodclientfd[i] != -1)
        {
          close(vodclientfd[i]);
	  vodclientfd[i] = -1;
        }
   }
   exit(0);

   return 0;
}

void display_menu(void)
{
   int choice;

   while(1)
   {
      /* display the  menu */
      printf("\n\n=============  Main Menu =================\n");
      printf(" Please choose one of the following  server modes \n");
      printf("   1. Simulate one VOD server ( 1 server 1 heartbeat, no streaming\n");
      printf("   2. Simulate one MS server and use nCube VOD server ( 1 server 1 heartbeat -- streaming doesn't work yet\n");
      printf("   3. Simulate one MS server and one VOD server ( 2sever 2 heartbeat, no streaming\n");
      printf("\n============================================\n");

      scanf("%d", &choice);

      if (( choice >= 1) && (choice <=3 ) ) 
      {
         menu_choice = choice;
         sub_choice = display_sub_menu();
         return;
      }
      
      else if ( choice > 3)
      {
         printf(" You just entered wrong choice. Do it again\n"); 
      }

   }
}

int  display_sub_menu(void)
{
   int choice, i = 0;

   while(1)
   {
      /* display the  menu */
      printf("\n\n================ Test Cases =====================\n");
      printf("\t Please choose one of the following cases \n");
      printf("\t   %d. Default case ( sucessful case )\n", i++);
      printf("\t   %d. SETUP response error\n", i++);
      printf("\t   %d. SETUP response: has timout value returned\n\n", i++);
      printf("\t   %d. DESCRIBE resposne  error\n\n", i++);
      printf("\t   %d. PLAY resposne  error\n", i++);
      printf("\t   %d. PLAY resposne  no header field: Range: npt=0\n\n", i++);
      printf("\t   %d. GET_PARAMETER resposne  error\n\n", i++);
      printf("\t   %d. ANNOUNCE event code < 4000\n", i++);
      printf("\t   %d. ANNOUNCE event code > 4000\n", i++);
      printf("\t   %d. GET_PARAMETER sends back msg body even if it is requested\n", i++);
      printf("\n============================================\n");
      printf("\n");

      scanf("%d", &choice);

      if ( (choice >=0 ) && (choice < i))
      {
         return (choice);
      }
      else
         printf(" You just entered wrong choice. Do it again\n"); 

   } /* end of while */
} // end of display_sub_menu

void set_vod_server_ip_address ( )
{
   int choice, i;

   while (1)
   {
      printf("\n\n============== IP Address ==================\n");
      printf("\n\tChoose one of ip addresses for simulator\n");
      for (i=0; strlen(ms_server_ip_addr[i]); i++)
      {
         printf("\t\t%d. %s: %s\n", i+1, ms_server_if_name[i], ms_server_ip_addr[i]);

      }
      printf("\n============================================\n");

      scanf("%d", &choice);

      //printf("choice=%d\n", choice);

      if ( (choice >= 1) && (choice <= i))
      {
         strcpy(vod_server_ip_addr, ms_server_ip_addr[choice-1]);
         printf("\n IP address is: %s\n", vod_server_ip_addr);
         return;
      }
      else
         printf(" You just entered wrong choice. Do it again\n"); 

   }
}


void set_simulator_port_number ( )
{
   int choice;

   while (1)
   {
      printf("\n\n============== Port Number ==================\n");
      printf("\n\tChoose a port number for simulator ( 1- 65534 ):");
      printf("\n\t   WARNING: If you choose a well-known port, e.g., 554 ,"); 
      printf("\n\t            you need root privilege to run the simulator\n");
      printf("\n============================================\n");


      scanf("%d", &choice);

      //printf("choice=%d\n", choice);

      if ( (choice >  0) && (choice <= 65534))
      {
         printf("\n Port numbe is  %d\n", choice);
         MS_SERVER_PORT = choice;
         VOD_SERVER_PORT = MS_SERVER_PORT + 1;
         return;
      }
      else
         printf(" You just entered wrong choice. Do it again\n"); 

   }

   return;
}

/* display info about network interfaces */
void display_ip_address()
{

  //unsigned char      *u;
  int                sockfd, size  = 1;
  struct ifreq       *ifr;
  struct ifconf      ifc;
  struct sockaddr_in sa;
  int                i = 0;

  if (0 > (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP))) {
          fprintf(stderr, "Cannot open socket.\n");
    exit(EXIT_FAILURE);
  }

  ifc.ifc_len = IFRSIZE;
  ifc.ifc_req = NULL;

  do {
    ++size;
    /* realloc buffer size until no overflow occurs  */
    if (NULL == (ifc.ifc_req = realloc(ifc.ifc_req, IFRSIZE))) {
      fprintf(stderr, "Out of memory.\n");
      exit(EXIT_FAILURE);
    }
    ifc.ifc_len = IFRSIZE;
    if (ioctl(sockfd, SIOCGIFCONF, &ifc)) {
      perror("ioctl SIOCFIFCONF");
      exit(EXIT_FAILURE);
    }
  } while  (IFRSIZE <= ifc.ifc_len);


  ifr = ifc.ifc_req;
  for (;(char *) ifr < (char *) ifc.ifc_req + ifc.ifc_len; ++ifr) {

    if (ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data) {
      continue;  /* duplicate, skip it */
    }

    if (ioctl(sockfd, SIOCGIFFLAGS, ifr)) {
      continue;  /* failed to get flags, skip it */
    }

    if ( !(strncmp("127.0.0.1", inet_ntoa(inaddrr(ifr_addr.sa_data)), 9) == 0))
    {
      strcpy(ms_server_ip_addr[i], inet_ntoa(inaddrr(ifr_addr.sa_data)));
      strcpy(ms_server_if_name[i], ifr->ifr_name);
      //printf("\t\t%d. %s: %s\n", i+1, ifr->ifr_name, ms_server_ip_addr[i]);
      i++;
    }


  }

  close(sockfd);
  return ;
}


void  display_vod_streaming_type(void)
{
   int choice;

   while(1)
   {
      /* display the  menu */
      printf("\n\n========== stream transport type ===========\n");
      printf("\t Please choose VOD stream transport type \n");
      printf("\t\t 1. vod_over_ip\n");
      printf("\t\t 2. vod_over_QAM\n");
      printf("\n============================================\n");
      printf("\n");

      scanf("%d", &choice);

      if ( (choice >=1 ) && (choice <= 2))
      {
         vod_stream_transport_type = choice;
         return ;
      }
      else
         printf(" You just entered wrong choice. Do it again\n"); 
    }

   return;
}
