/*
 * AfterStep Sound System - sound module for Wharf
 *
 * Copyright (c) 1996 by Alfredo Kojima
 */
/*
 * Todo: 
 * realtime audio mixing
 * replace the Audio module 
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#define AUDIO_DEVICE		"/dev/audio"

typedef struct LList {
    struct LList *next;
    struct LList *prev;
    int data;
    int timestamp;
} LList;

typedef struct SoundEntry {
    char *data;
    int size;
} SoundEntry;

/* timeout time for sound playing in seconds */
#define PLAY_TIMEOUT 1

/* table of audio data (not filenames) */
SoundEntry **SoundTable;

int LastSound=0;
char *SoundPlayer;

char *ProgName;
LList *PlayQueue=NULL, *OldestQueued=NULL;

int InPipe;

void *ckmalloc(size_t size)
{
    void *tmp;
    
    tmp=malloc(size);
    if (tmp==NULL) {
	fprintf(stderr,"%s: virtual memory exhausted.",ProgName);
	exit(1);
    }
    return tmp;
}

/*
 * Register --
 * 	makes a new sound entry
 */
void Register(char *sound_file, int code)
{
    int file;
    struct stat stat_buf;

    if (sound_file[0]=='.' && sound_file[1]==0) {
	SoundTable[code]=ckmalloc(sizeof(SoundEntry));	
	SoundTable[code]->size = -1;
	return; /* dummy sound */
    }    
    if (stat(sound_file, &stat_buf)<0) {
	fprintf(stderr,"%s: sound file %s not found\n",ProgName,
		sound_file);
	return;
    }
    SoundTable[code]=ckmalloc(sizeof(SoundEntry));
    if (SoundPlayer!=NULL) {
	SoundTable[code]->data=ckmalloc(strlen(sound_file));
	SoundTable[code]->size=1;
	strcpy(SoundTable[code]->data,sound_file);
	return;
    }
    SoundTable[code]->data=ckmalloc(stat_buf.st_size);
    file=open(sound_file, O_RDONLY);
    if (file<0) {
	fprintf(stderr,"%s: can't open sound file %s\n",ProgName, sound_file);
	free(SoundTable[code]->data);
	free(SoundTable[code]);
	SoundTable[code]=NULL;
	return;
    }
    SoundTable[code]->size=read(file,SoundTable[code]->data,stat_buf.st_size);
    if (SoundTable[code]->size<1) {
	fprintf(stderr,"%s: error reading sound file %s\n",ProgName,
	       sound_file);
	free(SoundTable[code]->data);
	free(SoundTable[code]);
	close(file);	
	SoundTable[code]=NULL;
	return;
    }
    close(file);
}

/*
 * PlaySound --
 * 	plays a sound
 */

void PlaySound(int sid)
{
    int audio=-1;
    if ((sid < LastSound) && (SoundTable[sid]->size<=0)) return;
    if ((sid>=LastSound) || (sid<0) || SoundTable[sid]==NULL) {
	fprintf(stderr,"%s: request to play invalid sound received\n",
		ProgName);
	return;
    }
    if (SoundPlayer!=NULL) {
	static char cmd[1024];
	pid_t child;
	
	child = fork();
	if (child<0) return;
	else if (child==0) {
	    execlp(SoundPlayer,SoundPlayer,SoundTable[sid]->data,NULL);
	} else {
	    while (wait(NULL)<0);
	}	
	/*
	sprintf(cmd,"%s %s",SoundPlayer,SoundTable[sid]->data);
	system(cmd);
	 */
	return;
    }
#if 0    
    audio = open(AUDIO_DEVICE,O_WRONLY|O_NONBLOCK);
    if ((audio<0) && errno==EAGAIN) {
	sleep(1);
	audio = open(AUDIO_DEVICE,O_WRONLY|O_NONBLOCK);
	if (audio<0) return;
    }
    write(audio, SoundTable[sid]->data,SoundTable[sid]->size);
    close(audio);
    audio=-1;
#endif    
}

void DoNothing(int foo) 
{
    signal(SIGUSR1, DoNothing);    
}

/*
 * HandleRequest --
 * 	 play requested sound
 * sound -1 is a quit command
 * 
 * Note:
 * 	Something not good will happed if a new play request 
 * arrives before exiting the handler
 */
void HandleRequest(int foo)
{
    int sound, timestamp;
    char *buffer;
    LList *tmp;
/*    
    signal(SIGUSR1, DoNothing);
 */
    read(InPipe,&sound,sizeof(sound));
    read(InPipe,&timestamp,sizeof(timestamp));    
    if (sound<0) {
	printf("exitting ASSound..\n");
	exit(0);
    }
    if ((clock()-timestamp)<PLAY_TIMEOUT)
      PlaySound(sound);
#if 0
    tmp = ckmalloc(sizeof(LList));
    tmp->data = sound;
    tmp->timestamp = clock();
    tmp->next = PlayQueue;
    if (PlayQueue==NULL) {
	OldestQueued = tmp;
	tmp->prev = NULL;	
    } else {
	PlayQueue->prev = tmp;	
    }    
    PlayQueue = tmp;
    signal(SIGUSR1, HandleRequest);
#endif    
}

/*
 * Program startup.
 * Arguments:
 * argv[1] - pipe for reading data 
 * argv[2] - the name of the sound player to be used. ``-'' indicates 
 * 	internal player.
 * argv[3]... - filenames of sound files with code n-3
 */
int main(int argc, char **argv) 
{
    int i;

    signal(SIGUSR1, HandleRequest);
    ProgName=argv[0];
    if (argc<4) {
	fprintf(stderr, "%s can only be started by an AfterStep module\n",
		ProgName);
	exit(1);
    }
    SoundPlayer=argv[2];
    if (SoundPlayer[0]=='-' && SoundPlayer[1]==0) {
	SoundPlayer=NULL;
	printf("%s:need a sound player.\n",ProgName);
    }
    SoundTable=ckmalloc(sizeof(SoundEntry *)*(argc-3));
    for(i=3; i<argc; i++) {
	Register(argv[i], i-3);
    }
    LastSound=i-3;
    InPipe = atoi(argv[1]);
    while(1) {
#if 0	
	LList *tmp;
	while (OldestQueued!=NULL) {
	    if ((clock()-OldestQueued->timestamp) < PLAY_TIMEOUT)
	      PlaySound(OldestQueued->data);
	    tmp = OldestQueued->prev;
	    free(OldestQueued);
	    OldestQueued=tmp;
	}
	pause();
#endif	
	HandleRequest(0);
    }    
}

