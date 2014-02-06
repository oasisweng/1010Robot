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


const double D_SPEED = 20.0;
double l_speed = D_SPEED;
double r_speed = D_SPEED;
double SPEED_ERR = 1; //Distance can err by one per reading
int lst_lenc = 0;
int lst_renc = 0;
int cur_lenc = 0;
int cur_renc = 0;
int diff = 0; //The difference between left and right encoder
int dis_l = 0; //Current left distance
int dis_r = 0;
int max_dis_l_nor = -1000;
int max_dis_r_nor = -1000;
int max_dis_l_err = -1000;
int max_dis_r_err = -1000;

void set_dis_control(){
	//if left wheel is off(using right encoder reading as reference)
	if (dis_r>max_dis_r_nor)
	   max_dis_r_nor = dis_r;
	if (dis_l>max_dis_l_err && (dis_l>max_dis_r_nor+SPEED_ERR||dis_l<max_dis_r_nor-SPEED_ERR))
		max_dis_l_err = dis_l;
	//if right wheel is off(using left encoder reading as reference)
	if (dis_l>max_dis_l_nor)
	   max_dis_l_nor = dis_l;
	if (dis_r>max_dis_r_err && (dis_r>max_dis_l_nor+SPEED_ERR||dis_r<max_dis_l_nor-SPEED_ERR))
		max_dis_r_err = dis_r;
	printf("LN:%i LE:%i RN:%i RE:%i ",max_dis_l_nor,max_dis_l_err,max_dis_r_nor,max_dis_r_err);
}

void reset_dis_control(){
	max_dis_l_err = -1000;
	max_dis_r_err = -1000;
	max_dis_l_nor = -1000;
	max_dis_r_nor = -1000;
}

void updateData(){
	get_motor_encoders(&cur_lenc,&cur_renc);
	dis_l = cur_lenc-lst_lenc;
	dis_r = cur_renc-lst_renc;
	lst_lenc = cur_lenc;
	lst_renc = cur_renc;
	diff += dis_l-dis_r;
	set_dis_control();
}

int main() {
	sock = connect_to_robot();
	usleep(1000);
	set_asr(0);
	while (1)
	{
		updateData();
		if (diff>0){//If right wheel is off
			if (max_dis_r_err!=-1000){
				r_speed = D_SPEED*(1+(double)(max_dis_l_nor-max_dis_r_err)/max_dis_l_nor);
				max_dis_r_err = -1000;
			}
			if (r_speed > 127.0) {
				r_speed = 127.0;
			}
		} else if (diff<0){//If left wheel is off
			if (max_dis_r_err != 1000){
				r_speed = D_SPEED*(double)(max_dis_r_err-max_dis_l_nor)/max_dis_r_err;
				max_dis_r_err = -1000;
			}
			if (r_speed < 0)
				r_speed = 0;
		} else {
			r_speed = D_SPEED;
			l_speed = D_SPEED;
			reset_dis_control();
		}
		
		printf("D:%i L:%i R:%i LS:%g RS:%g\n",diff,dis_l,dis_r,l_speed,r_speed);
		set_motors(l_speed,r_speed);
		log_trail();
		
	}
	
	return 0;
}
