/*
 * malsa.cpp
 *
 *  Created on: Feb 19, 2023
 *      Author: piotrek
 */

#include <alsa/asoundlib.h>
#include <locale.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <termios.h>

void* capture(void *arg);
void* finish(void *arg);

bool do_finish = false;
struct capture_params {
	timespec t_period;
	snd_pcm_uframes_t frames;
	signed short *buffer;
	snd_pcm_t *pcm;

};

int getch(void) {
	struct termios term, oterm;

	int c = 0;

	tcgetattr(0, &oterm);
	memcpy(&term, &oterm, sizeof(term));
	term.c_lflag &= ~(ICANON);
	tcsetattr(0, TCSANOW, &term);
	c = getchar();
	tcsetattr(0, TCSANOW, &oterm);
	return c;
}

timespec timespec_diff(timespec *time1, timespec *time0) {

	assert(time1);
	assert(time0);
	struct timespec diff = { .tv_sec = time1->tv_sec - time0->tv_sec, .tv_nsec =
			time1->tv_nsec - time0->tv_nsec };

	if (diff.tv_nsec < 0) {
		diff.tv_nsec += 1000000000;
		diff.tv_sec--;
	}

	return diff;

}

long timespec_round2u(timespec *val) {

	long nsec = val->tv_nsec;
	return (nsec < 0) ? ((nsec - 1000 / 2) / 1000) : ((nsec + 1000 / 2) / 1000);
}

FILE *fd;

int set_rt() {
	//--to allow rt  priorities
	struct rlimit limit = { 99, 99 };

	if (setrlimit(RLIMIT_RTPRIO, &limit) != 0) {
		printf(
				"setLimit failed:%m\n"
						"remember to change hard rtprio limit to 99 in /etc/security/limits.conf "
						"for normal user\n");
		return -1;
	} else {
		printf("limits are:\%d, %d\n", limit.rlim_cur, limit.rlim_max);
		return 0;
	}

}

void capture_prepare(capture_params *c_params) {
	int rc;
	int size;
	unsigned int val;
	int dir;
	snd_pcm_t *pcm;
	snd_pcm_hw_params_t *params;

	snd_pcm_uframes_t frames = 32;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	u_int channels = 2;
	u_int sample_rate = 48000;
	char *device_name = "default";

	rc = snd_pcm_open(&pcm, device_name, SND_PCM_STREAM_CAPTURE,
	SND_PCM_NONBLOCK);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));

	}

	snd_pcm_hw_params_alloca(&params);

	snd_pcm_hw_params_any(pcm, params);
	snd_pcm_hw_params_set_access(pcm, params, access);
	snd_pcm_hw_params_set_format(pcm, params, format);
	snd_pcm_hw_params_set_channels_near(pcm, params, &channels);
	snd_pcm_hw_params_set_rate_near(pcm, params, &sample_rate, 0);
	snd_pcm_hw_params_set_period_size_near(pcm, params, &frames, &dir);
	snd_pcm_hw_params(pcm, params);

	c_params->pcm = pcm;
	c_params->frames = frames;

	size = frames * 4;  //2 bytes/sample, 2 channels
	c_params->buffer = (signed short*) malloc(size);

	snd_pcm_hw_params_get_period_time(params, &val, &dir);
	c_params->t_period.tv_sec = val / 1000000;
	c_params->t_period.tv_nsec = (val % 1000000) * 1000;

}

