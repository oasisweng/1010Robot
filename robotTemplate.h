#ifndef __ROBOT_TEMPLATE_H__
#define __ROBOT_TEMPLATE_H__

typedef int bool;
#define true 1
#define false 0

#define ERROR_MAX 0

#define COUNTS_PER_DEGREE 2.347 //845/360
#define ROBOT_RADIUS 135.5 //845/2Pi ~~ 134.915, but 135.5 gives best performance on simulator
#define Pi 3.145926

#define DEBUG

typedef struct Mission
{
	int distanceL;
	int distanceR;
	int speedL;
	int speedR;
}Mission;

/// List of mission, set size to 30 temporarily
Mission m_missionList[30];
/// Total mission number, and current mission index
int m_missionTotal=0,m_missionCurrent=0;
/// Socket for communication
int m_sock;
/// Global Speed for Motors, shoule be set privately
int m_speed[2];
///
int m_currentPosition[2] = {-1,-1};
///
int m_destPosition[2];
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
/// Read left and right motor encoder counts.
/// return: [left, right]
void getCurrentPosition(int encoder[2]);
/// Send Command to robot
void sendCommend(char* buffer);
/// Read robot feedback
void readFeedback(char* buffer);
///
void goStraight(int distance, unsigned int speed);
///
void turnLeft(int degree, unsigned int speed, double radius);
///
void turnRight(int degree, unsigned int speed, double radius);
///
void createMissionList();
///
bool checkMissionStatus();
///
void decelerateRobot(int diff[2]);
#endif
