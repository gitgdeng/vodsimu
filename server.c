#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <sys/select.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>

#include <server.h>


typedef void (*sighandler_t)(int);

/* global variables */
extern char vod_server_ip_addr[256];
extern uint16_t VOD_SERVER_PORT;
extern rtsp_server_modes_t menu_choice;
extern rtsp_testcase_t sub_choice;
extern char location_hdr[256];
extern int vod_stream_transport_type;

extern int startTimer (unsigned int);
extern void init_timer(void );

int process_client_request (int ,  char *, int, int);
int relay_rtsp_msg(int, char *, int);
int write_number_of_bytes (int , char *, int); 
//int read_each_line (int , char *, int * );
int send_announce (int socket);
int cseq_parser(char *line);
int read_line(char *, char *);
int read_opentv_msg(int s, char *msgBuffer);
int read_ncube_msg(int s, char *msgBuffer);
int read_virtual_vod_server_msg(int s, char *msgBuffer);
int customize_ncube_response(char *line, int *total_line_num);

#if 0
static int read_one ( int s, char *c, int *lenptr)
{
    static int nread = 0;
    static char *ptr;
    static char readBuf[MAX_BUFFER_SIZE];
    char                *tmp, tmp_string[MAX_BUFFER_SIZE] ;
    int                 tmp_idx;
    int  rc;

    if (nread == 0 ) {
        repeat:
        if (( nread = read (s, readBuf, sizeof(readBuf))) < 0) {
            if (errno == EINTR )
                goto repeat;
            return (-1);
        } else if (nread == 0)
            return 0;

        ptr = readBuf;
        //printf("======= total nread=%d\n", nread);
    }

    tmp = ptr;
    tmp_idx = 0;

    while ( (*tmp != 0x0d) && (*(tmp+1) != 0x0a) )
    {
           tmp_string[tmp_idx] = *tmp;
           tmp++;
           tmp_idx++;
    }
    //printf("nread=%d, tmp_idx=%d\n", nread, tmp_idx);
    tmp_string[tmp_idx]= *tmp;
    tmp_string[tmp_idx+1]= *(tmp+1);

    nread -= tmp_idx + 2;
    tmp_string[tmp_idx+2] = '\0';
    strcpy(c, tmp_string);

    //printf("nread=%d, tmp_string=%s\n", nread, tmp_string);
    rc = tmp_idx + 2 ; 
    ptr += tmp_idx + 2;

    *lenptr = nread;

    if (nread == 0)
       return 2;
    else
       return rc;



}

int read_each_line (int s, char *readBuf, int *lenptr)
{
    int  rc;

    rc = read_one (s, readBuf, lenptr);
    //printf("read_each_line (rc=%d) =%s", rc, readBuf); 
    return rc;
}

#endif

int read_line(char *msgBuffer, char *line)
{

    static char *ptr;
    char        *tmp, tmp_string[MAX_BUFFER_SIZE] ;
    int         tmp_idx;
    int         i;

    ptr = msgBuffer;
   
    for (i=0; i< MAX_LINE_NUMBER; i++)
    {
       bzero(tmp_string, MAX_BUFFER_SIZE);

       tmp = ptr;
       tmp_idx = 0;

       while ( (*tmp != 0x0d) && (*(tmp+1) != 0x0a) )
       {
           tmp_string[tmp_idx] = *tmp;
           tmp++;
           tmp_idx++;
       }
       //printf("nread=%d, tmp_idx=%d\n", nread, tmp_idx);
       tmp_string[tmp_idx]= *tmp;
       tmp_string[tmp_idx+1]= *(tmp+1);

       strcpy(line, tmp_string);
       printf("read_line: line[%d]=%s", i, tmp_string);

       line += 256;
       ptr += tmp_idx+2;
       if (ptr >= msgBuffer + strlen(msgBuffer))
         return i; 
    }
    return 0;
}

