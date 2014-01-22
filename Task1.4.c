#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "robotTemplate.h"

int DEFAULT_STRAIGHT_LINE = 600;
int DEFAULT_CIRCLE_RADIUS = 300;
int DEFAULT_SPEED = 50;
int ACCELARATED_SPEED = 70;
void drawScalableStar(double scale);
void drawScalableCircle(double scale);
void drawScalableTriangle(double scale);
void drawScalableSquare(double scale);

/*
    MAIN
    */

int main()
{
    m_sock = setupConnection();
    checkConnection();

    clock_t timer = clock(), superfastLoopTimer, fastLoopTimer, mediumLoopTimer, slowLoopTimer;
    superfastLoopTimer = fastLoopTimer = mediumLoopTimer = slowLoopTimer = clock() *1000 / CLOCKS_PER_SEC;

    getCurrentPosition(m_currentPosition); // Check encoders
    m_destPosition[0] = m_currentPosition[0];
    m_destPosition[1] = m_currentPosition[1];

#ifdef DEBUG
    printf("Start: L: %d, D: %d\n",m_currentPosition[0],m_destPosition[0]);
    printf("Start: R: %d, D: %d\n",m_currentPosition[1],m_destPosition[1]);
#endif

    bool isMissionAccomplished = true;
    createMissionList();
    while (1)
    {
        timer = clock() *1000 / CLOCKS_PER_SEC;
        if(timer-superfastLoopTimer >= 10) //100Hz Loop, this will check mission progress at run time
        {
            getCurrentPosition(m_currentPosition);
            //printf("L %d R %d\n",m_currentPosition[0],m_currentPosition[1]);
            if(!isMissionAccomplished)
                isMissionAccomplished = checkMissionStatus();


            superfastLoopTimer = timer;
        }
        if(timer-fastLoopTimer >= 20 || isMissionAccomplished) //50Hz Loop, this will pop new mission for execution
        {
            if(isMissionAccomplished && m_missionCurrent<m_missionTotal)
            {
                m_missionCurrent++;
                m_destPosition[0] = m_currentPosition[0] + m_missionList[m_missionCurrent].distanceL;
                m_destPosition[1] = m_currentPosition[1] + m_missionList[m_missionCurrent].distanceR;
            #ifdef DEBUG
                printf("NewM L: %d\n",m_destPosition[0]);
                printf("NewM R: %d\n",m_destPosition[1]);
            #endif
                m_speed[0] = m_missionList[m_missionCurrent].speedL;
                m_speed[1] = m_missionList[m_missionCurrent].speedR;

                updateMotor();
                isMissionAccomplished = false;
            } else if (isMissionAccomplished && m_missionCurrent == m_missionTotal){
                break;
            }

            fastLoopTimer = timer;
        }
        if(timer-mediumLoopTimer >= 50) //20Hz Loop, this will adjust robot speed at runtime before meeting the destination
        {
            int diff[2];
            diff[0] = m_destPosition[0] - m_currentPosition[0];
            diff[1] = m_destPosition[1] - m_currentPosition[1];

            printf("EnL: %d EnR: %d\n", m_currentPosition[0], m_currentPosition[1]);
            printf("DL %d DR %d\n", diff[0], diff[1]);

            if(abs(diff[0])<m_missionList[m_missionCurrent].speedL*2 || abs(diff[1])<m_missionList[m_missionCurrent].speedR*2)
                decelerateRobot(diff);
            else
            {
                if(diff[0]>0 == m_missionList[m_missionCurrent].distanceL>0)
                    m_speed[0] = m_missionList[m_missionCurrent].speedL;
                else
                    m_speed[0] = -m_missionList[m_missionCurrent].speedL;

                if(diff[1]>0 == m_missionList[m_missionCurrent].distanceR>0)
                    m_speed[1] = m_missionList[m_missionCurrent].speedR;
                else
                    m_speed[1] = -m_missionList[m_missionCurrent].speedR;

                updateMotor();
            }
            printTrail();


            mediumLoopTimer = timer;
        }
        if(timer-slowLoopTimer >= 400) //5Hz loop
        {
        #ifdef DEBUG
            if(m_speed[0]!=0)
                printf("L: %d S: %d\n",m_currentPosition[0],m_speed[0]);
            if(m_speed[1]!=0)
                printf("R: %d S: %d\n",m_currentPosition[1],m_speed[1]);
        #endif
            updateMotor();
            slowLoopTimer = timer;
        }
    }
}

/*
    SETUP
    */

void createMissionList()
{
    double scale;
    printf("How would you like to scale (1.0 gives default image): ");
    scanf("%lf",&scale);
    drawScalableStar(scale);
    drawScalableSquare(scale);
    drawScalableTriangle(scale);
    drawScalableCircle(scale);
    goStraight(800,70);

}

/*
    AVAIABLE ACTIONS/COMMANDS
    */

void drawScalableStar(double scale)
{
    printf("Drawing Star ....\n");
    int distance = DEFAULT_STRAIGHT_LINE * scale;
    for(int i=0;i<5;i++)
    {
        goStraight(distance,ACCELARATED_SPEED);
        turnRight(144,DEFAULT_SPEED,0);
    }
}

