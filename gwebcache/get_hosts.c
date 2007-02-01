#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ctype.h>

#define BUFSIZE 2048


int main(int argc, char *argv[]) {
    
    FILE *fpin;
	FILE *fpout;
   	char buf[256];
	char host[256];
	char extra[256];
	char *colon;
	char *rest;
	char ip[16];
	int port;
	int sd;
	struct sockaddr_in servaddr;
	int retval = 0;
	int handshake_ret = 0;
	int r;

	       

        if(argc < 2) {
                printf("Usage: %s <cache filename>\n",argv[0]);
                exit(-1);
        }

        if((fpin = fopen(argv[1],"r")) == NULL) {
                printf("fopen error\n");
                exit(-1);
        }

	

        while(fgets(buf,254,fpin)) {
 	               
			buf[strlen(buf)-1] = '\0';
                
			//check to see if shit has alternate port        
			if((colon = index(buf,':')) != NULL) {
	                
				*(colon) = '\0';
				strcpy(host,buf); 
				colon = colon+1;
				port = atoi(colon);
	
				//check to see if theres shit after the port
				if((rest = index(colon,'/')) != NULL) {
					*(rest) = '\0';
					rest = rest+1;
					strcpy(extra,"/");
					strcat(extra,rest);
				}
				             
			} else {
				//check to see if theres shit after the port
				if((rest = index(buf,'/')) != NULL) {
					*(rest) = '\0';
					rest = rest+1;
					strcpy(extra,"/");
					strcat(extra,rest);
				}
				
				strcpy(host,buf);
			}
        	
			//or whatever you had here...   
                
//			printf("host: %s, port: %d, extra: %s\n",host,port,extra);


			r = attempt_connect(host,port,extra);

			port = 80;
			memset(extra,0x00,256);
			memset(host,0x00,256);
        }

}

int attempt_connect(char *host, int port, char *extra) 
{
        int sd;
        struct sockaddr_in servaddr;
        int retval = 0;
        int handshake_ret = 0;
        int r;
    	struct hostent *h;
        char *ip;

        //for testing purposes....

        sd = socket(AF_INET, SOCK_STREAM, 0);
    	if (sd == -1) {
        	//perror("socket");
        	return -1;
    	}


    	if ((h=gethostbyname(host)) == NULL) {  // get the host info
        	//herror("gethostbyname");
        	return -1;
			//exit(1);
    	}
	
	    //printf("Host name  : %s\n", h->h_name);
	    //printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *)h->h_addr)));
	   
	    ip = (char *)inet_ntoa(*((struct in_addr *)h->h_addr));
	
	    //printf("--created a socket\n");
	
	    memset(&servaddr, 0, sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_port = htons(port);
	    if (inet_pton(AF_INET, (const char *)ip, &servaddr.sin_addr) <= 0) {
	        //perror("inet_pton");
	        return -2;
	    }

   	 	//printf("--setup the server struct\n");
    	//printf("--attempting to connect\n");


		retval = connect(sd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    	if (retval == -1) {
    	    //perror("connect");
    	    return -3;
    	}
        
        if((handshake_ret = handle_handshake(sd,1,host,port,extra)) == 0) {
                //printf("return was 0!\n");
        } else {
                //handle_handshake(sd,1,host,port,extra);
                //printf("return was not 0\n");
        }

		close(sd);
        return 0;

}

int handle_handshake(int fd,int datatry,char *host, int port, char *excess) {
	char hdr1[256];
	char hdr2[256];
	int retval = 0;
	char recv_buff[BUFSIZE];
	char tempbuf[BUFSIZE];
	char *ipportcombo;
	char ip[16];
	int newport;
	char *templine;
	char *line;
	char *line2;
	char *saveptr1, *saveptr2;
	char *colon;

	//printf("--attempting to write\n");
        
        
	if(datatry == 1) {
		sprintf(hdr1,"GET %s?client=XXXX&hostfile=1 HTTP/1.1\r\nHost: %s:%d\r\n\r\n",excess,host,port); 
		write(fd, (const void *)hdr1, strlen(hdr1));
		//printf("Wrote (%s)\n", hdr1);
	} else {
		sprintf(hdr2,"GET %s?client=XXXX&urlfile=1 HTTP/1.1\r\nHost: %s:%d\r\n\r\n",excess,host,port);
		write(fd, (const void *)hdr2, strlen(hdr2));
		//printf("Wrote (%s)\n", hdr2);
	}

    retval = recv(fd, (char *)recv_buff,BUFSIZE-1,0);
    recv_buff[retval] = '\0';

    //printf("Read (%s).\n", recv_buff);
	
	if(strlen(recv_buff) > 2) {
		memset(tempbuf,0x00,BUFSIZE);
		//this is where we get to parse everything!
		//check for \r\n\r\n, then 				
		
		
	    	strcpy(tempbuf,recv_buff);
			if ((templine = strstr(tempbuf, "\r\n\r\n")) != NULL) {
                        
				//printf("templine: %s\n",templine);
        	    templine += 4;
		
				
			    line = strtok_r(templine, "\r\n",&saveptr1);

				while ( (line=strtok_r(saveptr1, "\r\n",&saveptr1)) != NULL)        
        		{
        			if((templine = strstr(line,".")) != NULL) {
						templine+=1;
	        			if((templine = strstr(line,".")) != NULL) {
							templine+=1;	        			
							if((templine = strstr(line,".")) != NULL) {
								templine+=1;
								if((templine = strstr(line,":")) != NULL) {
									//printf("ok this should be an ip: %s\n",line);
								
									//now lets do shit with it, put it in a list?
									colon = index(line,':');
                					*(colon) = '\0';
                					strcpy(ip,line); 
                					colon = colon+1;
                					newport = atoi(colon);									
		
									if(newport == 0) {
										return;
									}
				                    printf("%s:%d\n", ip, newport);	
									//printf("new host: %s, new ip: %d\n",ip,newport);
				
									memset(ip,0x00,16);
									newport = 0;
								}
														
				        	}
	
				        }
					}
				}
	    	}
    }    
        

	return 2;

}