int read_opentv_msg(int s, char *msgBuffer)
{
    static int  nread = 0;
    static char *ptr;
    static char readBuf[MAX_BUFFER_SIZE];
    char        *tmp, *tmp_2, tmp_string[MAX_BUFFER_SIZE] ;

    if (nread == 0 ) {
        bzero(readBuf, MAX_BUFFER_SIZE);
        repeat:
        if (( nread = read (s, readBuf, sizeof(readBuf))) < 0) {
            if (errno == EINTR )
                goto repeat;
            return (-1);
        } else if (nread == 0)
            return 0;

        ptr = readBuf;
        printf("======= total nread=%d, readBuf=%s\n", nread, readBuf);
    }

    bzero(tmp_string, MAX_BUFFER_SIZE);
    tmp= strstr(ptr, "\r\n\r\n");

    if (tmp > 0 )
    {
        tmp += 4;
        strncpy(tmp_string, ptr, (tmp-ptr));
    }
    else
    {
        strcpy(tmp_string, ptr); /* copy the whole string*/
        nread = 0;
    }
    
    if ( strstr(tmp_string, "Content-Length") == NULL)
    {
        strcpy(msgBuffer, tmp_string);
        nread -= tmp-ptr;
        ptr = tmp; 
        return 1 ;
    }
    else /* contain msg body when Content-Length presents */
    {
       tmp_2 = strstr(tmp, "\r\n\r\n");
       if (tmp_2 >0)
       {
          tmp = tmp_2 + 4;
          nread -= tmp-ptr;
          strncpy(msgBuffer, ptr, (tmp-ptr));
          msgBuffer[tmp-ptr] = '\0'; /* null terminate it */
       }
       else
       {
         strcpy(msgBuffer, ptr);
         ptr = tmp; 
         nread = 0;
       }
       return 1; 

    }
}




int read_ncube_msg(int s, char *msgBuffer)
{
    static int  nread = 0;
    static char *ptr;
    static char readBuf[MAX_BUFFER_SIZE];
    char        *tmp, *tmp_2, tmp_string[MAX_BUFFER_SIZE] ;

    if (nread == 0 ) {
        bzero(readBuf, MAX_BUFFER_SIZE);
        repeat:
        if (( nread = read (s, readBuf, sizeof(readBuf))) < 0) {
            if (errno == EINTR )
                goto repeat;
            return (-1);
        } else if (nread == 0)
            return 0;

        ptr = readBuf;
        printf("======= total nread=%d, readBuf=%s\n", nread, readBuf);
    }

    bzero(tmp_string, MAX_BUFFER_SIZE);
    tmp= strstr(ptr, "\r\n\r\n");

    if (tmp > 0 )
    {
        tmp += 4;
        strncpy(tmp_string, ptr, (tmp-ptr));
    }
    else
    {
        strcpy(tmp_string, ptr); /* copy the whole string*/
        nread = 0;
    }
    
    if ( strstr(tmp_string, "Content-Length") == NULL)
    {
        strcpy(msgBuffer, tmp_string);
        nread -= tmp-ptr;
        ptr = tmp; 
        return 1 ;
    }
    else /* contain msg body when Content-Length presents */
    {
       tmp_2 = strstr(tmp, "\r\n\r\n");
       if (tmp_2 >0)
       {
          tmp = tmp_2 + 4;
          nread -= tmp-ptr;
          strncpy(msgBuffer, ptr, (tmp-ptr));
          msgBuffer[tmp-ptr] = '\0'; /* null terminate it */
       }
       else
       {
         strcpy(msgBuffer, ptr);
         ptr = tmp; 
         nread = 0;
       }
       return 1; 

    }

   

}



/* Function name: write_number_of_bytes 
 * This function writes on a socket where output might be less then the requested
 * due to the buffer limit in kernel.  It call write function again to output the 
 * remaining bytes 
 */
