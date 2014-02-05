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


//when robot gets on carpet, its right encoder reading reduces


const int D_SPEED = 20;
const double STEERING_GAIN = 20;
int l_speed = D_SPEED;
int r_speed = D_SPEED;
int i = 0;
int lst_lenc = 0;
int lst_renc = 0;
int cur_lenc = 0;
int cur_renc = 0;
int diff = 0; //The difference between left and right encoder
int dis_l = 0;
int dis_r = 0;
double speed = 0;

void updateData(){
	get_motor_encoders(&cur_lenc,&cur_renc);
	dis_l = cur_lenc-lst_lenc;
	dis_r = cur_renc-lst_renc;
	lst_lenc = cur_lenc;
	lst_renc = cur_renc;
	diff = dis_l-dis_r;
	
}

int main() {
	sock = connect_to_robot();
	usleep(1000);
	get_motor_encoders(&lst_lenc,&lst_renc);
	usleep(10000);
	set_asr(0);
	while (1)
	{
		updateData();
		if (diff>0){
			if (diff == dis_l)
				speed = D_SPEED+ 20;
			else
				speed =D_SPEED/(1-diff/dis_l);
			printf("S:%f ",speed);
		   r_speed = speed;
			l_speed = D_SPEED;
			if (r_speed >= 127) {
				r_speed = 127;
			}
		} else if (diff<0){
			if (diff == dis_r)
				speed = D_SPEED+ 20;
			else
				speed =D_SPEED/(1-diff/dis_r);
 
			r_speed = D_SPEED;
			l_speed = speed;
			if (l_speed>=127)
				l_speed = 127;
		}
		printf("%i %i %i %i\n",diff,dis_l,dis_r,r_speed);
		set_motors(l_speed,r_speed);
		log_trail();
		
	}
	
	return 0;
}
