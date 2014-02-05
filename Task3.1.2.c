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

const int D_SPEED = 20;
const double STEERING_GAIN = 2.0;
const int ASR_T = 1; //Short for ASR Threshold
int l_speed = D_SPEED;
int r_speed = D_SPEED;
int last_lenc=0;
int last_renc=0;
int cur_lenc=0;
int cur_renc=0;
int cur_dlt_l=0; // The current distance left wheel travels
int cur_dlt_r=0;
int ini_dlt_l=0;// Initial distance left wheel traveled on record
int ini_dlt_r=0;
int diff_l = 0; // The difference between current distance and last distance
int diff_r = 0;
bool leftOn = false; // Is left wheel on rough surface?
bool rightOn = false;

void updateData(){
	get_motor_encoders(&cur_lenc,&cur_renc);
	cur_dlt_l = abs(cur_lenc-last_lenc);
	cur_dlt_r = abs(cur_renc-last_renc);
	if (ini_dlt_l <=D_SPEED/5)
		ini_dlt_l = cur_dlt_l;
	if (ini_dlt_r <=D_SPEED/5)
		ini_dlt_r = cur_dlt_r;
	diff_l = ini_dlt_r-cur_dlt_l;
	diff_r = ini_dlt_l-cur_dlt_r;
	last_lenc = cur_lenc;
	last_renc = cur_renc;
}

int main() {
	sock = connect_to_robot();
	
	usleep(1000);
	get_motor_encoders(&last_lenc,&last_renc);
	
	set_asr(0);
	while (1)
	{
		updateData();
		
		printf("%i %i DL:%i DR:%i\n",cur_dlt_l,cur_dlt_r,diff_l,diff_r);
		//If left wheel needs speed adjustment
		if (abs(diff_l)>=ASR_T)
			leftOn = true;
		else
			leftOn = false;
		//If right wheel needs speed adjustment
		if (abs(diff_r)>=ASR_T)
			rightOn = true;
		else
			rightOn = false;
		
		if (!leftOn&&!rightOn){
			set_motors(l_speed,r_speed);
		} else {
			if (leftOn){
				l_speed += D_SPEED*STEERING_GAIN;
				printf("L S:%i ",l_speed);
				set_motors(l_speed,r_speed);
			}
			
			if (rightOn){
				r_speed += D_SPEED*STEERING_GAIN;
				printf("R S:%i ",r_speed);
				set_motors(l_speed,r_speed);
			}
		}
		
		log_trail();
		
	}
	
	return 0;
}
