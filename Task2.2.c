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

/*
 MAIN
 */

#define sensor_queue_length 963
int sensor_vals[sensor_queue_length] = {0}; /* Cyclic sensor reading array: 0 is Front Left IR, 1 is Front Right IR,
															2 is Side Left IR, 3 is Side Right IR, 4 is Left Encoder, 5 is Right Encoder
															6 is Ultra Sonic, 7 is Bumper Front Left, 8 is Bumper Front Right */
int current_sensor_pointer = 0;// Currently, this value serves both pointer for new sensor value input and sensor value being processed next.
const int WALL_THRESHOLD = 30;
const int SIDE_THRESHOLD = 10;
const int US_THRESHOLD = 30;
const int DEFAULT_SPEED = 20;
const int ACCELARATED_SPEED = 23;
const int SLOWED_SPEED = 17;

bool leftWillSweep = true; //bool value indicating if left servo should move
bool rightWillSweep = true; //bool value indicating if right servo should move
int current_left_degree = 45; //int value indicating position of left servo
int current_right_degree = 90; //int value indicating position of right servo

#define TIMER1_FREQ 90
#define TIMER2_FREQ 30
#define TIMER3_FREQ 20
#define TIMER4_FREQ 1

typedef enum{
	SEARCH_WALL = 0,
	WALL_FOLLOW_LEFT = 1,
	WALL_FOLLOW_RIGHT = 2
	} STATE_t;

STATE_t cur_state = SEARCH_WALL;

void updateSensorValues();
void reportSensorValues();
void sweepIR(int freq);
void getCurrentPosition(int encoder[2]);
int main() {
	m_sock = connect_to_robot();
	initialize_robot();
	usleep(1000);
	clock_t timer = clock(), timer1,timer2,timer3,timer4;
	timer1=timer2=timer3=timer4 = clock() *1000 / CLOCKS_PER_SEC;
	while (1)
	{
		sweepIR(TIMER1_FREQ); //Sweep IR sensors in pace with sensor update
		timer = clock() *1000 / CLOCKS_PER_SEC; //Value in millisec
		//printf("%i",abs(timer-timer1));
		
		if (abs(timer-timer4)>=TIMER4_FREQ){
			updateSensorValues();
			reportSensorValues();
			switch (cur_state) {
				case SEARCH_WALL:{
					int left_ir = sensor_vals[current_sensor_pointer];
					int right_ir = sensor_vals[current_sensor_pointer+1];
					printf("left:%i,%i",current_left_degree,left_ir);
					printf("right:%i,%i",current_right_degree,right_ir);
					if (current_left_degree<=45&&current_left_degree>=-60)
						if (left_ir<=WALL_THRESHOLD){
							leftWillSweep = false;
							cur_state = WALL_FOLLOW_LEFT;
						}
					if (current_right_degree>=-45&&current_right_degree<=60)
						if (right_ir<=WALL_THRESHOLD){
							rightWillSweep = false;
							cur_state = WALL_FOLLOW_RIGHT;
						}
					set_motors(DEFAULT_SPEED,DEFAULT_SPEED);
					printf("S\n");
				}
					break;
				case WALL_FOLLOW_LEFT:{
					//if side sensor is in range, following with side sensor
					int side_val = sensor_vals[current_sensor_pointer+2];
					int front_val = sensor_vals[current_sensor_pointer];
					int us_val = sensor_vals[current_sensor_pointer+6];
					if (us_val<US_THRESHOLD)
						set_motors(ACCELARATED_SPEED,-ACCELARATED_SPEED);
					else if (side_val==40)
						set_motors(0,ACCELARATED_SPEED);
					else if (front_val<WALL_THRESHOLD||side_val<SIDE_THRESHOLD){
						printf(". %i %i",front_val,side_val);
							set_motors(ACCELARATED_SPEED,DEFAULT_SPEED);
					}else if (front_val == WALL_THRESHOLD || side_val==SIDE_THRESHOLD)
						set_motors(DEFAULT_SPEED,DEFAULT_SPEED);
					else if (front_val>WALL_THRESHOLD||(side_val<40&&side_val>SIDE_THRESHOLD)){
							set_motors(SLOWED_SPEED,ACCELARATED_SPEED);
					} else
						cur_state = SEARCH_WALL;
					printf("L\n");
				}
					break;
				case WALL_FOLLOW_RIGHT:{
					//if side sensor is in range, following with side sensor
					int side_val = sensor_vals[current_sensor_pointer+3];
					int front_val = sensor_vals[current_sensor_pointer+1];
					int us_val = sensor_vals[current_sensor_pointer+6];
					if (us_val<US_THRESHOLD)
						set_motors(-ACCELARATED_SPEED,ACCELARATED_SPEED);
					else if (side_val==40)
						set_motors(ACCELARATED_SPEED,0);
					else if (front_val<WALL_THRESHOLD||side_val<SIDE_THRESHOLD)
							set_motors(DEFAULT_SPEED,ACCELARATED_SPEED);
					else if (front_val == WALL_THRESHOLD||side_val==SIDE_THRESHOLD)
						set_motors(DEFAULT_SPEED,DEFAULT_SPEED);
					else if (front_val>WALL_THRESHOLD&&side_val<40){
							set_motors(ACCELARATED_SPEED,SLOWED_SPEED);
					} else
						cur_state = SEARCH_WALL;
					printf("R\n");
				}
					break;
					
				default:
					break;
			}
			current_sensor_pointer += 9;
			current_sensor_pointer %= sensor_queue_length;
			//printf("c_s_p: %i\n",current_sensor_pointer);
			timer4 = timer;
		}
		
	}

	return 0;
}

