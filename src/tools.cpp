//============================================================================
// Name        : .cpp
// Author      : laskapi
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <alsa/asoundlib.h>
#define ALSA_PCM_NEW_HW_PARAMS_API

int capture() {
	long loops;
	int rc;
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	//char *buffer;
	signed short *buffer;
	/* Open PCM device for recording (capture). */
	rc = snd_pcm_open(&handle, "default"/*"hw:0,1"*/, SND_PCM_STREAM_CAPTURE,
			0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));

	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, 2);

	/* 44100 bits/second sampling rate (CD quality) */
	val = 44100;
	snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

	/* Set period size to 32 frames. */
	frames = 32;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));

	}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	size = frames * 4; /* 2 bytes/sample, 2 channels */
	buffer = (signed short*) malloc(size);

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(params, &val, &dir);
//	  loops = 50000000 / val;

	loops = 5000000 / val;

	while (loops > 0) {
		loops--;
		rc = snd_pcm_readi(handle, buffer, frames);
		if (rc == -EPIPE) {
			/* EPIPE means overrun */
			fprintf(stderr, "overrun occurred\n");
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
		} else if (rc != (int) frames) {
			fprintf(stderr, "short read, read %d frames\n", rc);
		}

	//	rc = write(1, buffer, size);

			    for(int j=0;j<128;j++){
//		if (buffer+j>1000)
	    	fprintf(stdout,"%d: %d\n",j,buffer[j]);
		}

		if (rc != size)
			fprintf(stderr, "short write: wrote %d bytes\n", rc);
	}

	snd_pcm_drop(handle);
	snd_pcm_close(handle);
	free(buffer);
	fflush(stdout);
	fprintf(stdout,"\nSound captured\n");
	return 0;
}

/*
 int capture()
 {
 //char name[]="sysdefault";
 int i;
 int err;
 char *buffer;
 int buffer_frames = 128;
 unsigned int rate = 44100;
 snd_pcm_t *capture_handle;
 snd_pcm_hw_params_t *hw_params;
 snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

 if ((err = snd_pcm_open (&capture_handle, "defaults", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
 fprintf (stderr, "cannot open audio device %s (%s)\n",
 "defaults",
 snd_strerror (err));
 return 1;
 }

 fprintf(stdout, "audio interface opened\n");

 if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
 fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
 snd_strerror (err));
 return 1;
 }

 fprintf(stdout, "hw_params allocated\n");

 if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
 fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
 snd_strerror (err));
 return 1;
 }

 fprintf(stdout, "hw_params initialized\n");

 if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
 fprintf (stderr, "cannot set access type (%s)\n",
 snd_strerror (err));
 return 1;
 }

 fprintf(stdout, "hw_params access setted\n");

 if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, format)) < 0) {
 fprintf (stderr, "cannot set sample format (%s)\n",
 snd_strerror (err));
 return 1;
 }

 fprintf(stdout, "hw_params format setted\n");

 if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0) {
 fprintf (stderr, "cannot set sample rate (%s)\n",
 snd_strerror (err));
 return 1;
 }

 fprintf(stdout, "hw_params rate setted\n");

 if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 2)) < 0) {
 fprintf (stderr, "cannot set channel count (%s)\n",
 snd_strerror (err));
 return 1;
 }

 fprintf(stdout, "hw_params channels setted\n");

 if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
 fprintf (stderr, "cannot set parameters (%s)\n",
 snd_strerror (err));
 return 1;
 }

 fprintf(stdout, "hw_params setted\n");

 snd_pcm_hw_params_free (hw_params);

 fprintf(stdout, "hw_params freed\n");

 if ((err = snd_pcm_prepare (capture_handle)) < 0) {
 fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
 snd_strerror (err));
 return 1;
 }

 fprintf(stdout, "audio interface prepared\n");

 buffer = (char*) malloc(128 * snd_pcm_format_width(format) / 8 * 2);

 fprintf(stdout, "buffer allocated\n");

 struct timespec start,end;

 clock_gettime(CLOCK_MONOTONIC, &start);


 while(1){

 clock_gettime(CLOCK_MONOTONIC, &end);
 if(((end.tv_sec)-(start.tv_sec))>100) break;

 if ((err = snd_pcm_readi (capture_handle, buffer, buffer_frames)) != buffer_frames) {
 fprintf (stderr, "read from audio interface failed (%s)\n",
 err, snd_strerror (err));
 return 1;
 }
 // printf("%.*s",128,buffer);
 //fwrite(buffer,2,1,stdout);
 // fflush(stdout);

 // for(int j=0;j<128;j++){
 //    	fprintf(stdout,"%d,",buffer[0]);
 // }

 fprintf(stdout, "buffer %d\n", i++);

 }

 free(buffer);

 fprintf(stdout, "buffer freed\n");

 snd_pcm_close (capture_handle);
 fprintf(stdout, "audio interface closed\n");

 return 0;
 }*/

