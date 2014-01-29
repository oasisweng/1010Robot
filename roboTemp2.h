//
//  roboTemp2.h
//  
//
//  Created by Dingzhong Weng on 1/28/14.
//
//

#ifndef _roboTemp2_h
#define _roboTemp2_h

typedef int bool;
#define true 1
#define false 0

#define ERROR_MAX 0

#define COUNTS_PER_DEGREE 2.347 //845/360
#define ROBOT_RADIUS 135.5 //845/2Pi ~~ 134.915, but 135.5 gives best performance on simulator
#define Pi 3.145926

#define DEBUG

/// Socket for communication
int m_sock;
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

#endif
