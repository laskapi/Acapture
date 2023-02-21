/*
 * malsa.cpp
 *
 *  Created on: Feb 19, 2023
 *      Author: piotrek
 */

#include <alsa/asoundlib.h>

int record(char *filename) {
	long loops;
	int rc;
	int size;
	unsigned int val;
	int dir;
	signed short *buffer;

	snd_pcm_t *pcm;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;

	char *device_name = "default";

	FILE *fd;
	fd = fopen(filename, "w");
	if (fd == NULL) {
		fprintf(stderr, "Error opening file %s\n", filename);
		return 1;
	}

	rc = snd_pcm_open(&pcm, device_name, SND_PCM_STREAM_CAPTURE,
	SND_PCM_NONBLOCK);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));

	}

	snd_pcm_hw_params_alloca(&params);

	snd_pcm_hw_params_any(pcm, params);

	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	snd_pcm_hw_params_set_access(pcm, params, access);

	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	snd_pcm_hw_params_set_format(pcm, params, format);

	u_int channels = 2;
	snd_pcm_hw_params_set_channels_near(pcm, params, &channels);

	u_int sample_rate = 48000;
	snd_pcm_hw_params_set_rate_near(pcm, params, &sample_rate, 0);

	frames = 32;
	snd_pcm_hw_params_set_period_size_near(pcm, params, &frames, &dir);

	snd_pcm_hw_params(pcm, params);

	size = frames * 4;  //2 bytes/sample, 2 channels
	buffer = (signed short*) malloc(size);

	snd_pcm_hw_params_get_period_time(params, &val, &dir);
	loops = 5000000 / val;

	rc = snd_pcm_start(pcm);
	if (rc < 0) {
		fprintf(stderr, "error starting: %s\n", snd_strerror(rc));
	}

	while (loops > 0) {
		loops--;
		snd_pcm_sframes_t avail = snd_pcm_avail(pcm);
		fprintf(stdout, "loop: %d, available: %u\n", loops, avail);

		if (avail < frames) {

			snd_pcm_uframes_t remain = frames - avail;

			unsigned long usec = (remain * 1000000) / 48000;

			fprintf(stdout, "loop:%d, remains:%u, usec:%u\n", loops, remain,
					usec);

			struct timespec ts;
			ts.tv_sec = usec / 1000000;
			ts.tv_nsec = (usec % 1000000) * 1000;
			int a;
			while ((a=snd_pcm_avail(pcm))==0) {
				rc = nanosleep(&ts, NULL);

				printf("slept for %d\n",ts.tv_nsec);
				if (rc < 0) {
					fprintf(stderr, "error sleeping: %m\n");
				}
			}
		}
		rc = snd_pcm_readi(pcm, buffer, frames);
		if (rc < 0) {
			fprintf(stdout, "loop: %d, cant read frames: %s\n", loops,
					snd_strerror(rc));

		} else {
			fprintf(stdout, "loop: %d, read frames: %d\n", loops, rc);
		}

		write(fileno(fd), buffer, size);

	}

	close(fileno(fd));

	snd_pcm_drop(pcm);
	snd_pcm_close(pcm);
	free(buffer);

	return 0;
}
