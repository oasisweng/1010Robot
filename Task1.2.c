#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "timeControl.h"

void drawSquare();
void drawTriangle();
void drawCircle();
void drawStar();


int main()
{
    m_sock = setupConnection();

    clock_t timer = clock(), superfastLoopTimer, fastLoopTimer, mediumLoopTimer, slowLoopTimer;
    superfastLoopTimer = fastLoopTimer = mediumLoopTimer = slowLoopTimer = clock() *1000 / CLOCKS_PER_SEC;

    bool isMissionAccomplished = true;
    createMissionList();
    while (1)
    {
        timer = clock() *1000 / CLOCKS_PER_SEC;
        if(timer-superfastLoopTimer >= 1) //100Hz Loop
        {
            m_timeUsed++;
            printTrail();

            if(!isMissionAccomplished)
            {
                isMissionAccomplished = checkMissionStatus();
                //printf("T %d\n",m_timeUsed);
            }

            superfastLoopTimer = timer;
        }
        if(timer-fastLoopTimer >= 100) //50Hz Loop
        {
            if(isMissionAccomplished && m_missionCurrent<m_missionTotal)
            {
                m_missionCurrent++;
                m_timeCost = m_missionList[m_missionCurrent].timeCost;
            #ifdef DEBUG
                printf("NewM T: %d\n",m_timeCost);
            #endif
                m_speed[0] = m_missionList[m_missionCurrent].speedL;
                m_speed[1] = m_missionList[m_missionCurrent].speedR;
                m_timeUsed = 0;
                updateMotor();
                isMissionAccomplished = false;
            }

            fastLoopTimer = timer;
        }
        else if(timer-slowLoopTimer >= 200) //5Hz loop
        {
            updateMotor();
            slowLoopTimer = timer;
        }
    }
}

void createMissionList()
{
    //drawSquare();
    //drawTriangle();
    //drawCircle();
    drawStar();
}

void drawStar()
{
    for(int i=0;i<5;i++)
    {
        goStraight(60,50);
        turnRight(117,10,100);
    }
}

void drawCircle()
{
    turnLeft(300,70,500);
}

void drawTriangle()
{
    for(int i=0;i<3;i++)
    {
        goStraight(60,50);
        turnRight(97,10,100);
    }
}

void drawSquare()
{
    for(int i=0;i<4;i++)
    {
        goStraight(60,50);
        turnRight(73,10,100);
    }
}

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

void setMotorSpeed(int leftSpeed, int rightSpeed)
{
    m_speed[0] = leftSpeed;
    m_speed[1] = rightSpeed;
    updateMotor();
}

void updateMotor()
{
    char buffer[80];
    sprintf(buffer, "M LR %d %d\n", m_speed[0], m_speed[1]);
    printf(buffer);
    sendCommend(buffer);
    readFeedback(buffer);
}

void stopRobot()
{
    m_speed[0] = m_speed[1] = 0;
    updateMotor();
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

void goStraight(int timeCost, unsigned int speed)
{
    m_missionTotal++;
    Mission newMission;
    newMission.timeCost = timeCost;
    newMission.speedL = newMission.speedR = speed;
    m_missionList[m_missionTotal] = newMission;
}

void turnRight(int timeCost, unsigned int speed, double radius)
{
    m_missionTotal++;
    Mission newMission;

    if(radius < ROBOT_RADIUS)
        radius = ROBOT_RADIUS;

    double radius2 = radius-2*ROBOT_RADIUS;

    newMission.timeCost = timeCost;

    newMission.speedL = speed;
    newMission.speedR = speed*(radius2/radius);

    m_missionList[m_missionTotal] = newMission;
}

void turnLeft(int timeCost, unsigned int speed, double radius)
{
    m_missionTotal++;
    Mission newMission;

    if(radius < ROBOT_RADIUS)
        radius = ROBOT_RADIUS;

    double radius2 = radius-2*ROBOT_RADIUS;

    newMission.timeCost = timeCost;

    newMission.speedR = speed;
    newMission.speedL = speed*(radius2/radius);

    m_missionList[m_missionTotal] = newMission;
}

bool checkMissionStatus()
{
    if(abs(m_timeCost - m_timeUsed)<= ERROR_MAX)
    {
        stopRobot();
    #ifdef DEBUG
        printf("M%d Done T: %d, D: %d\n",m_missionCurrent,m_timeUsed,m_timeCost);
    #endif
        return true;
    }

    return false;
}
