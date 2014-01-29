#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "picomms.h"
#include "roboTemp2.h"

/*
 MAIN
 */

const int sensor_queue_length = 960;
int sensor_vals[960] = {0}; /* Cyclic sensor reading array: 0 is Front Left IR, 1 is Front Right IR,
															2 is Side Left IR, 3 is Side Right IR, 4 is Left Encoder, 5 is Right Encoder
															6 is Ultra Sonic, 7 is Bumper Front Left, 8 is Bumper Front Right */
int current_sensor_pointer = 0;// Currently, this value serves both pointer for new sensor value input and sensor value being processed next.
const int WALL_THRESHOLD = 35;
const int DEFAULT_SPEED = 20;
const int ACCELARATED_SPEED = 21;
const int SLOWED_SPEED = 19;

bool leftWillSweep = true; //bool value indicating if left servo should move
bool rightWillSweep = true; //bool value indicating if right servo should move
int current_left_degree = 0; //int value indicating position of left servo
int current_right_degree = 0; //int value indicating position of right servo

#define TIMER1_FREQ 40;
#define TIMER2_FREQ 30;
#define TIMER3_FREQ 20;
#define TIMER4_FREQ 10;

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
int main()
{
	m_sock = connect_to_robot();
    initialize_robot();
    clock_t timer = clock(), timer1,timer2,timer3,timer4;
	timer1=timer2=timer3=timer4 = clock() *1000 / CLOCKS_PER_SEC;
	while (1)
	{
		timer = clock() *1000 / CLOCKS_PER_SEC; //Value in millisec
        set_motors(DEFAULT_SPEED,DEFAULT_SPEED);
		sweepIR(TIMER1_FREQ); //Sweep IR sensors in pace with sensor update
        
		if(abs(timer-timer1) >= TIMER1_FREQ) //25HZ, this will update sensor value (GP2D12 refreshes every 38ms, SRF08 refreshes at most 70ms)
		{
			updateSensorValues();
			reportSensorValues();
			timer1 = timer;
			
			//currently, it will also adjust robot's speed
			switch (cur_state) {
				case SEARCH_WALL:{
					if (sensor_vals[current_sensor_pointer]<=WALL_THRESHOLD){
						leftWillSweep = false;
						cur_state = WALL_FOLLOW_LEFT;
					}
					if (sensor_vals[current_sensor_pointer+1]<=WALL_THRESHOLD){
						rightWillSweep = false;
						cur_state = WALL_FOLLOW_RIGHT;
					}
                    break;
				}
				case WALL_FOLLOW_LEFT:{
					if (sensor_vals[current_sensor_pointer]<WALL_THRESHOLD){
						set_motor(0,ACCELARATED_SPEED);
					} else if (sensor_vals[current_sensor_pointer] == WALL_THRESHOLD){
						set_motor(0,DEFAULT_SPEED);
					} else if (sensor_vals[current_sensor_pointer]>WALL_THRESHOLD){
						if (sensor_vals[current_sensor_pointer]>WALL_THRESHOLD*2){
							set_motor(0,SLOWED_SPEED/2);
						} else
							set_motor(0,SLOWED_SPEED);
					}
					break;
				}
				case WALL_FOLLOW_RIGHT:{
					if (sensor_vals[current_sensor_pointer+1]<WALL_THRESHOLD){
						set_motor(1,ACCELARATED_SPEED);
					} else if (sensor_vals[current_sensor_pointer+1] == WALL_THRESHOLD){
						set_motor(1,DEFAULT_SPEED);
					} else if (sensor_vals[current_sensor_pointer+1]>WALL_THRESHOLD){
						if (sensor_vals[current_sensor_pointer+1]>WALL_THRESHOLD*2){
							set_motor(1,SLOWED_SPEED/2);
						} else
							set_motor(1,SLOWED_SPEED);
					}
					break;
				}
                    
				default:
					break;
			}
		}
        current_sensor_pointer += 9;

	}

	return 0;
}

/*
 AVAIABLE ACTIONS/COMMANDS
 */
void updateSensorValues()
{
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
		printf("An error occurred, error code = %i",error);
	error = get_side_ir_dists(&slir,&srir);
	if (error<0)
		printf("An error occurred, error code = %i",error);
	error = check_bumpers(&lbump,&rbump);
	if (error<0)
		printf("An error occurred, error code = %i",error);
	error = get_motor_encoders(&lenc,&renc);
	if (error==0)
		printf("An error occurred, error code = %i",error);
	us = get_us_dist();
	if (us==500)
		printf("An error occurred, error code = %i",error);
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

void reportSensorValues()
{
	int i = current_sensor_pointer;
	printf("Frt Lft IR: %i Frt Rt IR: %i Sd Lft IR: %i Sd Rt IR: %i",
			 sensor_vals[i],sensor_vals[i+1],sensor_vals[i+2],sensor_vals[i+3]);
	printf("US: %i Lft Enc: %i Rt Enc: %i",sensor_vals[i+6],sensor_vals[i+4],sensor_vals[i+5]);
	printf("Lft Bump: %i Rt Bump: %i",sensor_vals[i+7],sensor_vals[i+8]);
}

void sweepIR(int freq){
	double degree_per_turn = 180 / freq;
	if (leftWillSweep){
		current_left_degree += degree_per_turn;
		current_left_degree = current_left_degree % 180;
		set_ir_angle(LEFT,current_left_degree-90);
	}
		
	if (rightWillSweep){
		current_right_degree += degree_per_turn;
		current_right_degree = current_right_degree % 180;
		set_ir_angle(RIGHT,current_right_degree-90);
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