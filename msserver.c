#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
//#include <signal.h>
//#include <wait.h>
//#include <sys/select.h>
//#include <strings.h>
#include <pthread.h>

#include <server.h>



/* global variables */
int msclientfd[FD_SETSIZE];
uint16_t MS_SERVER_PORT = 554;
int  use_ncube_flag = 0;
char location_hdr[256];
int is_ms_server = 1;

extern rtsp_server_modes_t menu_choice;
extern rtsp_testcase_t sub_choice;
extern char vod_server_ip_addr[256];

extern int startTimer (unsigned int);
extern void init_timer(void );

extern int process_client_request (int ,  char *, int, int);
extern int relay_rtsp_msg (int ,  char *, int);
extern int write_number_of_bytes (int , char *, int); 
//extern int read_each_line (int , char *, int * );
extern int send_annouce (int socket);

extern int read_opentv_msg(int sock_ms, char *msgBuffer);
extern int read_ncube_msg(int sock_ms, char *msgBuffer);
extern int read_line(char *msgBuffer, char *line); 
extern int customize_ncube_response(char *line, int *total_line_num);

int mainMS(void);
void childMain(int);

void read_from_opentv_vod(int sock_ms, int sock_vod, int use_ncube_flag);
void read_from_ncube(int sock_ms, int sock_vod, int use_ncube_flag); 

#define NCUBE_IP_ADDR	   "192.168.0.241"
#define NCUBE_SERVER_PORT  554      
#define MAX(a,b)           ( (a)> (b)) ? (a):(b))

#define SO_REUSEPORT 15 

extern uint16_t VOD_SERVER_PORT;

/******************************** mainMS() **************/
int mainMS (void)
{
   int sock_ms, s, i;
   socklen_t  client_len;
   struct sockaddr_in client_addr, server_addr;
   pid_t  childpid;
   int optval =1;

   bzero(location_hdr, 256);

   /*create a TCP socket for MS server*/
   if ( (s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
	printf (" Socker error: errno=%d\n", errno);
        return (1) ;   /* Error socket */
   }

   bzero (&server_addr, sizeof (server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);

   server_addr.sin_port = htons ( MS_SERVER_PORT);
   setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
   setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

   /*bind the socket*/
   if (  (bind ( s, ( struct sockaddr *) &server_addr, sizeof(server_addr)) ) < 0 ) {
	printf (" msserver Bind error: errno=%d\n", errno);
        return(2);     /* bind error */
   }

  
   /*listen to the socket*/
   if ( (listen(s, LISTEN_QUEUE_LEN)) < 0 ){
	printf (" Listen error: errno=%d\n", errno);
       return (3);   /*listen error */
   }


   for (i =0 ; i < FD_SETSIZE; i++) {
        msclientfd[i] = -1;
   }

   for ( ; ; ) {
	client_len = sizeof (client_addr);
	sock_ms = accept(s, (struct sockaddr *) &client_addr, &client_len);
    if (sock_ms < 0)
    {
        printf("can't accept socket, errno=%d\n", errno);
        return (4); /*accept error*/
    }
        for (i=0; i< FD_SETSIZE; i++) 
        {
            if ( msclientfd[i] == -1 )
            {
                 msclientfd[i] = sock_ms;
                 break;
            }
        }

        if ((childpid = fork()) == 0 ) /* child process */
        {
          //close (s); /* close listening socket */
          childMain(sock_ms);
          exit(0);
        }
        //close(sock_ms); //this will cause reset_by_peer in XSocket.lib
   } /*end of for */

   return 0;
}


void childMain( int sock_ms)
{

   int sock_vod = -1;
   struct sockaddr_in  ncube_svr_addr;

   if ((menu_choice == ONE_MS_SERVER_AND_NCUBE))
   {
      printf("create a connection to nCube\n");
      use_ncube_flag = 1;
      /* create a vod_client's socket which talks to nCubeserver */
      if ( (sock_vod = socket (AF_INET, SOCK_STREAM, 0)) < 0 ) {
          printf ("Socket error: errno=%d\n", errno);
          return;
      }

      bzero (&ncube_svr_addr, sizeof(ncube_svr_addr));
      ncube_svr_addr.sin_family = AF_INET;
      ncube_svr_addr.sin_port = htons ( NCUBE_SERVER_PORT);
      inet_pton (AF_INET, NCUBE_IP_ADDR, &ncube_svr_addr.sin_addr);

      if ( connect (sock_vod, (struct sockaddr *) &ncube_svr_addr , sizeof(ncube_svr_addr)) < 0 ) {
          printf ("connect error: errno =%d\n", errno);
          return;
      }

     //sprintf (location_hdr,"Location: rtsp://%s:%d\r\n", NCUBE_IP_ADDR,NCUBE_SERVER_PORT);
     //printf("location_hdr=%s\n", location_hdr);
   } //end of menu_choice ==2
   else if (menu_choice ==  ONE_MS_SERVER_AND_ONE_VOD_SERVER)
   {
     sprintf (location_hdr,"Location: rtsp://%s:%d\r\n", vod_server_ip_addr, VOD_SERVER_PORT);
     printf("location_hdr=%s\n", location_hdr);
   }


   for (; ;)
   {
       // read the socket from OpenTV's VOD extension 
       read_from_opentv_vod(sock_ms, sock_vod, use_ncube_flag);

       if (use_ncube_flag ==1 )
          read_from_ncube(sock_ms, sock_vod, use_ncube_flag); 

   }  

     //close(sock_ms);

   return;
}


   
void   read_from_opentv_vod(int sock_ms, int sock_vod, int use_ncube_flag)
{
   int  line_num=0;
   int  rc=-1;
   char line[MAX_LINE_NUMBER][256];
   char msgBuffer[MAX_BUFFER_SIZE];

   bzero(msgBuffer, MAX_BUFFER_SIZE);
   bzero(line, ((MAX_LINE_NUMBER) * 256));

   printf("===read from OpenTV STB : sock_ms=%d, pthread_id=%d\n", sock_ms,  (int) pthread_self());

   if (read_opentv_msg(sock_ms,msgBuffer) <= 0)
      return;

   line_num = read_line(msgBuffer, line[0]); 

   if (use_ncube_flag == 0) 
   {
      //send back to MS's client, which is OpenTV VOD extension
      rc = process_client_request(sock_ms,  line[0], line_num, is_ms_server );
   }
   else
   {  /* send to ncube if it is not heartbeat GET_PARAMETER
       * the following line needs to be uncommented out when 2
       * servers model are implemented in vod_ifc */
      //if ( strstr(line[0], "GET_PARAMETER") <0 )
         rc = relay_rtsp_msg(sock_vod, line[0], line_num+1);
   }

   return;
}

void   read_from_ncube(int sock_ms, int sock_vod, int use_ncube_flag)
{
   int  rc=-1;
   int  line_num =0;
   char line[MAX_LINE_NUMBER][256];
   char msgBuffer[MAX_BUFFER_SIZE];

   bzero(msgBuffer, MAX_BUFFER_SIZE);
   bzero(line, ((MAX_LINE_NUMBER) * 256));

   printf("++++ read_from_ncube (): sock_vod=%d\n", sock_vod);

   if ( read_ncube_msg(sock_vod, msgBuffer) == 1)
   {
      line_num = read_line(msgBuffer, line[0]); 
      //customize_ncube_response(line[0], &line_num);
      rc = relay_rtsp_msg(sock_ms,  line[0], line_num+1 );
   }


   return;
}
