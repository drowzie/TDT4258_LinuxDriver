#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

int test = 0;
static unsigned int buttonval = 0;
static int btn_driver_fd;

void btn_handler(int sig){
	if(sig == SIGIO){
		read(btn_driver_fd, &buttonval, 1);
		test = 1;
	}
}

int main(int argc, char *argv[])
{
	//Dette kan flyttes over i en anne funksjon :)
	signal(SIGIO, btn_handler);
	btn_driver_fd = open("/dev/btn_gamepad", O_RDWR);
	
	if(btn_driver_fd < 0){
		printf("Error, Can't Open Device");
		exit(EXIT_FAILURE);
	}
	if(fcntl(btn_driver_fd, F_SETOWN, getpid()) < 0){
		printf("Error, Can't set owner ship of Device");
		exit(EXIT_FAILURE);
	}
	int flag = fcntl(btn_driver_fd, F_GETFL);
	if (flag < 0){
		printf("Error, Can't get flags from Device");
		exit(EXIT_FAILURE);
	}
	if(fcntl(btn_driver_fd, F_SETFL, flag | FASYNC) < 0){
		printf("Error, Can't set flags from Device");
		exit(EXIT_FAILURE);
	}
	//Altså alt over her
	printf("All is up!\n");
	while(1){
		if(test){
		printf("Interrupt has been triggered and button value is: %#X\n", buttonval);
		test = 0;
		}	
		sleep(1);
	}
	close(btn_driver_fd);
	printf("Hello World, I'm game!\n");
	exit(EXIT_SUCCESS);
}