int write_number_of_bytes (int s, char *stringBuffer, int n)
{
    unsigned int nleft;
    int nwrite;
    char *ptrWrite;

    ptrWrite = stringBuffer;
    nleft = n;
    
    while (nleft >0) {
        if ( (nwrite = write (s, ptrWrite, nleft)) < 0 ) {
            if (errno == EINTR)
                nwrite = 0; /* call write again */
            else
                return (-1);
        }
        nleft -= nwrite;
        ptrWrite += nwrite;
    }

    return n;
}


int send_announce (int socket)
{
   char reqBuf[MAX_LINE_NUMBER][256];
   char *ptrTemp, *ptrReq, buffer[MAX_BUFFER_SIZE];
   int line_num = 0;
   int nbytes, i;


   /* build announce msg */
   bzero(reqBuf, sizeof(char)*MAX_LINE_NUMBER*256);
   strcpy(reqBuf[line_num++], "ANNOUNCE rtsp://192.168.0.20/vanhelsing.mpi RTSP/1.0\r\n");
   strcpy(reqBuf[line_num++], "CSeq: 1\r\n");
   strcpy(reqBuf[line_num++], "Session: 4050682558055376826\r\n");
   strcpy(reqBuf[line_num++], "Require: method.announce\r\n");
   if (sub_choice == ANNOUNCE_EVENT_CODE_ERROR )
      strcpy(reqBuf[line_num++], "Notice: 5502 Internal Server error\r\n");
   else
      strcpy(reqBuf[line_num++], "Notice: 2101 End-of-Stream Reached\r\n");
   strcpy(reqBuf[line_num++], "Range: npt=10-100\r\n");
   strcpy(reqBuf[line_num++], "Content-Type: text/parameters\r\n");
   strcpy(reqBuf[line_num++], "Content-Length: 36\r\n");
   strcpy(reqBuf[line_num++],   "\r\n");
   if (sub_choice == ANNOUNCE_EVENT_CODE_ERROR )
      strcpy(reqBuf[line_num++], "eos-reason: Internal Server error.\r\n");
   else
      strcpy(reqBuf[line_num++], "eos-reason: End of file reached.\r\n");
   strcpy(reqBuf[line_num],   "\r\n");
   /* end of announce msg body */

   ptrReq = buffer;

   bzero( ptrReq, MAX_BUFFER_SIZE);

   ptrTemp= ptrReq;

   for (i=0; i< line_num ; i++)
   {
       printf("reqBuf[%d]=%s", i, reqBuf[i]);
       nbytes = strlen( reqBuf[i]);
       /*printf("i=%d, nbytes=%d\n", i, nbytes);*/
       strncpy(ptrTemp, reqBuf[i], nbytes);
       ptrTemp  += nbytes;

    } /*end of for */

    *ptrTemp= '\r';
    ptrTemp++;
    *ptrTemp= '\n';

    nbytes= ptrTemp - ptrReq;
    write_number_of_bytes ( socket, ptrReq, nbytes);
    printf("send_announce: nbytes=%d\n",nbytes);

    return 0;
}

int customize_ncube_response(char *line, int *total_line_num)
{

   rtsp_method_t method = UNKNOWN;
   char  *ptr;
   int i;

   ptr = line;

   for (i=0; i< *total_line_num; i++)
   {
      if (strstr(ptr, "Transport") >0 )
      {
         method = SETUP;
         break;
      }
      ptr += 256;
   }

   switch (method)
   {
      case SETUP:
           ptr = line +256 *( *total_line_num); /* move to the begining of the last line */      
           printf(" SETUP location_hdr=%s\n",  location_hdr);
           strcpy(ptr, location_hdr);
           ptr += 256;
           strcpy(ptr, "\r\n");
           *total_line_num = *total_line_num +1;
        break;

      default:
        break;

   }

   return 0;

}

