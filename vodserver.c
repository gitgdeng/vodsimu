#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <wait.h>
#include <sys/select.h>
#include <strings.h>

//#include <signal.h>

#include <server.h>



/* global variables */
int vodclientfd[FD_SETSIZE];
uint16_t VOD_SERVER_PORT=555; /* this has to be different from 
                          MS_SERVER_PORT because we can't create
                          socket on same port one same PC*/

extern rtsp_testcase_t menu_choice;

extern int startTimer (unsigned int);
extern void init_timer(void );
extern int write_number_of_bytes (int , char *, int); 
extern int process_client_request (int, char *, int line_num, int is_ms_server);
extern int send_annouce (int socket);
extern int read_line(char *msgBuffer, char *line);
extern int read_opentv_msg(int sock_ms, char *msgBuffer);
extern int read_virtual_vod_server_msg (int sock_vod, char *stringBuffer);

int  mainVOD(void);
void vodChildMain( int sock_vod);

/******************************** mainVOD() **************/
int mainVOD (void)
{
   int                  sock_vod, s;
   socklen_t            client_len;
   pid_t                childpid;
   struct sockaddr_in   client_addr, server_addr;

   /*create a TCP socket*/
   if ( (s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    printf (" Socker error: errno=%d\n", errno);
        return (1) ;   /* Error socket */
   }

   bzero (&server_addr, sizeof (server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);

   server_addr.sin_port = htons ( VOD_SERVER_PORT);

   /*bind the socket*/
   if (  (bind ( s, ( struct sockaddr *) &server_addr, sizeof(server_addr)) ) < 0 ) {
    printf (" Bind error: errno=%d\n", errno);
        return(2);     /* bind error */
   }

  
   /*listen to the socket*/
   if ( (listen(s, LISTEN_QUEUE_LEN)) < 0 ){
    printf (" Listen error: errno=%d\n", errno);
       return (3);   /*list error */
   }

   for ( ; ; ) {

        client_len = sizeof (client_addr);
        sock_vod = accept(s, (struct sockaddr *) &client_addr, &client_len);
        if (sock_vod < 0)
        {
            printf("vodserver can't accept socket, errno=%d\n", errno);
            return (4); /*accept error*/
        }
        if ((childpid = fork()) == 0 ) /* child process */
        {
          //close (s); /* close listening socket */
          vodChildMain(sock_vod);
          exit(0);
        }
    }

   return 0;
}

void vodChildMain( int sock_vod)
{
   char stringBuffer[MAX_BUFFER_SIZE];
   char line[MAX_LINE_NUMBER][256];
   int  n, line_num = 0;

   bzero(stringBuffer, MAX_BUFFER_SIZE);
   for (; ;)
   {

            bzero (&line, sizeof(char)* (MAX_LINE_NUMBER*256));
            printf("===read virtual vod server : sock_vod=%d\n", sock_vod);
            n = read_virtual_vod_server_msg (sock_vod, stringBuffer);
            if ( n == 1 )
            {
               line_num = read_line(stringBuffer, line[0]);
               process_client_request(sock_vod, line[0], line_num, 0);
            }
    }
   return;
}

   

