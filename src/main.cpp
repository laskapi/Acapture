//============================================================================
// Name        : acapture.cpp
// Author      : laskapi
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
int playback(char *);
int record(char *);

int main(int argc, char *argv[]) {

	//setvbuf(stdout, NULL, _IONBF, 0);
	//setvbuf(stderr, NULL, _IONBF, 0);
	int opt;


	char *filename;
	if(optind<argc){
		filename=argv[optind+1];
	}
	else{
		printf("Podaj nazwe pliku.");
		exit(0);
	}
	while ((opt = getopt(argc, argv, "rp")) != -1) {

		switch (opt) {
		case 'r':
			printf("Nagrywam\n");
			record(filename);
			break;
		case 'p':
			printf("Odtwarzam\n");
			playback(filename);
			break;
		}
	}

	//  display_pcm_types();
	// capture();
	//playback();
	//prepare_recording();
	printf("Dziekuje\n");
	exit(0);
}