/*
 AVAIABLE ACTIONS/COMMANDS
 */
void updateSensorValues() {
	int flir;
	int frir;
	int slir;
	int srir;
	int lbump;
	int rbump;
	int lenc;
	int renc;
	int us;
	int error;
	error = get_front_ir_dists(&flir,&frir);
	if (error<0)
		printf("FRONTIR: An error occurred, error code = %i\n",error);
	error = get_side_ir_dists(&slir,&srir);
	if (error<0)
		printf("SIDEIR: An error occurred, error code = %i\n",error);
	error = check_bumpers(&lbump,&rbump);
	if (error<0)
		printf("BUMP: An error occurred, error code = %i\n",error);
	error = get_motor_encoders(&lenc,&renc);
	if (error<0)
		printf("ENC:An error occurred, error code = %i\n",error);
	us = get_us_dist();
	if (us==500)
		printf("US:An error occurred, error code = %i\n",error);
	sensor_vals[current_sensor_pointer] = flir;
	sensor_vals[current_sensor_pointer+1] = frir;
	sensor_vals[current_sensor_pointer+2] = slir;
	sensor_vals[current_sensor_pointer+3] = srir;
	sensor_vals[current_sensor_pointer+4] = lenc;
	sensor_vals[current_sensor_pointer+5] = renc;
	sensor_vals[current_sensor_pointer+6] = us;
	sensor_vals[current_sensor_pointer+7] = lbump;
	sensor_vals[current_sensor_pointer+8] = rbump;
}

void reportSensorValues(){
	int i = current_sensor_pointer;
	printf("Frt Lft IR: %i Frt Rt IR: %i Sd Lft IR: %i Sd Rt IR: %i\n",
			 sensor_vals[i],sensor_vals[i+1],sensor_vals[i+2],sensor_vals[i+3]);
	printf("US: %i Lft Enc: %i Rt Enc: %i\n",sensor_vals[i+6],sensor_vals[i+4],sensor_vals[i+5]);
	//printf("Lft Bump: %i Rt Bump: %i\n",sensor_vals[i+7],sensor_vals[i+8]);
}

bool l_sweep_back = true;
bool r_sweep_back = true;
void sweepIR(int freq){
	double degree_per_turn = 180 / freq;
	if (leftWillSweep){
		if (l_sweep_back)
			current_left_degree -= degree_per_turn;
		else
			current_left_degree += degree_per_turn;
		if (current_left_degree >= 45)
			l_sweep_back = true;
		if (current_left_degree <= -90)
			l_sweep_back = false;
		set_ir_angle(LEFT,current_left_degree);
	}
		
	if (rightWillSweep){
		if (r_sweep_back)
			current_right_degree -= degree_per_turn;
		else
			current_right_degree += degree_per_turn;
		if (current_right_degree >= 90)
			r_sweep_back = true;
		if (current_right_degree <= -45)
			r_sweep_back = false;
		set_ir_angle(RIGHT,current_right_degree);
	}
}

/*
 HELPER
 */

int setupConnection()
{
	struct sockaddr_in s_addr;
	int sock;
	
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		fprintf(stderr, "Failed to create socket\n");
		exit(1);
	}
	
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	s_addr.sin_port = htons(55443);
	
	if (connect(sock, (struct sockaddr *) &s_addr, sizeof(s_addr)) < 0)
	{
		fprintf(stderr, "Failed to connect socket\n");
		exit(1);
	}
	
	return sock;
}

void checkConnection()
{
	int timeOut = 0;
	int position[2] = {-1,-1};
	while(position[0] == -1 || position[1] == -1)
	{
      getCurrentPosition(position);
		usleep(100000);
		timeOut++;
		if(timeOut>=10)
		{
			fprintf(stderr, "No command received from the robot!\n");
			exit(1);
		}
	}
}

/*
 Get current encoder value
 */
void getCurrentPosition(int encoder[2])
{
	char buffer[80];
	sprintf(buffer, "S MELR\n");
	sendCommend(buffer);
	readFeedback(buffer);
	sscanf(buffer,"S MELR %d %d",&encoder[0],&encoder[1]);
}


void printTrail()
{
	char buffer[80];
	sprintf(buffer, "C TRAIL\n");
	sendCommend(buffer);
	readFeedback(buffer);
}

void sendCommend(char* buffer)
{
	write(m_sock, buffer, strlen(buffer));
}

void readFeedback(char* buffer)
{
	memset(buffer, 0, strlen(buffer));
	read(m_sock, buffer, 80);
	//printf(feedBack);
}
