#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

typedef int bool;
#define true 1
#define false 0

#define errorMax 1

int initConnection();
void checkConnection();
char* sendCommend(char*);
void goStraight(int, int);
void rotate(int, int);
void turn(int, int);
void readEndoder(int[2]);
void updateMotorSpeed();
void stopRobot();
void drawTrack();
void setCommands();

int m_sock;

int destPosition[2] = {-1, -1};
int currentPosition[2] = {-1, -1};
int m_speed[2];

int positionQueue[20][4];

int commandNumber=0,commandIndex=0;

int main()
{
    m_sock = initConnection();
    checkConnection();

    clock_t timer = clock(), superfastLoopTimer, fastLoopTimer, slowLoopTimer;
    superfastLoopTimer = fastLoopTimer = slowLoopTimer = clock() * 1000 / CLOCKS_PER_SEC;

    destPosition[0] = currentPosition[0];
    destPosition[1] = currentPosition[1];
    printf("Start: L: %d, D: %d\n",currentPosition[0],destPosition[0]);
    printf("R: %d, D: %d\n",currentPosition[1],destPosition[1]);
    drawTrack();
    setCommands();
    bool isMissionFinished = false;

    while (1)
    {
        timer = clock() * 1000 / CLOCKS_PER_SEC;

        if(timer-superfastLoopTimer >= 1)
        {
            readEndoder(currentPosition);
            if(abs(destPosition[0] - currentPosition[0])<= errorMax && abs(destPosition[0] - currentPosition[0])<= errorMax)
            {
                stopRobot();
                if(!isMissionFinished)
                {
                    printf("MC L: %d, D: %d\n",currentPosition[0],destPosition[0]);
                    printf("MC R: %d, D: %d\n",currentPosition[1],destPosition[1]);
                }
                isMissionFinished = true;

                if(commandIndex<commandNumber)
                {
                    destPosition[0] = currentPosition[0] + positionQueue[commandIndex][0];
                    destPosition[1] = currentPosition[1] + positionQueue[commandIndex][1];
                    printf("ND: %d\n",destPosition[0]);
                    printf("ND: %d\n",destPosition[1]);
                    m_speed[0] = positionQueue[commandIndex][2];
                    m_speed[1] = positionQueue[commandIndex][3];
                    updateMotorSpeed();
                    commandIndex++;
                    isMissionFinished = false;
                }
            }

            superfastLoopTimer = timer;
        }
        else if(timer-fastLoopTimer >= 100)
        {
            // int diff = destPosition[0]>currentPosition[0];
            // if(abs(diff)<50)
            // {
            //     m_speed[0] = diff;
            //     if(m_speed[0]>10)
            //         m_speed[0] = 10;
            //     else if(m_speed[0]<-10)
            //         m_speed[0] = -10;
            // }
            // else
            // {
            //     if(diff>0)
            //     {
            //         m_speed[0] = 20;
            //     }
            //     else
            //     {
            //         m_speed[0] = -20;
            //     }
            // }

            // diff = destPosition[1]>currentPosition[1];
            // if(abs(diff)<50)
            // {
            //     m_speed[1] = diff;
            //     if(m_speed[1]>10)
            //         m_speed[1] = 10;
            //     else if(m_speed[1]<-10)
            //         m_speed[1] = -10;
            // }
            // else
            // {
            //     if(diff>0)
            //     {
            //         m_speed[1] = 20;
            //     }
            //     else
            //     {
            //         m_speed[1] = -20;
            //     }
            // }


            // if(destPosition[0]>currentPosition[0])
            // {
            //     m_speed[0] += 1;
            // }
            // else
            // {
            //     m_speed[0] -= 1;
            // }

            // if(destPosition[1]>currentPosition[1])
            // {
            //     m_speed[1] += 1;
            // }
            // else
            // {
            //     m_speed[1] -= 1;
            // }

            if(!isMissionFinished)
            {
                m_speed[0] = destPosition[0] - currentPosition[0];
                if(m_speed[0]>50)
                    m_speed[0] = 50;
                else if(m_speed[0]<-50)
                    m_speed[0] = -50;
                m_speed[1] = destPosition[1] - currentPosition[1];
                if(m_speed[1]>50)
                    m_speed[1] = 50;
                else if(m_speed[1]<-50)
                    m_speed[1] = -50;
                m_speed[0] /= 5;
                m_speed[1] /= 5;
            }

            drawTrack();
            fastLoopTimer = timer;
        }
        else if(timer-slowLoopTimer >= 200)
        {
            if(m_speed[0]!=0)
                printf("L: %d S: %d\n",currentPosition[0],m_speed[0]);
            if(m_speed[1]!=0)
                printf("R: %d S: %d\n",currentPosition[1],m_speed[1]);
            updateMotorSpeed();
            slowLoopTimer = timer;
        }
    }
}

void setCommands()
{
    /// Square
    //goStraight(600,20);
    //rotate(211,5);
    //goStraight(600,20);
    //rotate(211,5);
    //goStraight(600,20);
    //rotate(211,5);
    //goStraight(800,20);
    /// Circle
    //turn(1500,30);
}

int initConnection()
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
    int timeOut;
    while(currentPosition[0] == -1 || currentPosition[1] == -1)
    {
        readEndoder(currentPosition);
        usleep(100000);
        timeOut++;
        if(timeOut>=10)
        {
            fprintf(stderr, "No command received from the robot!\n");
            exit(1);
        }
    }
}

void stopRobot()
{
    m_speed[0] = m_speed[1] = 0;
    updateMotorSpeed();
}

void updateMotorSpeed()
{
    char command[80];
    sprintf(command, "M LR %d %d\n", m_speed[0], m_speed[1]);
    sendCommend(command);
}

void readEndoder(int encoder[2])
{
    char command[80];
    sprintf(command, "S MELR\n");
    char* feedback = sendCommend(command);
    sscanf(feedback,"S MELR %d %d",&encoder[0],&encoder[1]);

    return encoder;
}

void goStraight(int distance, int speed)
{
    positionQueue[commandNumber][0] = distance;
    positionQueue[commandNumber][1] = distance;
    if(distance>0)
    {
        positionQueue[commandNumber][2] = speed;
        positionQueue[commandNumber][3] = speed;
    }
    else
    {
        positionQueue[commandNumber][2] = -speed;
        positionQueue[commandNumber][3] = -speed;
    }

    commandNumber++;
}

void rotate(int degree, int speed)
{
    positionQueue[commandNumber][0] = degree;
    positionQueue[commandNumber][1] = -degree;
    if(degree > 0)
    {
        positionQueue[commandNumber][2] = speed;
        positionQueue[commandNumber][3] = -speed;
    }
    else
    {
        positionQueue[commandNumber][2] = -speed;
        positionQueue[commandNumber][3] = speed;
    }

    commandNumber++;
}

void turn(int degree, int speed)
{
    positionQueue[commandNumber][0] = degree;
    positionQueue[commandNumber][1] = 0;
    if(degree > 0)
    {
        positionQueue[commandNumber][2] = speed;
        positionQueue[commandNumber][3] = 0;
    }
    else
    {
        positionQueue[commandNumber][2] = -speed;
        positionQueue[commandNumber][3] = 0;
    }

    commandNumber++;
}

void drawTrack()
{
    char command[80];
    sprintf(command, "C TRAIL\n");
    sendCommend(command);
}

char* sendCommend(char* command)
{
    write(m_sock, command, strlen(command));
    memset(command, 0, 80);
    read(m_sock, command, 80);
    //printf(command);
    return command;
}