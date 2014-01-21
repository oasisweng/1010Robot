#ifndef __TIME_CONTROL_H__
#define __TIME_CONTROL_H__

typedef int bool;
#define true 1
#define false 0

#define ERROR_MAX 0

#define COUNTS_PER_DEGREE 2.347 //845/360
#define ROBOT_RADIUS 134.915 //845/2Pi
#define Pi 3.145926

#define DEBUG

typedef struct Mission
{
	int timeCost;
	int speedL;
	int speedR;
}Mission;

int m_timeUsed = 0;
int m_timeCost = 0;
/// List of mission, set size to 20 temporarily
Mission m_missionList[20];
/// Total mission number, and current mission index
int m_missionTotal=0,m_missionCurrent=0;
/// Socket for communication
int m_sock;
/// Global Speed for Motors, shoule be set privately
int m_speed[2];
/// Esteblish Connection
/// return: socket
int setupConnection();
/// Check whether connection is esteblished with checking encoder feedback
void checkConnection();
/// Set Motor Speed Globally
void setMotorSpeed(int leftSpeed, int rightSpeed);
/// Update Motor as motor will going to sleep without receiving command for setting speed
void updateMotor();
/// Stops robot instantly
void stopRobot();
/// Draw a dot at present position
void printTrail();
/// Send Command to robot
void sendCommend(char* buffer);
/// Read robot feedback
void readFeedback(char* buffer);
///
void goStraight(int timeCost, unsigned int speed);
///
void turnLeft(int timeCost, unsigned int speed, double radius);
///
void turnRight(int timeCost, unsigned int speed, double radius);
///
void createMissionList();
///
bool checkMissionStatus();
#endif
