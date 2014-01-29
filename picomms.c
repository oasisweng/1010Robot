#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/select.h>
#include "picomms.h"

#define LEFT 0
#define RIGHT 1
static int sock = -1;

int send_msg(char* msg, int len) {
  if (write(sock, msg, len) <= 0) {
    /* the write failed - likely the robot was switched off - attempt
       to reconnect and reinitialize */
    connect_to_robot();
    initialize_robot();
    return 0;
  } else {
    return 1;
  }
}

int recv_msg(char *buf, int bufsize) {
  int val;
  fd_set read_fdset;
  fd_set except_fdset;
  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  FD_ZERO(&read_fdset);
  FD_ZERO(&except_fdset);
  FD_SET(sock, &read_fdset);
  FD_SET(sock, &except_fdset);
  if (select(sock+1, &read_fdset, NULL, &except_fdset, &tv) == 0) {
    /* we've waited 2 seconds and got no response - too long - conclude
       the socket is dead */
    printf("timed out waiting response\n");
    connect_to_robot();
    initialize_robot();
    return 0;
  }
  if (FD_ISSET(sock, &except_fdset)) {
    connect_to_robot();
    initialize_robot();
    return 0;
  }
  
  assert(FD_ISSET(sock, &read_fdset));
  val = read(sock, buf, bufsize);
  if (val > 0) {
  } else {
    /* the write failed - likely the robot was switched off - attempt
       to reconnect and reinitialize */
    connect_to_robot();
    initialize_robot();
  }
  return val;
}

void clear_input_stream() {
  // If we're out of sync, we read until there's no more input
  int val;
  fd_set read_fdset;
  struct timeval tv;
  char buf[1024];
  usleep(500000);
  while (1) {
    /* check if there's any data to read */
    /* we want to return immediately if there's nothing to read */
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&read_fdset);
    FD_SET(sock, &read_fdset);
    if (select(sock+1, &read_fdset, NULL, NULL, &tv) == 0) {
      /* nothing to read - we're done */
      return;
    }
    assert(FD_ISSET(sock, &read_fdset));
    val = read(sock, buf, 1024);
    if (val < 0) {
      /* we got an error; leave cleaning up to elsewhere */
      return;
    }
  }
}


int gp2d12_to_dist(int ir) {
  int dist;
  if (ir > 35)
    dist = (6787 / (ir - 3)) - 4;
  else
    dist=200;
  return dist;
}

int gp2d120_to_dist(int ir) {
  int dist;
  if (ir > 80)
    dist = (2914 / (ir + 5)) - 1;
  else
    dist = 40;
  return dist;
}


void set_motor(int side, int speed) {
  char  buf[80];
  switch(side) {
  case LEFT:
    sprintf(buf, "M L %d\n", speed);
    break;
  case RIGHT:
    sprintf(buf, "M R %d\n", speed);
    break;
  }
  if (send_msg(buf, strlen(buf))) {
    memset(buf, 0, 80);
    recv_msg(buf, 80);
  }
}

void set_motors(int speed_l, int speed_r) {
  char  buf[80];
  sprintf(buf, "M LR %d %d\n", speed_l, speed_r);
  if (send_msg(buf, strlen(buf))) {
    memset(buf, 0, 80);
    recv_msg(buf, 80);
  }
}  

void set_ir_angle(int side, int angle) {
  char  buf[80];
  switch(side) {
  case LEFT:
    sprintf(buf, "I L %d\n", angle);
    break;
  case RIGHT:
    sprintf(buf, "I R %d\n", angle);
    break;
  }
  if (send_msg(buf, strlen(buf))) {
    memset(buf, 0, 80);
    recv_msg(buf, 80);
  }
}

int one_sensor_read(char *sensorname, int *value) {
  char  sendbuf[80];
  char  recvbuf[80];
  char *arg;
  sprintf(sendbuf, "S %s\n", sensorname);
  if (send_msg(sendbuf, strlen(sendbuf))) {
    /* now we loop, reading until we get the answer to the request we just asked. */
    while (1) {
      memset(recvbuf, 0, 80);
      recv_msg(recvbuf, 80);

      /* compare request string to response */
      if (strncmp(recvbuf, sendbuf, strlen(sendbuf)-1)==0) {

	/* skip over the fixed part of the response to get to the
	   returned value */
	arg = recvbuf + strlen(sendbuf);
	*value = atoi(arg);
	/* all done */
	return 0;
      } else if (recvbuf[0] == 'W') {
	/* response is an asynchronous warning - we'll print it and ignore it. */
	printf("Warning: %s", recvbuf+2);
	/* now go round the loop again and re-read */
      } else {
	/* what we got back wasn't the response we expected or a warning. */
	printf("got an error >>%s<<\n", recvbuf);
	/* we don't really know what happened - just clear any
	   remaining input in the hope we can get back in sync. */
	clear_input_stream();
	return -1;
      }
    }
  } else {
    /* the send failed - retry the whole request; it should auto-reconnect */
    return one_sensor_read(sensorname, value);
  }
}