void drawScalableCircle(double scale)
{
    printf("Drawing Circle ...\n");
    double radius = DEFAULT_CIRCLE_RADIUS * scale;
    turnLeft(360,DEFAULT_SPEED,radius);
}

void drawScalableTriangle(double scale)
{
    printf("Drawing Triangle ...\n");
    int distance = DEFAULT_STRAIGHT_LINE * scale;
    for(int i=0;i<3;i++)
    {
        goStraight(distance,ACCELARATED_SPEED);
        turnRight(120,DEFAULT_SPEED,0);
    }
}

void drawScalableSquare(double scale)
{
    printf("Drawing Square ...\n");
    int distance = DEFAULT_STRAIGHT_LINE * scale;
    for(int i=0;i<4;i++)
    {
        goStraight(distance,ACCELARATED_SPEED);
        turnRight(90,DEFAULT_SPEED,0);
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

void setMotorSpeed(int leftSpeed, int rightSpeed)
{
    m_speed[0] = leftSpeed;
    m_speed[1] = rightSpeed;
}

void updateMotor()
{
    char buffer[80];
    sprintf(buffer, "M LR %d %d\n", m_speed[0], m_speed[1]);
    printf("%s \n",buffer);
    sendCommend(buffer);
    readFeedback(buffer);
}

void stopRobot()
{
    m_speed[0] = m_speed[1] = 0;
    updateMotor();
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

void goStraight(int distance, unsigned int speed)
{
    m_missionTotal++;
    Mission newMission;
    newMission.distanceL = newMission.distanceR = distance;
    if(distance>0)
    {
        newMission.speedL = newMission.speedR = speed;
    }
    else
    {
        newMission.speedL = newMission.speedR = -speed;
    }
    m_missionList[m_missionTotal] = newMission;
}

void turnRight(int degree, unsigned int speed, double radius)
{
    m_missionTotal++;
    Mission newMission;

    if(radius < ROBOT_RADIUS)
        radius = ROBOT_RADIUS;

    double radius2 = radius-2*ROBOT_RADIUS;

    newMission.distanceL = (double)degree/360 * 2 * Pi* radius;
    newMission.distanceR = (double)degree/360 * 2 * Pi* radius2;
    //     newMission.distanceL = 1;
    // newMission.distanceR = -1;

    if(degree > 0) // Go forward
    {
        newMission.speedL = speed;
        newMission.speedR = speed*(radius2/radius);
    }
    else // Go backword
    {
        newMission.speedL = -speed;
        newMission.speedR = -speed*(radius2/radius);
    }

    m_missionList[m_missionTotal] = newMission;
}

void turnLeft(int degree, unsigned int speed, double radius)
{
    m_missionTotal++;
    Mission newMission;

    if(radius < ROBOT_RADIUS)
        radius = ROBOT_RADIUS;

    double radius2 = radius-2*ROBOT_RADIUS;

    newMission.distanceR = (double)degree/360 * 2 * Pi* radius;
    newMission.distanceL = (double)degree/360 * 2 * Pi* radius2;

    if(degree > 0) // Go forward
    {
        newMission.speedR = speed;
        newMission.speedL = speed*(radius2/radius);
    }
    else // Go backword
    {
        newMission.speedR = -speed;
        newMission.speedL = -speed*(radius2/radius);
    }

    m_missionList[m_missionTotal] = newMission;
}

bool checkMissionStatus()
{
    if(abs(m_destPosition[0] - m_currentPosition[0])<= ERROR_MAX && abs(m_destPosition[1] - m_currentPosition[1])<= ERROR_MAX)
    {
        stopRobot();
    #ifdef DEBUG
        printf("M%d Done L: %d, D: %d\n",m_missionCurrent,m_currentPosition[0],m_destPosition[0]);
        printf("M%d Done R: %d, D: %d\n",m_missionCurrent,m_currentPosition[1],m_destPosition[1]);
    #endif
        return true;
    }

    return false;
}

void decelerateRobot(int diff[2])
{
    double ratio;

    if(m_speed[0]!=0)
    {
        ratio = m_speed[1]/m_speed[0];
        if(diff[0]>diff[1])
        {
            m_speed[1] = diff[1]/5 + 1;
            if(diff[1]<0)
                m_speed[1] -= 2;
            m_speed[0] = m_speed[1] / ratio;
        }
        else
        {
            m_speed[0] = diff[0]/5;
            if(diff[0]<0)
                m_speed[0] -= 2;
            m_speed[1] = m_speed[0] * ratio;
        }

    }
    else if(m_speed[1]!=0)
    {
        ratio = m_speed[0]/m_speed[1];

        if(diff[0]>diff[1])
        {
            m_speed[1] = diff[1]/5+1;
            m_speed[0] = m_speed[1] * ratio;
        }
        else
        {
            m_speed[0] = diff[0]/5;
            m_speed[1] = m_speed[0] / ratio;
        }
    }

    //avoid m_speed (0,0) when diff != (0,0)
    if (m_speed[0] == 0){
        if (diff[0]>0){
            m_speed[0] = 1;
        } else if (diff[0]<0){
            m_speed[0] = -1;
        }
    }
    if (m_speed[1] == 0){
        if (diff[1]>0){
            m_speed[1] = 1;
        } else if (diff[1]<0){
            m_speed[1] = -1;
        }
    }
    updateMotor();
}