int process_client_request (int sock_s,  char *line, int total_line_num, int is_ms_server)
{
   unsigned int nleft;
   int nwrite;
   char *ptrWrite, *ptrTemp, writeBuf[MAX_BUFFER_SIZE];
   char responseBuf[MAX_LINE_NUMBER][256];
   int i, next_line_num;
   rtsp_method_t method = UNKNOWN;
   int CSeq;
   int CL_line_num = 0;

   nleft = 0;
   nwrite = 0;
    
   bzero(responseBuf, sizeof(char) * MAX_LINE_NUMBER*256);
   printf("\nprocess_client_request(): \n");
   for (i=0; i< total_line_num; i++)
   {
       //printf("rcvd line[%d]= %s", i,line);
       if ( i == 0 ) { /* the first line in the request */ 
           if (strstr(line, "SETUP") >0 ) {
                 method = SETUP;
                 printf("\tmethod = SETUP\n");
           } else  if ( strstr(line, "PLAY") > 0 ) {
                 method = PLAY;
                 printf("\tmethod = PLAY\n");
           } else if ( strstr(line, "TEARDOWN") > 0 ) {
                 method = TEARDOWN;
                 printf("\tmethod = TEARDOWN\n");
           } else if ( strstr(line, "DESCRIBE") > 0 ) {
                 method = DESCRIBE;
                 printf("\tmethod = DESCRIBE\n");
           } else if ( strstr(line, "GET_PARAMETER") >0 ) {
                 method = GET_PARAMETER;
                 printf("\tmethod = GET_PARAMETER\n");
           } else if ( strstr(line, "PAUSE") > 0 ) {
                 method = PAUSE;
                 printf("\tmethod = PAUSE\n");
           } else if ( strstr(line, "200 OK") >0 ) {
                 method = ANNOUNCE_REPLY;
                 printf("\tmethod = ANNOUNCE_REPLY");
           } else {
                 method = UNKNOWN;
                 //printf("\tmethod = UNKNOWN\n");
           }

         
       } /* done with the fisrt line */
       else 
       { /*the rest lines */ 
           if ( strstr(line, "Content-Length") > 0 )   /* skip the conten-Length, we need to add 
                                                           Content_Lenght according to how many 
                                                           bytes we send out*/
           {
              CL_line_num =i; //will be used for Get_parameter later
           }
           else   //has no Content-Length
           {
 
              if (( strstr(line, "Range:") > 0 ) && (sub_choice == PLAY_NO_RANGE_HEADER))
              {
                /* for testing when no Range: hdr is not include in PLAY response */
                //do not copy teh line
              }
              else
              {
                 strcpy(responseBuf[i], line);
              }
           }


           if (strstr(line, "CSeq") > 0)
           {
              CSeq = cseq_parser(line);
              //printf("\n CSeq is %d\n", CSeq); 
           }

       } /*done the rest lines*/        

       line= line +256; /* move to next line */      

   } /* done for loop */

   next_line_num =i;

   /*printf("total lines: next_line_num=%d\n", next_line_num);*/

   /* build response msg */

   switch (method)
   {
     case SETUP:
         if (sub_choice == SETUP_ERROR)
         {
            strcpy(responseBuf[0], "RTSP/1.0 404 NOT FOUND\r\n");
            strcpy(responseBuf[next_line_num],"\r\n");
    
           break;
         }
        
         /* NORMAL case: 200 OK */ 

//         if (menu_choice != ONE_MS_SERVER_AND_ONE_VOD_SERVER)
//         {
//         strcpy(responseBuf[0], "RTSP/1.0 200 OK\r\n");
//         strcpy(responseBuf[next_line_num++], "Session: 4050682558055376826\r\n");
//         }
//         else// (menu_choice == ONE_MS_SERVER_AND_ONE_VOD_SERVER)
//         {
            next_line_num = 0;
            strcpy(responseBuf[next_line_num++], "RTSP/1.0 200 OK\r\n");
            strcpy(responseBuf[next_line_num++], "CSeq: 1\r\n");
            if (sub_choice == SETUP_HAS_TIMEOUT_VALUE_RETURNED)
              strcpy(responseBuf[next_line_num++], "Session: 5059691751022412168;timeout=60\r\n");
            else
              strcpy(responseBuf[next_line_num++], "Session: 5059691751022412168\r\n");


            if (menu_choice == ONE_MS_SERVER_AND_ONE_VOD_SERVER)
            {
                  strcpy(responseBuf[next_line_num++], "ControlSession: 8888691751022412168\r\n");
                  strcpy(responseBuf[next_line_num++], location_hdr);
            }

            if (vod_stream_transport_type == 1 ) //VOD_over IP
            {
               sprintf(responseBuf[next_line_num++], "StreamAddress: %s:%d\r\n", vod_server_ip_addr,VOD_SERVER_PORT);
            }
            else
            {
               sprintf(responseBuf[next_line_num++], "Tuning: frequency=11570\r\n");
            }
            strcpy(responseBuf[next_line_num++], "Channel: Tsid=3;Svcid=12\r\n");
 //        }
            
         strcpy(responseBuf[next_line_num],"\r\n");
       break;

     case PLAY:
         if (sub_choice == PLAY_ERROR)
         {
            strcpy(responseBuf[0], "RTSP/1.0 400 Bad Request\r\n");
            strcpy(responseBuf[next_line_num],"\r\n");
            break;
         }
        
         /* NORMAL case: 200 OK */ 
         strcpy(responseBuf[0], "RTSP/1.0 200 OK\r\n");

       break;
    
     case PAUSE:
         /* NORMAL case: 200 OK */ 
         strcpy(responseBuf[0], "RTSP/1.0 200 OK\r\n");
       break;


     case DESCRIBE:
         if (sub_choice == DESCRIBE_ERROR)
         {
            strcpy(responseBuf[0], "RTSP/1.0 400 Bad Request\r\n");
            strcpy(responseBuf[next_line_num],"\r\n");
            break;
         }
        
         /* NORMAL case: 200 OK */ 
         strcpy(responseBuf[0], "RTSP/1.0 200 OK\r\n");
         
         if (menu_choice != ONE_MS_SERVER_AND_ONE_VOD_SERVER)
         {

            strcpy(responseBuf[next_line_num++],"Date: Tue, 02 May 2006  00:02:31 GMT\r\n");
            strcpy(responseBuf[next_line_num++],"Content-Length: 236\r\n");
            strcpy(responseBuf[next_line_num++],"\r\n");
            strcpy(responseBuf[next_line_num++],"v=0\r\n");
            strcpy(responseBuf[next_line_num++],"o=- 1146528151955250 9727864823646 IN IP4 192.168.0.102\r\n");
            strcpy(responseBuf[next_line_num++],"s=RTSP Session\r\n");
            strcpy(responseBuf[next_line_num++],"t=0 0\r\n");
            strcpy(responseBuf[next_line_num++],"i=\r\n");
            strcpy(responseBuf[next_line_num++],"b=AS:4837\r\n");
            strcpy(responseBuf[next_line_num++],"a=type:vod\r\n");
            strcpy(responseBuf[next_line_num++],"a=range:npt=0-91.052\r\n");
            strcpy(responseBuf[next_line_num++],"c=IN IP4 0.0.0.0\r\n");
            strcpy(responseBuf[next_line_num++],"a=control:rtsp://192.168.0.241/vanhelsing.mpi\r\n");
            strcpy(responseBuf[next_line_num++],"m=video 0 RTP/AVP 33\r\n");
            strcpy(responseBuf[next_line_num++],"a=framerate:25.00\r\n"); 
            strcpy(responseBuf[next_line_num],"\r\n");
         }
         else
         {
            strcpy(responseBuf[next_line_num++],"Content-Type: application/sdp\r\n");
            strcpy(responseBuf[next_line_num++],"Content-Length: 41\r\n");
            strcpy(responseBuf[next_line_num++],"\r\n");
            strcpy(responseBuf[next_line_num++],"i=\r\n");
            strcpy(responseBuf[next_line_num++],"a=type:vod\r\n");
            strcpy(responseBuf[next_line_num++],"a=range:npt=0-139.234\r\n");
            strcpy(responseBuf[next_line_num],"\r\n");
         }
       break;

     case TEARDOWN:
         strcpy(responseBuf[0], "RTSP/1.0 200 OK\r\n");
         strcpy(responseBuf[next_line_num],"\r\n");
       break;
   
     case GET_PARAMETER:
         if (sub_choice == GET_PARAMETER_ERROR)
         {
            strcpy(responseBuf[0], "RTSP/1.0 400 Bad Request\r\n");
            strcpy(responseBuf[next_line_num],"\r\n");
            break;
         }
        
         /* NORMAL case: 200 OK */ 
         strcpy(responseBuf[0], "RTSP/1.0 200 OK\r\n");

         if (CL_line_num >0 )
         {
           next_line_num = CL_line_num;
           if ((menu_choice == ONE_MS_SERVER_AND_ONE_VOD_SERVER))
           {
              if ((is_ms_server == 0) /* VOD server */ ||
                  (sub_choice == GET_PARAMETER_ALWAYS_SEND_MSG_BODY))
              {
                 strcpy(responseBuf[next_line_num++],"Content-Length: 53\r\n");
                 strcpy(responseBuf[next_line_num++],"\r\n");
                 strcpy(responseBuf[next_line_num++],"stream_state: playing\r\n");
                 strcpy(responseBuf[next_line_num++],"position: 92.99\r\n");
                 strcpy(responseBuf[next_line_num++],"Scale: 1.5\r\n");
                 strcpy(responseBuf[next_line_num],"\r\n");
              }
              else /* ms server */
              {
                 strcpy(responseBuf[next_line_num],"\r\n");
              }
           }
           else
           {
              strcpy(responseBuf[next_line_num++],"Content-Length: 53\r\n");
              strcpy(responseBuf[next_line_num++],"\r\n");
              strcpy(responseBuf[next_line_num++],"stream_state: playing\r\n");
              strcpy(responseBuf[next_line_num++],"position: 92.99\r\n");
              strcpy(responseBuf[next_line_num++],"Scale: 2.5\r\n");
              strcpy(responseBuf[next_line_num],"\r\n");
           }
         }
       
       break;

     case ANNOUNCE_REPLY:
     case UNKNOWN:
     default: 
       break;
    } /*end of switch (method) */

   if (method == ANNOUNCE_REPLY  )
   {
     return 0;
   }

   /* printf("total next_line_num=%d\n", next_line_num);*/

   for (i=0; i<= next_line_num; i++)
   {
     printf("\tresponseBuf[%d]=%s", i, responseBuf[i]);
   }

   if (method == UNKNOWN )
   {
     return 0;
   }


   ptrWrite = writeBuf;

   bzero( ptrWrite, MAX_BUFFER_SIZE);

   ptrTemp= ptrWrite;

   for (i=0; i< next_line_num ; i++)
   {
       nleft = strlen( responseBuf[i]);
       /*printf("i=%d, nleft=%d\n", i, nleft);*/
       strncpy(ptrTemp, responseBuf[i], nleft);
       ptrTemp +=  nleft;

    } /*end of for */

    *ptrTemp= '\r';
    ptrTemp++;
    *ptrTemp= '\n';

    nleft= ptrTemp - ptrWrite + 1;

    while (nleft >0) {
          //printf("nleft=%d\n",  nleft);
      if ( (nwrite = write (sock_s, ptrWrite, nleft)) < 0 ) {
        if (errno == EINTR)
            nwrite = 0; /* call write again */
        else
                {
                        printf("ERROR in  write. i=%d\n",i);
                return (-1);
                }
       } /* end if */
       nleft -= nwrite;
       ptrWrite += nwrite;
     } /* end of while (nleft) */

    printf("sent the response, CSeq=%d\n\n", CSeq);

    if ((sub_choice == ANNOUNCE_EVENT_CODE_OK) || ( sub_choice == ANNOUNCE_EVENT_CODE_ERROR))
    {
       if ( CSeq == 6 )
       {
           printf( "send a announce\n");
           send_announce( sock_s);
       }     
       return 0;
    }

    return 0;
}

