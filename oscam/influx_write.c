#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

void error(const char *msg) { perror(msg); exit(0); }

int main(int argc, char **argv) {
	char p_n[128], oscam_port[64],influx_str[128];
	if (argc == 1) {
		strcpy(p_n,"Oscam-noport ");
		strcpy(oscam_port,"no-port");
	} else {
		strcpy(p_n,"Oscam-");
		strcat(p_n,*(argv+1));
		strcat(p_n," ");
		strcpy(oscam_port,*(argv+1));
	}
	char buffer[128];
	int result, nread;
	fd_set inputs, testfds;
	struct timeval timeout;
	FD_ZERO(&inputs);
	FD_SET(0, &inputs);
	int fr;		// Freq card request for 10s

// Socker initialize:
	const char * host = "172.17.166.15";
	const int port = 8086;
	char message[1024], in_buf[512];
	int total;
	int sockfd = 0;
	struct sockaddr_in serv_addr;
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	if(inet_pton(AF_INET, host, &serv_addr.sin_addr)<=0) { error("\ninet_pton error occured.\n"); }

	while(1) {
		fr = 0;
  		for (int t_c=0;t_c<10;t_c++) {
	  		sleep(1);
			testfds = inputs;
			timeout.tv_sec = 0;
			timeout.tv_usec = 1000;
			result = select(FD_SETSIZE, &testfds, (fd_set *)NULL, (fd_set*)NULL, &timeout);
			switch(result) {
				case 0:
					break;
				case -1:
					printf("Input error\n");
					break;
				default:
					if (FD_ISSET(0, &testfds)) {
						ioctl(0, FIONREAD, &nread);
						if (nread != 0) {
							nread = read(0, buffer, nread);
							buffer[nread] = 0;
//Analyze:
							int str_len = strlen(buffer);
							char *c = buffer;
							for (int i=0; i<str_len; i++) {
								switch (buffer[i]) {
									case '\n':
										buffer[i] = 0;
										if (strstr(c,": found")) {
											fr += 1;
										}
										printf("%s%s\n",p_n,c);
										c = buffer+i+1;
										break;
									case 0:
										break;
									default:
										break;
								}
							}
						}
					}
					break;
			}
		}	// for
// Write to influx:
		if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("\n Error : Could not create socket.\n");
		} else if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			printf("\n Error : Connect Failed.\n");
		} else {
			sprintf(influx_str,"count,port=%s fr=%d",oscam_port,fr);
			sprintf(message,"POST /write?db=oscam HTTP/1.1\nHost: %s\nContent-Length: %d\n\n%s",host,(int)(strlen(influx_str)),influx_str);
//			printf("\n%s\n",message);
			write(sockfd,message,(int)strlen(message));
			close(sockfd);
		}
	}
}