int two_sensor_read(char *sensornames, int *value1, int *value2) {
  char  sendbuf[80];
  char  recvbuf[80];
  char *arg;
  sprintf(sendbuf, "S %s\n", sensornames);
  if (send_msg(sendbuf, strlen(sendbuf))) {
    /* now we loop, reading until we get the answer to the request we just asked. */
    while (1) {
      memset(recvbuf, 0, 80);
      recv_msg(recvbuf, 80);

      /* remove the trailing newline from the request string so we can
	 compare it to the response */
      sendbuf[strlen(sendbuf)-1]='\0';
      if (strncmp(recvbuf, sendbuf, strlen(sendbuf))==0) {

	/* skip over the fixed part of the response to get to the
	   first returned value */
	arg = recvbuf + strlen(sendbuf) + 1;
	*value1 = atoi(arg);

	/* skip to next space character that should be before the second 
	   returned value */
	arg = strchr(arg, ' ');
	if (arg == 0) {
	  printf("got an incomplete response >>%s<<\n", recvbuf);
	  return -1;
	}

	/* skip any spaces (shouldn't really be any though) */
	while(*arg==' ') arg++;

	*value2 = atoi(arg); 
	/* all done */
	return 0;
      } else if (recvbuf[0] == 'W') {
	/* response is an asynchronous warning - we'll print it and ignore it. */
	printf("Warning: %s\n", recvbuf+2);
	/* now go round the loop again and re-read */
      } else {
	/* what we got back wasn't the response we expected or a warning. */
	printf("got an error >>%s<<\n", recvbuf);
	/* we don't really know what happened - just clear any
	   remaining input in the hope we can get back in sync. */
	clear_input_stream();
	return -1;
      }
    }
  } else {
    /* the send failed - retry the whole request; it should auto-reconnect */
    return two_sensor_read(sensornames, value1, value2);
  }
}

int get_front_ir_dist(int side) {
  int result, value;
  switch(side) {
  case LEFT:
    result = one_sensor_read("IFL", &value);
    break;
  case RIGHT:
    result = one_sensor_read("IFR", &value);
    break;
  }
  if (result < 0)
    /* something broke - return 200cm */
    return 200;
  return gp2d12_to_dist(value); 
}

int get_side_ir_dist(int side) {
  int result, value;
  switch(side) {
  case LEFT:
    result = one_sensor_read("ISL", &value);
    break;
  case RIGHT:
    result = one_sensor_read("ISR", &value);
    break;
  }
  if (result < 0)
    /* something broke - return 50cm */
    return 50;
  return gp2d120_to_dist(value); 
}

int get_us_dist() {
  int result, value;
  result = one_sensor_read("US", &value);
  if (result < 0)
    /* something broke - return 5 meters */
    return 500;
  return value;
}

int get_front_ir_dists(int *leftdist, int *rightdist) {
  int leftrawdist, rightrawdist;
  int result;
  result = two_sensor_read("IFLR", &leftrawdist, &rightrawdist);
  if (result < 0)
    return result;
  *leftdist = gp2d12_to_dist(leftrawdist); 
  *rightdist = gp2d12_to_dist(rightrawdist); 
  return 0;
}

int get_side_ir_dists(int *leftdist, int *rightdist) {
  int leftrawdist, rightrawdist;
  int result;
  result = two_sensor_read("ISLR", &leftrawdist, &rightrawdist);
  if (result < 0)
    return result;
  *leftdist = gp2d120_to_dist(leftrawdist); 
  *rightdist = gp2d120_to_dist(rightrawdist); 
  return 0;
}

int get_motor_encoders(int *leftenc, int *rightenc) {
  int result;
  result = two_sensor_read("MELR", leftenc, rightenc);
  if (result < 0)
    return result;
  return 0;
}

int check_bump(int side) {
  int result, value;
  switch(side) {
  case LEFT:
    result = one_sensor_read("BFL", &value);
    break;
  case RIGHT:
    result = one_sensor_read("BFR", &value);
    break;
  }
  if (result < 0)
    return result;
  return value; 
}

int check_bumpers(int *lbump, int *rbump) {
  return two_sensor_read("BFLR", lbump, rbump);
}

int get_voltage() {
  int result, value;
  result = one_sensor_read("V", &value);
  if (result < 0)
    return result;
  return value; 
}

void log_trail() {
  char  buf[80];
  sprintf(buf, "C TRAIL\n");
  if (send_msg(buf, strlen(buf))) {
    memset(buf, 0, 80);
    recv_msg(buf, 80);
  }
}

void send_text(char *txt) {
  char  buf[80];
  sprintf(buf, "T %s\n", txt);
  if (send_msg(buf, strlen(buf))) {
    memset(buf, 0, 80);
    recv_msg(buf, 80);
  }
}

void set_asr(int flag) {
  char  buf[80];
  sprintf(buf, "C ASR %d\n", flag);
  if (send_msg(buf, strlen(buf))) {
    memset(buf, 0, 80);
    recv_msg(buf, 80);
  }
}

int connect_to_robot() {
  int volts;
  printf("connecting...");
  struct sockaddr_in s_addr;
  if (sock != -1) {
    close(sock);
    sock = -1;
  }

  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    fprintf(stderr, "Failed to create socket\n");
    exit(1);
  }

  while (1) {
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    s_addr.sin_port = htons(55443);

    if (connect(sock, (struct sockaddr *) &s_addr, sizeof(s_addr)) >= 0) {
      /* connection succeeded */
      printf("done\n");
      sleep(1);
      volts = get_voltage();
      printf("Battery state %2.1f volts\n", volts/10.0);
      return sock;
    }
    sleep(1);
    printf(".");
    fflush(stdout);
  }
}


void initialize_robot() {
  set_ir_angle(LEFT, 0);
  set_ir_angle(RIGHT, 0);
}