/*
 int capture(){
 int i;
 int err;
 short buf[128];
 snd_pcm_t *capture_handle;
 snd_pcm_hw_params_t *hw_params;

 if ((err = snd_pcm_open (&capture_handle,"default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
 fprintf (stderr, "cannot open audio device %s (%s)\n",
 "default",
 snd_strerror (err));
 return 1;
 }

 if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
 fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
 snd_strerror (err));
 return 1;
 }

 if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
 fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
 snd_strerror (err));
 return 1;
 }

 if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
 fprintf (stderr, "cannot set access type (%s)\n",
 snd_strerror (err));
 return 1;	}

 if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
 fprintf (stderr, "cannot set sample format (%s)\n",
 snd_strerror (err));
 return 1;	}

 static unsigned int rate = 44100;
 static int dir = 0;


 if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, &dir 44100, 0)) < 0) {
 fprintf (stderr, "cannot set sample rate (%s)\n",
 snd_strerror (err));
 return 1;	}

 if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 2)) < 0) {
 fprintf (stderr, "cannot set channel count (%s)\n",
 snd_strerror (err));
 return 1;	}

 if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
 fprintf (stderr, "cannot set parameters (%s)\n",
 snd_strerror (err));
 return 1;	}

 snd_pcm_hw_params_free (hw_params);

 if ((err = snd_pcm_prepare (capture_handle)) < 0) {
 fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
 snd_strerror (err));
 return 1;	}

 for (i = 0; i < 10; ++i) {
 if ((err = snd_pcm_readi (capture_handle, buf, 128)) != 128) {
 fprintf (stderr, "read from audio interface failed (%s)\n",
 snd_strerror (err));
 return 1;
 }
 }

 snd_pcm_close (capture_handle);
 return 0;
 }*/

int display_pcm_types() {
	int val;

	printf("ALSA library version: %s\n",
	SND_LIB_VERSION_STR);

	printf("\nPCM stream types:\n");
	for (val = 0; val <= SND_PCM_STREAM_LAST; val++)
		printf("  %s\n", snd_pcm_stream_name((snd_pcm_stream_t) val));

	printf("\nPCM access types:\n");
	for (val = 0; val <= SND_PCM_ACCESS_LAST; val++)
		printf("  %s\n", snd_pcm_access_name((snd_pcm_access_t) val));

	printf("\nPCM formats:\n");
	for (val = 0; val <= SND_PCM_FORMAT_LAST; val++)
		if (snd_pcm_format_name((snd_pcm_format_t) val) != NULL)
			printf("  %s (%s)\n", snd_pcm_format_name((snd_pcm_format_t) val),
					snd_pcm_format_description((snd_pcm_format_t) val));

	printf("\nPCM subformats:\n");
	for (val = 0; val <= SND_PCM_SUBFORMAT_LAST; val++)
		printf("  %s (%s)\n", snd_pcm_subformat_name((snd_pcm_subformat_t) val),
				snd_pcm_subformat_description((snd_pcm_subformat_t) val));

	printf("\nPCM states:\n");
	for (val = 0; val <= SND_PCM_STATE_LAST; val++)
		printf("  %s\n", snd_pcm_state_name((snd_pcm_state_t) val));

	return 0;
}

