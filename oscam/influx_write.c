#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream

void error(const char *msg) { perror(msg); exit(0); }

int main(int argc, char **argv) {	// argv[1] - APP_PORT, argv[2] - INFLUX_ADDRESS, argv[3] - INFLUX_PORT

// Init:
// Oscam port:
	char oscam_port[64];
	strcpy(oscam_port,*(argv+1));
// INFLUX DATABASE ADDRESS:
	char host[128];
	strcpy(host,*(argv+2));
// INFLUX DATABASE PORT:
	int port = atoi(*(argv+3));

        std::stringstream ss;
	std::string str;

	char buffer[4096];
	int result, nread;
	bool influx_is_ready=true, influx_is_ready_old_status;	// Influx database status, zero_count=true - 2 zero count
	fd_set inputs, testfds;
	struct timeval timeout;
	FD_ZERO(&inputs);
	FD_SET(0, &inputs);
	int fr;		// Freq card request for 10s
	bool need_restart = false;
// Socket initialize:
	char message[1024], in_buf[1024];
	
	int total;
	int sockfd = 0;
	struct sockaddr_in serv_addr;
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	if(inet_pton(AF_INET, host, &serv_addr.sin_addr)<=0) { error("\ninet_pton error occured.\n"); }
	while(1) {
		fr = 0;
  		for (int t_c=0;t_c<10;t_c++) {	// 10 second
	  		sleep(1);
			testfds = inputs;
			timeout.tv_sec = 0;
			timeout.tv_usec = 1000;
			result = select(FD_SETSIZE, &testfds, (fd_set *)NULL, (fd_set*)NULL, &timeout);
			switch(result) {
				case 0:
					break;
				case -1:
					std::cout << "Input error." << std::endl;
					std::cout.flush();
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
										if (strstr(c,"(NMR)")) {
											need_restart = true;
										}
										std::cout << "Oscam-" << oscam_port << " " << c << std::endl;
										std::cout.flush(); 
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
			}	// switch
		}		// for
// Check reader live:
		if (need_restart == true) {
			std::cout << "Oscam-" << oscam_port << " ----------------------------------" << std::endl;
			std::cout << "Oscam-" << oscam_port << " --- Required restart container ---" << std::endl;
			std::cout << "Oscam-" << oscam_port << " ----------------------------------" << std::endl;
			std::cout.flush();
			// Container restart code:
			system("rm /tmp/healthy");
		}
// Print oscam fr to stdout:
		std::cout << "Oscam-" << oscam_port << " -------- Current frequency = " << fr << std::endl;
		std::cout.flush();
// Write to influx:
		ss.str("");
		ss << "count,port=" << oscam_port << " fr=" << fr;
		str = ss.str();
		ss.str("");
		ss << "POST /write?db=oscam HTTP/1.1\nHost: " << host << "\nContent-Length: " << str.size() << "\n\n" << str;
		str = ss.str();
		std::strcpy (message, str.c_str());
		influx_is_ready_old_status = influx_is_ready;
		if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			influx_is_ready = false;
		} else if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			influx_is_ready = false;
		} else {
			influx_is_ready = true;
		}
		if (influx_is_ready) {
			write(sockfd,message,(int)strlen(message));
			close(sockfd);
			if (!influx_is_ready_old_status) {
				std::cout << "Oscam-" << oscam_port << "------------------------------------" << std::endl;
				std::cout << "Oscam-" << oscam_port << "--- Success: connection restore. ---" << std::endl;
				std::cout << "Oscam-" << oscam_port << "------------------------------------" << std::endl;
				std::cout.flush();
			}
		} else {
			if (influx_is_ready_old_status) {
				std::cout << "Oscam-" << oscam_port << "-------------------------------" << std::endl;
				std::cout << "Oscam-" << oscam_port << "--- Error: connection loss. ---" << std::endl;
				std::cout << "Oscam-" << oscam_port << "-------------------------------" << std::endl;
				std::cout.flush();
			}
		}
	}	// while
}