int capture_thread_start(pthread_t *thread,capture_params *c_params) {
	struct sched_param param;
	pthread_attr_t attr;
	int ret;

	/* Lock memory */
	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		printf("mlockall failed: %m\n");
		exit(-2);
	}

	/* Initialize pthread attributes (default values) */
	ret = pthread_attr_init(&attr);
	if (ret) {
		printf("init pthread attributes failed\n");
		return ret;
	}

	/* Set a specific stack size  */
	ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
	if (ret) {
		printf("pthread setstacksize failed\n");
		return ret;
	}

	/* Set scheduler policy and priority of pthread */
	ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	if (ret) {
		printf("pthread setschedpolicy failed\n");
		return ret;
	}
	param.sched_priority = 98;
	ret = pthread_attr_setschedparam(&attr, &param);
	if (ret) {
		printf("pthread setschedparam failed\n");
		return ret;
	}
	/* Use scheduling parameters of attr */
	ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	if (ret) {
		printf("pthread setinheritsched failed\n");
		return ret;
	}
	/* Create a pthread with specified attributes */
	ret = pthread_create(thread, &attr, capture, c_params);
	if (ret) {
		printf("create pthread failed: %d\n", ret);
		return ret;
	}

	return 0;
}
int record(char *filename) {
	struct capture_params c_params;
	capture_prepare(&c_params);
	pthread_t thread_capturing;
	int ret;
	//--to format time data
	setlocale(LC_NUMERIC, "");

	fd = fopen(filename, "w");
	if (fd == NULL) {
		fprintf(stderr, "Error opening file %s\n", filename);
		return 1;
	}

	set_rt();
	capture_thread_start(&thread_capturing, &c_params);

	/*Running thread waiting for console input to break programm*/
	pthread_t thread_finish;
	ret = pthread_create(&thread_finish, NULL, finish, NULL);

	/* Join the thread and wait until it is done */
	ret = pthread_join(thread_capturing, NULL);
	if (ret)
		printf("join pthread failed: %m\n");

	out: close(fileno(fd));
	snd_pcm_drop(c_params.pcm);
	snd_pcm_close(c_params.pcm);
	free(c_params.buffer);

	return ret;

}

void* finish(void *arg) {
	printf("press q to exit\n");

	while (1) {
		int c = getch();
		if (c == 'q') {
			do_finish = true;
			int ret = 0;
			pthread_exit(&ret);
		}
	}
}

void* capture(void *arg) {
	struct capture_params *c_params = (capture_params*) arg;
	int rc;
	unsigned long loops = 0;

	snd_pcm_uframes_t avail;
	struct timespec t_start, t_end;
	struct timespec t_start_old, t_diff;

	rc = snd_pcm_start(c_params->pcm);
	if (rc < 0) {
		fprintf(stderr, "error starting: %s\n", snd_strerror(rc));
	}

	while (!do_finish) {

		clock_gettime(CLOCK_MONOTONIC, &t_start);
		loops++;

		avail = snd_pcm_avail(c_params->pcm);

		t_diff = timespec_diff(&t_start, &t_start_old);
		fprintf(stdout, "loop:%ld, available frames:%ld, at time_diff:%'ld\n",
				loops, avail, timespec_round2u(&t_diff));
		t_start_old = t_start;

		if (avail > 0) {

			int count = (avail > c_params->frames) ? c_params->frames : avail;

			rc = snd_pcm_readi(c_params->pcm, c_params->buffer, count);
			if (rc < 0) {
				fprintf(stdout, "loop: %ld, cant read frames: %s\n", loops,
						snd_strerror(rc));

			} else {
				fprintf(stdout, "loop: %ld, read frames: %d\n", loops, rc);
				write(fileno(fd), c_params->buffer, rc * 4);
				if (rc < avail) {
					continue;
				}

			}

		}

		clock_gettime(CLOCK_MONOTONIC, &t_end);

		timespec t_elapsed = timespec_diff(&t_end, &t_start);
		printf("loop:%ld, elapsed:%'ld\n", loops, timespec_round2u(&t_elapsed));

		timespec t_remains = timespec_diff(&c_params->t_period, &t_elapsed);
		printf("loop:%ld, remains:%'ld\n", loops, timespec_round2u(&t_remains));
		if (t_remains.tv_sec == 0) {

			rc = nanosleep(&t_remains, NULL);
			if (rc < 0) {
				fprintf(stdout, "loop:%ld, error sleeping: %s\n", loops,
						strerror(errno));
			}
		}
		clock_gettime(CLOCK_MONOTONIC, &t_end);
		t_elapsed = timespec_diff(&t_end, &t_start);

		printf("loop:%ld. time diff start to end after sleep: %ld\n\n", loops,
				timespec_round2u(&t_elapsed));

	}

	return 0;
}