/*

 int set_default_pcm(){

 int rc;
 snd_pcm_t *handle;
 snd_pcm_hw_params_t *params;
 unsigned int val, val2;
 int dir;
 snd_pcm_uframes_t frames;

 Open PCM device for playback.
 rc = snd_pcm_open(&handle, "default",
 SND_PCM_STREAM_PLAYBACK, 0);
 if (rc < 0) {
 fprintf(stderr,
 "unable to open pcm device: %s\n",
 snd_strerror(rc));
 return 1;
 }

 Allocate a hardware parameters object.
 snd_pcm_hw_params_alloca(&params);

 Fill it in with default values.
 snd_pcm_hw_params_any(handle, params);

 Set the desired hardware parameters.

 Interleaved mode
 snd_pcm_hw_params_set_access(handle, params,
 SND_PCM_ACCESS_RW_INTERLEAVED);

 Signed 16-bit little-endian format
 snd_pcm_hw_params_set_format(handle, params,
 SND_PCM_FORMAT_S16_LE);

 Two channels (stereo)
 snd_pcm_hw_params_set_channels(handle, params, 2);

 44100 bits/second sampling rate (CD quality)
 val = 44100;
 snd_pcm_hw_params_set_rate_near(handle,
 params, &val, &dir);

 Write the parameters to the driver
 rc = snd_pcm_hw_params(handle, params);
 if (rc < 0) {
 fprintf(stderr,
 "unable to set hw parameters: %s\n",
 snd_strerror(rc));
 return 1;
 }

 Display information about the PCM interface

 printf("PCM handle name = '%s'\n",
 snd_pcm_name(handle));

 printf("PCM state = %s\n",
 snd_pcm_state_name(snd_pcm_state(handle)));

 snd_pcm_hw_params_get_access(params,
 (snd_pcm_access_t *) &val);
 printf("access type = %s\n",
 snd_pcm_access_name((snd_pcm_access_t)val));

 snd_pcm_hw_params_get_format(params, &val);
 printf("format = '%s' (%s)\n",
 snd_pcm_format_name((snd_pcm_format_t)val),
 snd_pcm_format_description(
 (snd_pcm_format_t)val));

 snd_pcm_hw_params_get_subformat(params,
 (snd_pcm_subformat_t *)&val);
 printf("subformat = '%s' (%s)\n",
 snd_pcm_subformat_name((snd_pcm_subformat_t)val),
 snd_pcm_subformat_description(
 (snd_pcm_subformat_t)val));

 snd_pcm_hw_params_get_channels(params, &val);
 printf("channels = %d\n", val);

 snd_pcm_hw_params_get_rate(params, &val, &dir);
 printf("rate = %d bps\n", val);

 snd_pcm_hw_params_get_period_time(params,
 &val, &dir);
 printf("period time = %d us\n", val);

 snd_pcm_hw_params_get_period_size(params,
 &frames, &dir);
 printf("period size = %d frames\n", (int)frames);

 snd_pcm_hw_params_get_buffer_time(params,
 &val, &dir);
 printf("buffer time = %d us\n", val);

 snd_pcm_hw_params_get_buffer_size(params,
 (snd_pcm_uframes_t *) &val);
 printf("buffer size = %d frames\n", val);

 snd_pcm_hw_params_get_periods(params, &val, &dir);
 printf("periods per buffer = %d frames\n", val);

 snd_pcm_hw_params_get_rate_numden(params,
 &val, &val2);
 printf("exact rate = %d/%d bps\n", val, val2);

 val = snd_pcm_hw_params_get_sbits(params);
 printf("significant bits = %d\n", val);

 snd_pcm_hw_params_get_tick_time(params,
 &val, &dir);
 printf("tick time = %d us\n", val);

 val = snd_pcm_hw_params_is_batch(params);
 printf("is batch = %d\n", val);

 val = snd_pcm_hw_params_is_block_transfer(params);
 printf("is block transfer = %d\n", val);

 val = snd_pcm_hw_params_is_double(params);
 printf("is double = %d\n", val);

 val = snd_pcm_hw_params_is_half_duplex(params);
 printf("is half duplex = %d\n", val);

 val = snd_pcm_hw_params_is_joint_duplex(params);
 printf("is joint duplex = %d\n", val);

 val = snd_pcm_hw_params_can_overrange(params);
 printf("can overrange = %d\n", val);

 val = snd_pcm_hw_params_can_mmap_sample_resolution(params);
 printf("can mmap = %d\n", val);

 val = snd_pcm_hw_params_can_pause(params);
 printf("can pause = %d\n", val);

 val = snd_pcm_hw_params_can_resume(params);
 printf("can resume = %d\n", val);

 val = snd_pcm_hw_params_can_sync_start(params);
 printf("can sync start = %d\n", val);

 snd_pcm_close(handle);

 return 0;
 }


 */

int playback(char* filename) {
	long loops;
	int rc;
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;

	FILE * fd;

	fd = fopen(filename, "r");
	if (fd == NULL) {
		fprintf(stderr, "Error opening file %s\n", filename);
		return 1;
	}


	/* Open PCM device for playback. */
	rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, 2);

	/* 44100 bits/second sampling rate (CD quality) */
	val = 44100;
	snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

	/* Set period size to 32 frames. */
	frames = 32;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
		exit(1);
	}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	size = frames * 4; /* 2 bytes/sample, 2 channels */
	buffer = (char*) malloc(size);

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(params, &val, &dir);
	/* 5 seconds in microseconds divided by
	 * period time */
	loops = 5000000 / val;

	while (loops > 0) {
		loops--;
		rc = read(fileno(fd), buffer, size);
		if (rc == 0) {
			fprintf(stderr, "end of file on input\n");
			break;
		} else if (rc != size) {
			fprintf(stderr, "short read: read %d bytes\n", rc);
		}
		rc = snd_pcm_writei(handle, buffer, frames);
		if (rc == -EPIPE) {
			/* EPIPE means underrun */
			fprintf(stderr, "underrun occurred\n");
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			fprintf(stderr, "error from writei: %s\n", snd_strerror(rc));
		} else if (rc != (int) frames) {
			fprintf(stderr, "short write, write %d frames\n", rc);
		}
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);

	return 0;
}
