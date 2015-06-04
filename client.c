#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_BUFFER_SIZE 	1024
#define MAX(a, b)		((a) > (b) ? (a) : (b))
#define MAX_LINE_NUMBER  30

extern int   fileno(FILE *);
void echo_request (int );
void sendSETUPmsg(int);
void sendGETPARAMETERmsg(int);
void sendDESCRIBEmsg(int);
void sendPLAYmsg(int);
void sendTEARDOWNmsg(int);
int write_number_of_bytes (int , char *, int);
void clientReader ( int *sock);

int cseq =1;
int server_port = 554;

int main (int argc, char ** argv)
{
    int s;
	struct sockaddr_in server_addr;
    int choice;
    pthread_t        client_reader_id;

	if (argc <2 ) {
		printf ("client usage:  ./client <server's IP address> \n");
		printf ("               e.g., ./client 10.65.3.149 \n");
		printf ("      or       e.g., ./client 10.65.3.149  555\n");
		return (1);
	}

	/* create a TCP socket */
	if ( (s = socket (AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf ("SOcket error: errno=%d\n", errno);
		return(1);
	}

	bzero (&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
    if (argc == 3 )
       server_port = atoi(argv[2]); 

    printf("server ip  : %s\n", argv[1]);
    printf("server port : %d\n", server_port);

	server_addr.sin_port = htons (server_port);
	inet_pton (AF_INET, argv[1], &server_addr.sin_addr);

 	if ( connect (s, (struct sockaddr *) &server_addr , sizeof(server_addr)) < 0 ) {
		printf ("connect error: errno =%d\n", errno);
		return(1);
	}

    if (pthread_create(&client_reader_id, NULL, (void *)clientReader, (void *)&s) != 0)
    {
       printf ("Cannot create new thread.\n");
           exit(1);
    }

//	echo_request (s);
    while (1)
    {
      printf("\n\n\t\t 1. send SETUP\n");
      printf("\t\t 2. send GET_PARAMETER\n");
      printf("\t\t 3. send DESCRIBE\n");
      printf("\t\t 4. send PLAY\n");
      printf("\t\t 5. send TEARDOWN\n\n");
      scanf("%d", &choice);
      switch (choice)
      {
        case 1 : //SETUP
           sendSETUPmsg( s );
           break;

        case 2 : //GET_PARAMETER
           sendGETPARAMETERmsg(s);
           break;

        case 3 : //DESCRIBE
           sendDESCRIBEmsg(s);
           break;

        case 4 : //PLAY
           sendPLAYmsg(s);
           break;

        case 5 : //TEARDOWN
           sendTEARDOWNmsg(s);
           break;

        default:
           break;

       } //end of switch
    }

	return 0;

}


void sendSETUPmsg( int socket)
{
   char reqBuf[MAX_LINE_NUMBER][256];
   char *ptrTemp, *ptrReq, buffer[MAX_BUFFER_SIZE];
   int line_num = 0;
   int nbytes, i;
   char cseq_str[256];

   bzero(cseq_str, 256);
   sprintf(cseq_str, "CSeq: %d\r\n", cseq);
   cseq++;

   bzero(reqBuf, sizeof(char)*MAX_LINE_NUMBER*256);

   strcpy(reqBuf[line_num++], "SETUP rtsp://192.168.0.241/vanhelsing.mpi RTSP/1.0\r\n");
   strcpy(reqBuf[line_num++], cseq_str);
   strcpy(reqBuf[line_num++], "Transport: MP2T/H2221/UDP;unicast;client_port=8000;mode=play\r\n");
   strcpy(reqBuf[line_num],"\r\n");


   ptrReq = buffer; 

   bzero( ptrReq, MAX_BUFFER_SIZE);

   ptrTemp= ptrReq; 

   printf("===Sending : \n");
   for (i=0; i< line_num ; i++)
   {
       printf("reqBuf[%d]=%s", i, reqBuf[i]);
       nbytes = strlen( reqBuf[i]);
       /*printf("i=%d, nbytes=%d\n", i, nbytes);*/
       strncpy(ptrTemp, reqBuf[i], nbytes);
       ptrTemp  += nbytes; 

    } /*end of for */

    *ptrTemp = '\r';
    ptrTemp++;
    *ptrTemp = '\n';
    ptrTemp++;

    nbytes= ptrTemp - ptrReq; 
    //printf("sending: %d bytes %s\n", nbytes, ptrReq);
    write( socket, ptrReq, nbytes);
}


void sendGETPARAMETERmsg(int socket)
{
   char reqBuf[MAX_LINE_NUMBER][256];
   char *ptrTemp, *ptrReq, buffer[MAX_BUFFER_SIZE];
   int line_num = 0;
   int nbytes, i;

   char cseq_str[256];

   bzero(cseq_str, 256);
   sprintf(cseq_str, "CSeq: %d\r\n", cseq);
   cseq++;

   bzero(reqBuf, sizeof(char)*MAX_LINE_NUMBER*256);

   strcpy(reqBuf[line_num++], "GET_PARAMETER rtsp://192.168.0.241/vanhelsing.mpi RTSP/1.0\r\n");
   strcpy(reqBuf[line_num++], cseq_str);
   strcpy(reqBuf[line_num++], "Session: 4050682558055376826\r\n");
   strcpy(reqBuf[line_num],"\r\n");


   ptrReq = buffer; 

   bzero( ptrReq, MAX_BUFFER_SIZE);

   ptrTemp= ptrReq; 

   printf("===Sending : \n");
   for (i=0; i< line_num ; i++)
   {
       printf("reqBuf[%d]=%s", i, reqBuf[i]);
       nbytes = strlen( reqBuf[i]);
       /*printf("i=%d, nbytes=%d\n", i, nbytes);*/
       strncpy(ptrTemp, reqBuf[i], nbytes);
       ptrTemp  += nbytes; 

    } /*end of for */


    *ptrTemp = '\r';
    ptrTemp++;
    *ptrTemp = '\n';
    ptrTemp++;

    nbytes= ptrTemp - ptrReq; 
    //printf("sending: %d bytes %s\n", nbytes, ptrReq);
    write ( socket, ptrReq, nbytes);
}


void sendDESCRIBEmsg( int socket)
{
   char reqBuf[MAX_LINE_NUMBER][256];
   char *ptrTemp, *ptrReq, buffer[MAX_BUFFER_SIZE];
   int line_num = 0;
   int nbytes, i;
   char cseq_str[256];

   bzero(cseq_str, 256);
   sprintf(cseq_str, "CSeq: %d\r\n", cseq);
   cseq++;

   bzero(reqBuf, sizeof(char)*MAX_LINE_NUMBER*256);

   strcpy(reqBuf[line_num++], "DESCRIBE rtsp://192.168.0.241/vanhelsing.mpi RTSP/1.0\r\n");
   strcpy(reqBuf[line_num++], cseq_str);
   strcpy(reqBuf[line_num++], "Session: 4050682558055376826\r\n");
   strcpy(reqBuf[line_num],"\r\n");


   ptrReq = buffer; 

   bzero( ptrReq, MAX_BUFFER_SIZE);

   ptrTemp= ptrReq; 

   printf("===Sending : \n");
   for (i=0; i< line_num ; i++)
   {
       printf("reqBuf[%d]=%s", i, reqBuf[i]);
       nbytes = strlen( reqBuf[i]);
       /*printf("i=%d, nbytes=%d\n", i, nbytes);*/
       strncpy(ptrTemp, reqBuf[i], nbytes);
       ptrTemp  += nbytes; 

    } /*end of for */

    *ptrTemp = '\r';
    ptrTemp++;
    *ptrTemp = '\n';
    ptrTemp++;

    nbytes= ptrTemp - ptrReq; 
    //printf("sending: %d bytes %s\n", nbytes, ptrReq);
    write( socket, ptrReq, nbytes);
}





void sendPLAYmsg( int socket)
{
   char reqBuf[MAX_LINE_NUMBER][256];
   char *ptrTemp, *ptrReq, buffer[MAX_BUFFER_SIZE];
   int line_num = 0;
   int nbytes, i;
   char cseq_str[256];

   bzero(cseq_str, 256);
   sprintf(cseq_str, "CSeq: %d\r\n", cseq);
   cseq++;

   bzero(reqBuf, sizeof(char)*MAX_LINE_NUMBER*256);

   strcpy(reqBuf[line_num++], "PLAY rtsp://192.168.0.241/vanhelsing.mpi RTSP/1.0\r\n");
   strcpy(reqBuf[line_num++], cseq_str);
   strcpy(reqBuf[line_num++], "Session: 4050682558055376826\r\n");
   strcpy(reqBuf[line_num++], "Scale: 1.0\r\n");
   strcpy(reqBuf[line_num++], "Range: npt=0.000-\r\n");
   strcpy(reqBuf[line_num],"\r\n");


   ptrReq = buffer; 

   bzero( ptrReq, MAX_BUFFER_SIZE);

   ptrTemp= ptrReq; 

   printf("===Sending : \n");
   for (i=0; i< line_num ; i++)
   {
       printf("reqBuf[%d]=%s", i, reqBuf[i]);
       nbytes = strlen( reqBuf[i]);
       /*printf("i=%d, nbytes=%d\n", i, nbytes);*/
       strncpy(ptrTemp, reqBuf[i], nbytes);
       ptrTemp  += nbytes; 

    } /*end of for */

    *ptrTemp = '\r';
    ptrTemp++;
    *ptrTemp = '\n';
    ptrTemp++;

    nbytes= ptrTemp - ptrReq; 
    //printf("sending: %d bytes %s\n", nbytes, ptrReq);
    write( socket, ptrReq, nbytes);
}


void sendTEARDOWNmsg( int socket)
{
   char reqBuf[MAX_LINE_NUMBER][256];
   char *ptrTemp, *ptrReq, buffer[MAX_BUFFER_SIZE];
   int line_num = 0;
   int nbytes, i;
   char cseq_str[256];

   bzero(cseq_str, 256);
   sprintf(cseq_str, "CSeq: %d\r\n", cseq);
   cseq++;

   bzero(reqBuf, sizeof(char)*MAX_LINE_NUMBER*256);

   strcpy(reqBuf[line_num++], "TEARDOWN rtsp://192.168.0.241/vanhelsing.mpi RTSP/1.0\r\n");
   strcpy(reqBuf[line_num++], cseq_str);
   strcpy(reqBuf[line_num++], "Session: 4050682558055376826\r\n");
   strcpy(reqBuf[line_num],"\r\n");


   ptrReq = buffer; 

   bzero( ptrReq, MAX_BUFFER_SIZE);

   ptrTemp= ptrReq; 

   printf("===Sending : \n");
   for (i=0; i< line_num ; i++)
   {
       printf("reqBuf[%d]=%s", i, reqBuf[i]);
       nbytes = strlen( reqBuf[i]);
       /*printf("i=%d, nbytes=%d\n", i, nbytes);*/
       strncpy(ptrTemp, reqBuf[i], nbytes);
       ptrTemp  += nbytes; 

    } /*end of for */

    *ptrTemp = '\r';
    ptrTemp++;
    *ptrTemp = '\n';
    ptrTemp++;

    nbytes= ptrTemp - ptrReq; 
    //printf("sending: %d bytes %s\n", nbytes, ptrReq);
    write( socket, ptrReq, nbytes);
}

void clientReader ( int *sock)
{
   char stringBuffer[MAX_BUFFER_SIZE];
   int  n=0;

   for (; ;)
   {

            bzero(stringBuffer, MAX_BUFFER_SIZE);
            n = read (*sock, stringBuffer, MAX_BUFFER_SIZE);
            printf("++++++Recived: %d bytes++++++++\n", n);
            printf("%s\n",stringBuffer);
    }
   return; 
}

