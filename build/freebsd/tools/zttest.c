#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <math.h>

#define SIZE 8000

static int pass = 0;
static float best = 0.0;
static float worst = 100.0;
static float total = 0.0;

int main(int argc, char *argv[])
{
	int fd;
	int res;
	int count=0;
	int seconds;
	int ms;
	int curarg = 1;
	int verbose=0;
	char buf[8192];
	float score;
	struct timeval start, now;
	fd = open("/dev/zap/pseudo", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Unable to open zap interface: %s\n", strerror(errno));
		exit(1);
	}
	while(curarg < argc) {
		if (!strcasecmp(argv[curarg], "-v"))
			verbose++;
		if (!strcasecmp(argv[curarg], "-c") && argc > curarg)
			seconds = atoi(argv[curarg + 1]);
		curarg++;
	}
	printf("Opened pseudo zap interface, measuring accuracy...\n");
	/* Flush input buffer */
	for (count = 0;count < 4; count++)
		res = read(fd, buf, sizeof(buf));
	count = 0;
	gettimeofday(&start, NULL);
	if (seconds > 0)
		alarm(seconds + 1);
	while(pass < 10) {
		res = read(fd, buf, sizeof(buf));
		if (res < 0) {
			fprintf(stderr, "Failed to read from pseudo interface: (error: %s\n", strerror(errno));
			exit(1);
		}
		count += res;
		if (count >= SIZE) {
			gettimeofday(&now, NULL);
			ms = (now.tv_sec - start.tv_sec) * 8000;
			ms += (now.tv_usec - start.tv_usec) / 125;
			start = now;
			if (verbose)
				printf("\n%d samples in %d sample intervals ", count, ms);
			else if ((pass % 8) == 7) printf("\n");
			score = 100.0 - 100.0 * fabs((float)count - (float)ms) / (float)count;
			if (score > best)
				best = score;
			if (score < worst)
				worst = score;
			printf("%f%% ", score);
			total += score;
			fflush(stdout);
			count = 0;
			pass++;
		}
	}
	
	printf("\n--- Results after %d passes ---\n", pass);
	printf("Best: %f -- Worst: %f -- Average: %f\n", best, worst, pass ? total/pass : 100.00);
	exit(0);
}
