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

int main(int argc, char **argv) {	// argv[1] - APP_PORT, argv[2] - MIN_KEY_NUMBER, argv[3] - INFLUX_ADDRESS,
					// argv[4] - INFLUX_PORT, argv[5] - live_delay, argv[6] - restart_delay

// Init:
// Oscam port:
	char oscam_port[64];
	strcpy(oscam_port,*(argv+1));
// MIN_KEY_NUMBER:
	int min_key_number = atoi(*(argv+2));
// INFLUX DATABASE ADDRESS:
	char host[128];
	strcpy(host,*(argv+3));
// INFLUX DATABASE PORT:
	int port = atoi(*(argv+4));
// LIVE DELAY - delay before start monitoring live:
	int live_delay = atoi(*(argv+5));
	int restart_delay = atoi(*(argv+6));
	int restart_delay_count = restart_delay;

        std::stringstream ss;
	std::string str;

	char buffer[2048];
	int result, nread;
	bool influx_is_ready=true, influx_is_ready_old_status, zero_count=false;	// Influx database status, zero_count=true - 2 zero count
	fd_set inputs, testfds;
	struct timeval timeout;
	FD_ZERO(&inputs);
	FD_SET(0, &inputs);
	int fr;		// Freq card request for 10s
// Socker initialize:
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
// Write to influx:
		switch (live_delay) {
			case 0:		// keep alive mode
				if (fr < min_key_number) {
					if (restart_delay_count != 0) {
						restart_delay_count--;
					} else {
						std::cout << "Oscam-" << oscam_port << " ----------------------------------" << std::endl;
						std::cout << "Oscam-" << oscam_port << " --- Required restart container ---" << std::endl;
                                        	std::cout << "Oscam-" << oscam_port << " ----------------------------------" << std::endl;
						std::cout.flush();
						// Container restart code:
						system("rm /tmp/healthy");
					}
				} else {
					restart_delay_count = restart_delay;
				}
				std::cout << "Oscam-" << oscam_port << " -------- Current frequency = " << fr << std::endl;
				std::cout.flush();
				break;
			case 1:		// Run keep alive mode
				live_delay = 0;
				std::cout << "Oscam-" << oscam_port << " ----------------------------" << std::endl;
				std::cout << "Oscam-" << oscam_port << " --- Run keep alive mode. ---" << std::endl;
				std::cout << "Oscam-" << oscam_port << " ----------------------------" << std::endl;
				std::cout.flush();
				break;
			default:			// keep alive delay
				live_delay--;
				break;
		}
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
		if (influx_is_ready && (live_delay == 0)) {
			write(sockfd,message,(int)strlen(message));
			close(sockfd);
			if (!influx_is_ready_old_status) {
				std::cout << "Oscam-" << oscam_port << "------------------------------------" << std::endl;
				std::cout << "Oscam-" << oscam_port << "--- Success: connection restore. ---" << std::endl;
				std::cout << "Oscam-" << oscam_port << "------------------------------------" << std::endl;
				std::cout.flush();
			}
		} else {
			if (influx_is_ready_old_status && (live_delay == 0)) {
				std::cout << "Oscam-" << oscam_port << "-------------------------------" << std::endl;
				std::cout << "Oscam-" << oscam_port << "--- Error: connection loss. ---" << std::endl;
				std::cout << "Oscam-" << oscam_port << "-------------------------------" << std::endl;
				std::cout.flush();
			}
		}
	}	// while
}