int cseq_parser(char *line)
{
   char *pCSeq, cseqBuf[64];
   int i = 0;
   int cseq_num =0;

   if ((pCSeq = strstr(line, "CSeq: ")) != NULL) 
   {      
           pCSeq = pCSeq + sizeof("CSeq: ") - 1;
           bzero(cseqBuf,  sizeof(cseqBuf));
           while ( (*pCSeq != 0x0d) && (*(pCSeq+1) != 0x0a) )
           {
               cseqBuf[i++] = *pCSeq;
               pCSeq++;
           }
           cseq_num = atoi(cseqBuf);
   } /* end of CSeq parsing */       

   return cseq_num;
}

int relay_rtsp_msg (int sock,  char *line, int total_line_num)
{
   unsigned int nleft;
   int nwrite;
   char *ptrWrite, *ptrTemp, writeBuf[MAX_BUFFER_SIZE];
   int i;

   nleft = 0;
   nwrite = 0;

   ptrWrite = writeBuf;

   bzero( ptrWrite, MAX_BUFFER_SIZE);

   ptrTemp= ptrWrite;

   for (i=0; i< total_line_num-1 ; i++)
   {
       nleft = strlen( line);
       /*printf("i=%d, nleft=%d\n", i, nleft);*/
       strncpy(ptrTemp, line, nleft);
       ptrTemp +=  nleft;
       line= line +256; /* move to next line */      

    } /*end of for */

    *ptrTemp= '\r';
    ptrTemp++;
    *ptrTemp= '\n';

    nleft= ptrTemp - ptrWrite + 1;

    printf("\n\nrelay_rtsp_msg: sock=%d, ptrWrite=%s\n", sock, ptrWrite);
    while (nleft >0) {
          //printf("nleft=%d\n",  nleft);
      if ( (nwrite = write (sock, ptrWrite, nleft)) < 0 ) {
        if (errno == EINTR)
            nwrite = 0; /* call write again */
        else
                {
                    printf("ERROR in  write. i=%d\n",i);
                    return (-1);
                }
       } /* end if */
       nleft -= nwrite;
       ptrWrite += nwrite;
    } /* end of while (nleft) */
    return 0;
}

