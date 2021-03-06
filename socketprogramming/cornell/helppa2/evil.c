#include "headers.h"

#include "evil.h"
#include "eventTimer.h"

//#define DEBUG

/* The following helps print debug statements optionally. */
#ifdef DEBUG
#define PRINTDEBUG(x) x
#else
#define PRINTDEBUG(x) 
#endif

extern int errno;

int crptProb = 0;
int dropProb = 0;
int dupProb = 0;
int delayProb = 0;
int minDelay = 0;
int maxDelay = 0;

int toss (int prob) {
  return ((rand() % 100) < prob);
}

typedef struct DSE {
  int sockfd;
  char *msg;
  size_t msgLen;
  int flags;
  struct sockaddr *toAddr;
  socklen_t addrLen;
} DelayedSendArgsType;

DelayedSendArgsType *makeDelayedSendArgs (int sockfd, char *msg, size_t msgLen, int flags, struct sockaddr *toAddr, socklen_t addrLen) {
  DelayedSendArgsType *args;

  args = (DelayedSendArgsType *)malloc(sizeof(DelayedSendArgsType));
  assert(args != NULL);

  args->sockfd = sockfd;
  args->msg = malloc(msgLen);
  assert(args->msg != NULL);
  memcpy(args->msg, msg, msgLen);
  args->msgLen = msgLen;
  args->flags = flags;
  args->toAddr = (struct sockaddr *)malloc(addrLen);
  assert(args->toAddr != NULL);
  memcpy((char *) args->toAddr, (char *) toAddr, addrLen);
  args->addrLen = addrLen;

  return args;
}

void delayedSend (void *args) {
  DelayedSendArgsType *temp;

  assert(args != NULL);

  temp = (DelayedSendArgsType *)args;

  PRINTDEBUG(printf("EVIL: sending delayed message on %d.\n", temp->sockfd));

  if (sendto(temp->sockfd, temp->msg, temp->msgLen, temp->flags, temp->toAddr, temp->addrLen) < 0) {
    return;
  }
  
  free(temp->msg);
  free(temp->toAddr);
}

void evilInit () {
  FILE *fp;
  char input[50];

  fp = fopen(CONFIG_FILE, "r");
  assert (fp != NULL);

  fscanf(fp, "%s%d", input, &crptProb);
  fscanf(fp, "%s%d", input, &dropProb);
  fscanf(fp, "%s%d", input, &dupProb);
  fscanf(fp, "%s%d", input, &delayProb);
  fscanf(fp, "%s%d", input, &minDelay);
  fscanf(fp, "%s%d", input, &maxDelay);

  fclose(fp);

  assert((0 <= crptProb) && (crptProb <= 100));
  assert((0 <= dropProb) && (dropProb <= 100));
  assert((0 <= dupProb) && (dupProb <= 100));
  assert((0 <= delayProb) && (delayProb <= 100));
  assert((0 <= minDelay) && (minDelay <= maxDelay));

  PRINTDEBUG(printf("Evil send initialized with parameters:\n\tCorrupt-Prob:\t\t%d%%\n\tDrop-Prob:\t\t%d%%\n\tDuplicate-Prob:\t\t%d%%\n\tDelay-Prob:\t\t%d%%\n\tMin-Delay:\t\t%dms\n\tMax-Delay:\t\t%dms\n", crptProb, dropProb, dupProb, delayProb, minDelay, maxDelay));

  eventTimerInit();
}

void evilQuit () {
  eventTimerQuit();
}

ssize_t evilsendto (int sockfd, char *msg, size_t msgLen, int flags, struct sockaddr *toAddr, socklen_t addrLen) {
  int mask;
  int bytesSent;
  DelayedSendArgsType *args;

  if (msgLen > MAX_MSG_LEN) {
    errno = EMSGSIZE;
    return -1;
  }

  mask = sigblock(sigmask(SIGALRM));

  if (toss(crptProb)) {
    int randomByte = (rand() % msgLen);
    msg[randomByte] = ~msg[randomByte];

    PRINTDEBUG(printf("EVIL: corrupting message on %d.\n", sockfd));

    bytesSent = sendto(sockfd, msg, msgLen, flags, toAddr, addrLen);
    msg[randomByte] = ~msg[randomByte];
  }

  else if (toss(dropProb)) {
    PRINTDEBUG(printf("EVIL: droping message on %d.\n", sockfd));

    bytesSent =  msgLen;
  }

  else if (toss(dupProb)) {
    struct timeval delay;
    int milliDelay;
  
    milliDelay = minDelay + rand()%(maxDelay-minDelay+1);
    delay.tv_sec = milliDelay/1000;
    delay.tv_usec = 1000 * (milliDelay%1000);
  
    PRINTDEBUG(printf("EVIL: duplicating message on %d.\n", sockfd));

    if (milliDelay > 0) {
      args = makeDelayedSendArgs(sockfd, msg, msgLen, flags, toAddr, addrLen);
      if (setEvent(delay, delayedSend, args, sizeof(*args)) < 0) {
	fprintf(stderr, "Error: %d in evil send: setEvent.\n", errno);
	exit(0);
      }
      free(args);
    }
    else {
      bytesSent = sendto(sockfd, msg, msgLen, flags, toAddr, addrLen);
    }

    bytesSent = sendto(sockfd, msg, msgLen, flags, toAddr, addrLen);
  }

  else if (toss(delayProb)) {
    struct timeval delay;
    int milliDelay;
  
    milliDelay = minDelay + rand()%(maxDelay-minDelay+1);
    delay.tv_sec = milliDelay/1000;
    delay.tv_usec = 1000 * (milliDelay%1000);
  
    PRINTDEBUG(printf("EVIL: delaying message on %d.\n", sockfd));

    if (milliDelay > 0) {
      args = makeDelayedSendArgs(sockfd, msg, msgLen, flags, toAddr, addrLen);
      if (setEvent(delay, delayedSend, args, sizeof(*args)) < 0) {
	fprintf(stderr, "Error: %d in evil send: setEvent.\n", errno);
	exit(0);
      }
      free(args);
      bytesSent = msgLen;
    }
    else {
      bytesSent = sendto(sockfd, msg, msgLen, flags, toAddr, addrLen);
    }
  }

  else {
    PRINTDEBUG(printf("EVIL: sending message on %d.\n", sockfd));
  
    bytesSent = sendto(sockfd, msg, msgLen, flags, toAddr, addrLen);
  }

  sigsetmask(mask);

  return bytesSent;
}







