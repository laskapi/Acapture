//============================================================================
// Name        : acapture.cpp
// Author      : laskapi
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int playback(char*);
int record(char*);

int main(int argc, char *argv[]) {

	//setvbuf(stdout, NULL, _IONBF, 0);
	//setvbuf(stderr, NULL, _IONBF, 0);
	int opt;

	char *filename = "sound.raw";

/*	if (optind < argc) {
		filename = argv[optind + 1];
	}*/

	while ((opt = getopt(argc, argv, "rp")) != -1) {

		switch (opt) {
		case 'r':
			printf("Nagrywam\n");
			record(filename);
			break;
		case 'p':
			printf("Odtwarzam %s\n",filename);
			playback(filename);
			break;

		}
	}

	if (argc == 1) {
		printf("Nagrywam\n");
		record(filename);
	}

	printf("Dziekuje\n");
	exit(0);
}
