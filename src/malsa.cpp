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

#include"malsa.h"

static void capture_params_set(capture_params *c_params);
static int rt_thread_start(pthread_t *thread, void* (*thread_function)(void*),
		void *thread_params, int priority);
static int rt_init();
static void* capture(void *arg);
static void* consume(void *arg);
static void* finish(void *arg);

static bool operator <(const timespec &lhs, const timespec &rhs) {
	if (lhs.tv_sec == rhs.tv_sec)
		return lhs.tv_nsec < rhs.tv_nsec;
	else
		return lhs.tv_sec < rhs.tv_sec;
}

static int getch(void) {
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

static timespec timespec_diff(timespec *time1, timespec *time0) {

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

static long timespec_round2u(timespec *val) {

	long nsec = val->tv_nsec;
	return (nsec < 0) ? ((nsec - 1000 / 2) / 1000) : ((nsec + 1000 / 2) / 1000);
}

int record(char *filename) {

	int ret;

	//--to format time data
	setlocale(LC_NUMERIC, "");

	rt_init();

	//file to write read data
	FILE *fd;
	fd = fopen(filename, "w");
	if (fd == NULL) {
		fprintf(stderr, "Error opening file %s\n", filename);
		return 1;
	}

	struct capture_params cap_params;
	capture_params_set(&cap_params);
	cap_params.fd = fd;

	struct consume_params con_params;
	con_params.fd = fd;
	con_params.buffer = cap_params.buffer;

	/*Running rt consuming thread*/
	pthread_t thread_consuming;
	rt_thread_start(&thread_consuming, consume, &con_params, 98);

	/*Running capturing thread*/
	pthread_t thread_capturing;
	ret = pthread_create(&thread_capturing, NULL, capture, &cap_params);

	/*Running thread waiting for console input to break program*/
	pthread_t thread_quitting;
	ret = pthread_create(&thread_quitting, NULL, finish, NULL);

	/* Join the thread and wait until it is done */
	ret = pthread_join(thread_capturing, NULL);
	if (ret)
		printf("join pthread failed: %m\n");

	pthread_cancel(thread_consuming);
	close(fileno(fd));

	snd_pcm_drop(cap_params.pcm);
	snd_pcm_close(cap_params.pcm);
	free(cap_params.buffer);

	return ret;

}

static void capture_params_set(capture_params *c_params) {
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
	u_int sample_rate = 8000;
	char *device_name = "default";

	rc = snd_pcm_open(&pcm, device_name, SND_PCM_STREAM_CAPTURE,
/*	SND_PCM_NONBLOCK*/0);
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
	val = 2;
	snd_pcm_hw_params_set_periods(pcm, params, val, dir);

	snd_pcm_hw_params(pcm, params);

	c_params->pcm = pcm;
	c_params->frames = frames;

	snd_pcm_hw_params_get_buffer_size(params, &frames);
	printf("buffer size:%d\n", frames);
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	printf("period size:%d\n", frames);

	size = frames * 4;  //2 bytes/sample, 2 channels
	c_params->buffer = (signed short*) malloc(size);

	snd_pcm_hw_params_get_period_time(params, &val, &dir);
	c_params->t_period.tv_sec = val / 1000000;
	c_params->t_period.tv_nsec = (val % 1000000) * 1000;

}

static int rt_init() {
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

static int rt_thread_start(pthread_t *thread, void* (*thread_function)(void*),
		void *thread_params, int priority) {
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
	param.sched_priority = priority;
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
	ret = pthread_create(thread, &attr, thread_function, thread_params);
	if (ret) {
		printf("create pthread failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static void* consume(void *con_params) {

	signed short *buffer = ((consume_params*) con_params)->buffer;
	int fd = fileno(((consume_params*) con_params)->fd);
	timespec t_wake;
	const int PERIOD_NS = 1000;

	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &t_wake);
		printf("Consumer wake time:%'lld.%'lld, ", t_wake.tv_sec,
				t_wake.tv_nsec);
		t_wake.tv_nsec += PERIOD_NS;
		if (t_wake.tv_nsec > 1000000000) {
			t_wake.tv_sec += 1;
			t_wake.tv_nsec -= 1000000000;
		}

		int i = 0;

		printf("Consumer buffer read:%d: ", buffer_read);

		while (i < buffer_read) {
			printf("%d,", buffer[i]);
			i++;
		}
		int rc=write(fd, buffer, buffer_read*4);
		buffer_read = 0;

		printf("\n wrote %d bytes\n",rc);

		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t_wake, NULL);
	}
	return 0;

}

static void* finish(void *arg) {
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

static void* capture(void *arg) {
	struct capture_params *c_params = (capture_params*) arg;
	int rc, avail;
	struct timespec t_start, t_end, t_old, t_diff, t_wake;
	unsigned long loops;
	rc = snd_pcm_start(c_params->pcm);
	while (!do_finish || avail>0) {
		loops++;
		if (do_finish){
			printf("avail:%d\n",avail);
					snd_pcm_drain(c_params->pcm);
		}
		clock_gettime(CLOCK_MONOTONIC, &t_start);

		avail = snd_pcm_avail(c_params->pcm);

		t_diff = timespec_diff(&t_start, &t_old);
		t_old = t_start;
		fprintf(stdout,
				"\nloop:%ld, available frames:%ld, at time_diff:%'ld ns\n",
				loops, avail, t_diff.tv_nsec);

		while (avail == 0 || buffer_read!=0) {
			t_wake.tv_sec = t_start.tv_sec;
			t_wake.tv_nsec = t_start.tv_nsec + c_params->t_period.tv_nsec;
			if (t_wake.tv_nsec > 1000000000) {
				t_wake.tv_sec++;
				t_wake.tv_nsec -= 1000000000;
			}

			clock_gettime(CLOCK_MONOTONIC, &t_end);
			t_diff = timespec_diff(&t_end, &t_start);
			printf("loop: %ld, sleeping %'ld\n", loops, t_diff.tv_nsec);
			rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t_wake, NULL);
			if (rc < 0) {
				fprintf(stdout, "loop:%ld, error sleeping: %s\n", loops, rc);
			}
			avail = snd_pcm_avail(c_params->pcm);

		}

		int amount_to_read;
		(c_params->frames) < avail ?
				amount_to_read = c_params->frames : amount_to_read = avail;
		rc = snd_pcm_readi(c_params->pcm, c_params->buffer, amount_to_read);
		buffer_read = rc;

	//	write(fileno(c_params->fd), c_params->buffer, rc*4);

		if (rc < 0) {
			fprintf(stdout, "loop: %ld, cant read frames: %s\n", loops,
					snd_strerror(rc));

		} else {
			fprintf(stdout, "loop: %ld, read frames: %d\n", loops, rc);

		}

		t_wake.tv_sec = t_start.tv_sec;
		t_wake.tv_nsec = t_start.tv_nsec + c_params->t_period.tv_nsec;
		if (t_wake.tv_nsec > 1000000000) {
			t_wake.tv_sec++;
			t_wake.tv_nsec -= 1000000000;
		}

		clock_gettime(CLOCK_MONOTONIC, &t_end);
		t_diff = timespec_diff(&t_end, &t_start);
		printf("loop: %ld, took %'ld\n", loops, t_diff.tv_nsec);

	}
	return 0;

}

static void* capture1(void *arg) {
	struct capture_params *c_params = (capture_params*) arg;
	int rc, avail;
	struct timespec t_start, t_end, t_old, t_diff, t_wake;
	unsigned long loops;
	rc = snd_pcm_start(c_params->pcm);
	while (!do_finish) {
		loops++;

		clock_gettime(CLOCK_MONOTONIC, &t_start);

		avail = snd_pcm_avail(c_params->pcm);

		t_diff = timespec_diff(&t_start, &t_old);
		t_old = t_start;
		fprintf(stdout,
				"\nloop:%ld, available frames:%ld, at time_diff:%'ld ns\n",
				loops, avail, t_diff.tv_nsec);

		int amount_to_read;
		(c_params->frames) < avail ?
				amount_to_read = c_params->frames : amount_to_read = avail;
		rc = snd_pcm_readi(c_params->pcm, c_params->buffer, amount_to_read);
		buffer_read = rc;

		write(fileno(c_params->fd), c_params->buffer, buffer_read);

		if (rc < 0) {
			fprintf(stdout, "loop: %ld, cant read frames: %s\n", loops,
					snd_strerror(rc));

		} else {
			fprintf(stdout, "loop: %ld, read frames: %d\n", loops, rc);

		}

		t_wake.tv_sec = t_start.tv_sec;
		t_wake.tv_nsec = t_start.tv_nsec + c_params->t_period.tv_nsec;
		if (t_wake.tv_nsec > 1000000000) {
			t_wake.tv_sec++;
			t_wake.tv_nsec -= 1000000000;
		}

		clock_gettime(CLOCK_MONOTONIC, &t_end);
		t_diff = timespec_diff(&t_end, &t_start);
		printf("loop: %ld, took %'ld\n", loops, t_diff.tv_nsec);

		/*if (t_end < t_wake) {
		 rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t_wake, NULL);
		 if (rc < 0) {
		 fprintf(stdout, "loop:%ld, error sleeping: %s\n", loops,
		 strerror(errno));
		 } else {
		 printf("loop: %ld should wake up at  %'ld.%'ld \n", loops,
		 t_wake.tv_sec, t_wake.tv_nsec);

		 clock_gettime(CLOCK_MONOTONIC, &t_end);
		 printf("loop: %ld, woke up at %'ld.%'ld\n", loops, t_end.tv_sec,
		 t_end.tv_nsec);

		 }

		 }*/
	}
	return 0;

}