int read_virtual_vod_server_msg(int s, char *msgBuffer)
{
    static int  nread = 0;
    static char *ptr;
    static char readBuf[MAX_BUFFER_SIZE];
    char        *tmp, *tmp_2, tmp_string[MAX_BUFFER_SIZE] ;

    if (nread == 0 ) {
        bzero(readBuf, MAX_BUFFER_SIZE);
        repeat:
        if (( nread = read (s, readBuf, sizeof(readBuf))) < 0) {
            if (errno == EINTR )
                goto repeat;
            return (-1);
        } else if (nread == 0)
            return 0;

        ptr = readBuf;
        printf("======= total nread=%d, readBuf=%s\n", nread, readBuf);
    }

    bzero(tmp_string, MAX_BUFFER_SIZE);
    tmp= strstr(ptr, "\r\n\r\n");

    if (tmp > 0 )
    {
        tmp += 4;
        strncpy(tmp_string, ptr, (tmp-ptr));
    }
    else
    {
        strcpy(tmp_string, ptr); /* copy the whole string*/
        nread = 0;
    }
    
    if ( strstr(tmp_string, "Content-Length") == NULL)
    {
        strcpy(msgBuffer, tmp_string);
        nread -= tmp-ptr;
        ptr = tmp; 
        return 1 ;
    }
    else /* contain msg body when Content-Length presents */
    {
       tmp_2 = strstr(tmp, "\r\n\r\n");
       if (tmp_2 >0)
       {
          tmp = tmp_2 + 4;
          nread -= tmp-ptr;
          strncpy(msgBuffer, ptr, (tmp-ptr));
          msgBuffer[tmp-ptr] = '\0'; /* null terminate it */
       }
       else
       {
         strcpy(msgBuffer, ptr);
         ptr = tmp; 
         nread = 0;
       }
       return 1; 

    }
}
