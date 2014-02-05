#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <math.h>
#include "picomms.h"
#include "roboTemp2.h"

const int D_SPEED = 100;
const double STEERING_GAIN = 2;
int main() {
	sock = connect_to_robot();
	
	usleep(1000);
	int l_speed = D_SPEED;
	int r_speed = D_SPEED;
	int last_lenc=0;
	int last_renc=0;
	int cur_lenc=0;
	int cur_renc=0;
	int delta_l=0; // The distance left encoder records per reading
	int delta_r=0;
	int diff = 0; // The difference between delta_l and delta_r
	bool leftOn = false; // Is left wheel on rough surface?
	bool rightOn = false;
	//This asr threshold changes if current setting is not suitable
	int ASR_T = D_SPEED/5;
	get_motor_encoders(&last_lenc,&last_renc);

	set_asr(0);
	while (1)
	{
		get_motor_encoders(&cur_lenc,&cur_renc);
		delta_l = abs(cur_lenc-last_lenc);
		delta_r = abs(cur_renc-last_renc);
		diff = abs(delta_l-delta_r);
		last_lenc = cur_lenc;
		last_renc = cur_renc;
		
		printf("%i %i ASR_T: %i\n",delta_l,delta_r,ASR_T);
		if (delta_l<=ASR_T)
			leftOn = true;
		else
			leftOn = false;
		if (delta_r<=ASR_T)
			rightOn = true;
		else
			rightOn = false;
		if (leftOn && rightOn){
			//ASR_T is too high, both wheels are on smoother surface
			ASR_T --;
			set_motors(l_speed,r_speed);
		} else if (!leftOn && !rightOn){
			//ASR_T is too low, both wheels are on rougher surface
			ASR_T ++;
			set_motors(l_speed,r_speed);
		}else if (leftOn){
			l_speed += diff*STEERING_GAIN;
			printf("L D:%i S:%i ",diff,l_speed);
			set_motors(l_speed,r_speed);
		} else if (rightOn){
			r_speed += diff*STEERING_GAIN;
			printf("R D:%i S:%i ",diff,r_speed);
			set_motors(l_speed,r_speed);
		}
		
		log_trail();
		
	}
	
	return 0;
}
