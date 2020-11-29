#ifndef FVWMLIB_FPLAY_H
#define FVWMLIB_FPLAY_H

#ifdef HAVE_RPLAY
#define USE_FPLAY 1
#else
#define USE_FPLAY 0
#endif

#if USE_FPLAY
#	include <rplay.h>
#	undef M_ERROR /* Solaris fix */
	typedef RPLAY FPLAY;
	typedef RPLAY_ATTRS FPLAY_ATTRS;

#	define FPLAY_DEFAULT_VALUE RPLAY_DEFAULT_VOLUME
#	define FPLAY_DEFAULT_PRIORITY RPLAY_DEFAULT_PRIORITY
#	define FPLAY_NULL RPLAY_NULL
#	define FPLAY_PLAY RPLAY_PLAY
#	define FPLAY_STOP RPLAY_STOP
#	define FPLAY_PAUSE RPLAY_PAUSE
#	define FPLAY_CONTINUE RPLAY_CONTINUE
#	define FPLAY_SOUND RPLAY_SOUND
#	define FPLAY_VOLUME RPLAY_VOLUME
#	define FPLAY_NSOUNDS RPLAY_NSOUNDS
#	define FPLAY_COMMAND RPLAY_COMMAND
#	define FPLAY_APPEND RPLAY_APPEND
#	define FPLAY_INSERT RPLAY_INSERT
#	define FPLAY_DELETE RPLAY_DELETE
#	define FPLAY_CHANGE RPLAY_CHANGE
#	define FPLAY_COUNT RPLAY_COUNT
#	define FPLAY_LIST_COUNT RPLAY_LIST_COUNT
#	define FPLAY_PRIORITY RPLAY_PRIORITY
#	define FPLAY_RANDOM_SOUND RPLAY_RANDOM_SOUND
#	define FPLAY_PING RPLAY_PING
#	define FPLAY_RPTP_SERVER RPLAY_RPTP_SERVER
#	define FPLAY_RPTP_SERVER_PORT RPLAY_RPTP_SERVER_PORT
#	define FPLAY_RPTP_SEARCH RPLAY_RPTP_SEARCH
#	define FPLAY_RPTP_FROM_SENDER RPLAY_RPTP_FROM_SENDER
#	define FPLAY_SAMPLE_RATE RPLAY_SAMPLE_RATE
#	define FPLAY_RESET RPLAY_RESET
#	define FPLAY_DONE RPLAY_DONE
#	define FPLAY_CLIENT_DATA RPLAY_CLIENT_DATA
#	define FPLAY_LIST_NAME RPLAY_LIST_NAME
#	define FPLAY_PUT RPLAY_PUT
#	define FPLAY_ID RPLAY_ID
#	define FPLAY_SEQUENCE RPLAY_SEQUENCE
#	define FPLAY_DATA RPLAY_DATA
#	define FPLAY_DATA_SIZE RPLAY_DATA_SIZE
#	define FPLAY_FORMAT_NONE RPLAY_FORMAT_NONE
#	define FPLAY_FORMAT_LINEAR_8 RPLAY_FORMAT_LINEAR_8
#	define FPLAY_FORMAT_ULINEAR_8 RPLAY_FORMAT_ULINEAR_8
#	define FPLAY_FORMAT_LINEAR_16 RPLAY_FORMAT_LINEAR_16
#	define FPLAY_FORMAT_ULINEAR_16	 RPLAY_FORMAT_ULINEAR_16
#	define FPLAY_FORMAT_ULAW RPLAY_FORMAT_ULAW
#	define FPLAY_FORMAT_G721 RPLAY_FORMAT_G721
#	define FPLAY_FORMAT_G723_3 RPLAY_FORMAT_G723_3
#	define FPLAY_FORMAT_G723_5 RPLAY_FORMAT_G723_5
#	define FPLAY_FORMAT_GSM RPLAY_FORMAT_GSM
#	define FPLAY_BIG_ENDIAN RPLAY_BIG_ENDIAN
#	define FPLAY_LITTLE_ENDIAN RPLAY_LITTLE_ENDIAN
#	define FPLAY_AUDIO_PORT_NONE RPLAY_AUDIO_PORT_NONE
#	define FPLAY_AUDIO_PORT_SPEAKER RPLAY_AUDIO_PORT_SPEAKER
#	define FPLAY_AUDIO_PORT_HEADPHONE RPLAY_AUDIO_PORT_HEADPHONE
#	define FPLAY_AUDIO_PORT_LINEOUT RPLAY_AUDIO_PORT_LINEOUT
#	define FPLAY_MIN_VOLUME RPLAY_MIN_VOLUME
#	define FPLAY_MAX_VOLUME RPLAY_MAX_VOLUME
#	define FPLAY_MIN_PRIORITY RPLAY_MIN_PRIORITY
#	define FPLAY_MAX_PRIORITY RPLAY_MAX_PRIORITY
#	define FPLAY_DEFAULT_VOLUME RPLAY_DEFAULT_VOLUME
#	define FPLAY_DEFAULT_PRIORITY RPLAY_DEFAULT_PRIORITY
#	define FPLAY_DEFAULT_COUNT RPLAY_DEFAULT_COUNT
#	define FPLAY_DEFAULT_LIST_COUNT RPLAY_DEFAULT_LIST_COUNT
#	define FPLAY_DEFAULT_RANDOM_SOUND RPLAY_DEFAULT_RANDOM_SOUND
#	define FPLAY_DEFAULT_SAMPLE_RATE RPLAY_DEFAULT_SAMPLE_RATE
#	define FPLAY_DEFAULT_OFFSET RPLAY_DEFAULT_OFFSET
#	define FPLAY_DEFAULT_BYTE_ORDER RPLAY_DEFAULT_BYTE_ORDER
#	define FPLAY_DEFAULT_CHANNELS RPLAY_DEFAULT_CHANNELS
#	define FPLAY_DEFAULT_BITS RPLAY_DEFAULT_BITS
#	define FPLAY_ERROR_NONE RPLAY_ERROR_NONE
#	define FPLAY_ERROR_MEMORY RPLAY_ERROR_MEMORY
#	define FPLAY_ERROR_HOST RPLAY_ERROR_HOST
#	define FPLAY_ERROR_CONNECT RPLAY_ERROR_CONNECT
#	define FPLAY_ERROR_SOCKET RPLAY_ERROR_SOCKET
#	define FPLAY_ERROR_WRITE RPLAY_ERROR_WRITE
#	define FPLAY_ERROR_CLOSE RPLAY_ERROR_CLOSE
#	define FPLAY_ERROR_PACKET_SIZE RPLAY_ERROR_PACKET_SIZE
#	define FPLAY_ERROR_BROADCAST RPLAY_ERROR_BROADCAST
#	define FPLAY_ERROR_ATTRIBUTE RPLAY_ERROR_ATTRIBUTE
#	define FPLAY_ERROR_COMMAND RPLAY_ERROR_COMMAND
#	define FPLAY_ERROR_INDEX RPLAY_ERROR_INDEX
#	define FPLAY_ERROR_MODIFIER RPLAY_ERROR_MODIFIER
#	define FRPTP_ERROR_NONE	RPTP_ERROR_NONE
#	define FRPTP_ERROR_MEMORY RPTP_ERROR_MEMORY
#	define FRPTP_ERROR_HOST RPTP_ERROR_HOST
#	define FRPTP_ERROR_CONNECT RPTP_ERROR_CONNECT
#	define FRPTP_ERROR_SOCKET RPTP_ERROR_SOCKET
#	define FRPTP_ERROR_OPEN RPTP_ERROR_OPEN
#	define FRPTP_ERROR_READ RPTP_ERROR_READ
#	define FRPTP_ERROR_WRITE RPTP_ERROR_WRITE
#	define FRPTP_ERROR_PING RPTP_ERROR_PING
#	define FRPTP_ERROR_TIMEOUT RPTP_ERROR_TIMEOUT
#	define FRPTP_ERROR_PROTOCOL RPTP_ERROR_PROTOCOL
#	define FRPTP_ERROR RPTP_ERROR
#	define FRPTP_OK RPTP_OK
#	define FRPTP_TIMEOUT RPTP_TIMEOUT
#	define FRPTP_NOTIFY RPTP_NOTIFY
#	define FOLD_RPLAY_PLAY OLD_RPLAY_PLAY
#	define FOLD_RPLAY_STOP OLD_RPLAY_STOP
#	define FOLD_RPLAY_PAUSE OLD_RPLAY_PAUSE
#	define FOLD_RPLAY_CONTINUE OLD_RPLAY_CONTINUE
#	define FRPTP_ASYNC_READ RPTP_ASYNC_READ
#	define FRPTP_ASYNC_WRITE RPTP_ASYNC_WRITE
#	define FRPTP_ASYNC_RAW RPTP_ASYNC_RAW
#	define FRPTP_ASYNC_ENABLE RPTP_ASYNC_ENABLE
#	define FRPTP_ASYNC_DISABLE RPTP_ASYNC_DISABLE
#	define FRPTP_EVENT_OK RPTP_EVENT_OK
#	define FRPTP_EVENT_ERROR RPTP_EVENT_ERROR
#	define FRPTP_EVENT_TIMEOUT RPTP_EVENT_TIMEOUT
#	define FRPTP_EVENT_OTHER RPTP_EVENT_OTHER
#	define FRPTP_EVENT_CONTINUE RPTP_EVENT_CONTINUE
#	define FRPTP_EVENT_DONE RPTP_EVENT_DONE
#	define FRPTP_EVENT_PAUSE RPTP_EVENT_PAUSE
#	define FRPTP_EVENT_PLAY RPTP_EVENT_PLAY
#	define FRPTP_EVENT_SKIP RPTP_EVENT_SKIP
#	define FRPTP_EVENT_STATE RPTP_EVENT_STATE
#	define FRPTP_EVENT_STOP RPTP_EVENT_STOP
#	define FRPTP_EVENT_VOLUME RPTP_EVENT_VOLUME
#	define FRPTP_EVENT_CLOSE RPTP_EVENT_CLOSE
#	define FRPTP_EVENT_FLOW RPTP_EVENT_FLOW
#	define FRPTP_EVENT_MODIFY RPTP_EVENT_MODIFY
#	define FRPTP_EVENT_LEVEL RPTP_EVENT_LEVEL
#	define FRPTP_EVENT_POSITION RPTP_EVENT_POSITION
#	define FRPTP_EVENT_ALL RPTP_EVENT_ALL
#	define FRPTP_MAX_ARGS RPTP_MAX_ARGS
#	define FRPTP_MAX_LINE RPTP_MAX_LINE
#	define FPLAY_PORT RPLAY_PORT
#	define FRPTP_PORT RPTP_PORT
#	define FOLD_RPLAY_PORT OLD_RPLAY_PORT
#	define FOLD_RPTP_PORT OLD_RPTP_PORT
#	define FPLAY_PACKET_ID	RPLAY_PACKET_ID

#	define Fplay_errno rplay_errno
#	define Frptp_errno rptp_errno

#	define Fplay(a,b) rplay(a,b)
#	define Fplay_create(a) rplay_create(a)
#	define Fplay_perror(a) rplay_perror(a)
/* variadict macros appeared in C99, so we can't use them */
#	define Fplay_set rplay_set
#	define Fplay_get rplay_get
#	define Frptp_putline rptp_putline
#	define Frptp_async_putline rptp_async_putline
#	define Fplay_destroy(a) rplay_destroy(a)
#	define Fplay_default_host() rplay_default_host()
#	define Fplay_display(a) rplay_display(a)
#	define Fplay_host(a,b) rplay_host(a,b)
#	define Fplay_host_volume(a,b,c) rplay_host_volume(a,b,c)
#	define Fplay_local(a) rplay_local(a)
#	define Fplay_open(a) rplay_open(a)
#	define Fplay_open_default() rplay_open_default()
#	define Fplay_open_display() rplay_open_display()
#	define Fplay_open_port(a,b) rplay_open_port(a,b)
#	define Fplay_open_sockaddr_in(a) rplay_open_sockaddr(a)
#	define Fplay_ping(a) rplay_ping(a)
#	define Fplay_ping_sockaddr_in(a) rplay_ping_sockaddr_in(a)
#	define Fplay_ping_sockfd(a) rplay_ping_sockfd(a)
#	define Fplay_close(a) rplay_close(a)
#	define Fplay_sound(a,b) rplay_sound(a,b)
#	define Fplay_default(a) rplay_default(a)
#	define Fplay_convert(a) rplay_convert(a)
#	define Fplay_pack(a) rplay_pack(a)
#	define Fplay_unpack(a) rplay_unpack(a)
#	define Frptp_open(a,b,c,d) rptp_open(a,b,c,d)
#	define Frptp_read(a,b,c) rptp_read(a,b,c)
#	define Frptp_write(a,b,c) rptp_write(a,b,c)
#	define Frptp_close(a) rptp_close(a)
#	define Frptp_perror(a) rptp_perror(a)
#	define Frptp_getline(a,b,c) rptp_getline(a,b,c)
#	define Frptp_command(a,b,c,d) rptp_command(a,b,c,d)
#	define Frptp_parse(a,b) rptp_parse(a,b)
#	define Frptp_async_write(a,b,c,d) rptp_async_write(a,b,c,d)
#	define Frptp_async_register(a,b,c) rptp_async_register(a,b,c)
#	define Frptp_async_notify(a,b,c) rptp_async_notify(a,b,c)
#	define Frptp_async_process(a,b) rptp_async_process(a,b)
#	define Frptp_main_loop() rptp_main_loop()
#	define Frptp_stop_main_loop(a) rptp_stop_main_loop(a)
#else
	typedef void FPLAY;
	typedef void FPLAY_ATTRS;
#	define FPLAY_DEFAULT_VALUE 0
#	define FPLAY_DEFAULT_PRIORITY 0
#	define FPLAY_NULL 0
#	define FPLAY_PLAY 0
#	define FPLAY_STOP 0
#	define FPLAY_PAUSE 0
#	define FPLAY_CONTINUE 0
#	define FPLAY_SOUND 0
#	define FPLAY_VOLUME 0
#	define FPLAY_NSOUNDS 0
#	define FPLAY_COMMAND 0
#	define FPLAY_APPEND 0
#	define FPLAY_INSERT 0
#	define FPLAY_DELETE 0
#	define FPLAY_CHANGE 0
#	define FPLAY_COUNT 0
#	define FPLAY_LIST_COUNT 0
#	define FPLAY_PRIORITY 0
#	define FPLAY_RANDOM_SOUND 0
#	define FPLAY_PING 0
#	define FPLAY_RPTP_SERVER 0
#	define FPLAY_RPTP_SERVER_PORT 0
#	define FPLAY_RPTP_SEARCH 0
#	define FPLAY_RPTP_FROM_SENDER 0
#	define FPLAY_SAMPLE_RATE 0
#	define FPLAY_RESET 0
#	define FPLAY_DONE 0
#	define FPLAY_CLIENT_DATA 0
#	define FPLAY_LIST_NAME 0
#	define FPLAY_PUT 0
#	define FPLAY_ID 0
#	define FPLAY_SEQUENCE 0
#	define FPLAY_DATA 0
#	define FPLAY_DATA_SIZE 0
#	define FPLAY_FORMAT_NONE 0
#	define FPLAY_FORMAT_LINEAR_8 0
#	define FPLAY_FORMAT_ULINEAR_8 0
#	define FPLAY_FORMAT_LINEAR_16 0
#	define FPLAY_FORMAT_ULINEAR_16 0
#	define FPLAY_FORMAT_ULAW 0
#	define FPLAY_FORMAT_G721 0
#	define FPLAY_FORMAT_G723_3 0
#	define FPLAY_FORMAT_G723_5 0
#	define FPLAY_FORMAT_GSM 0
#	define FPLAY_BIG_ENDIAN 0
#	define FPLAY_LITTLE_ENDIAN 0
#	define FPLAY_AUDIO_PORT_NONE 0
#	define FPLAY_AUDIO_PORT_SPEAKER 0
#	define FPLAY_AUDIO_PORT_HEADPHONE 0
#	define FPLAY_AUDIO_PORT_LINEOUT 0
#	define FPLAY_MIN_VOLUME 0
#	define FPLAY_MAX_VOLUME 0
#	define FPLAY_MIN_PRIORITY 0
#	define FPLAY_MAX_PRIORITY 0
#	define FPLAY_DEFAULT_VOLUME 0
#	define FPLAY_DEFAULT_PRIORITY 0
#	define FPLAY_DEFAULT_COUNT 0
#	define FPLAY_DEFAULT_LIST_COUNT 0
#	define FPLAY_DEFAULT_RANDOM_SOUND 0
#	define FPLAY_DEFAULT_SAMPLE_RATE 0
#	define FPLAY_DEFAULT_OFFSET 0
#	define FPLAY_DEFAULT_BYTE_ORDER 0
#	define FPLAY_DEFAULT_CHANNELS 0
#	define FPLAY_DEFAULT_BITS 0
#	define FPLAY_ERROR_NONE 0
#	define FPLAY_ERROR_MEMORY 0
#	define FPLAY_ERROR_HOST 0
#	define FPLAY_ERROR_CONNECT 0
#	define FPLAY_ERROR_SOCKET 0
#	define FPLAY_ERROR_WRITE 0
#	define FPLAY_ERROR_CLOSE 0
#	define FPLAY_ERROR_PACKET_SIZE 0
#	define FPLAY_ERROR_BROADCAST 0
#	define FPLAY_ERROR_ATTRIBUTE 0
#	define FPLAY_ERROR_COMMAND 0
#	define FPLAY_ERROR_INDEX 0
#	define FPLAY_ERROR_MODIFIER 0
#	define FRPTP_ERROR_NONE	0
#	define FRPTP_ERROR_MEMORY 0
#	define FRPTP_ERROR_HOST 0
#	define FRPTP_ERROR_CONNECT 0
#	define FRPTP_ERROR_SOCKET 0
#	define FRPTP_ERROR_OPEN 0
#	define FRPTP_ERROR_READ 0
#	define FRPTP_ERROR_WRITE 0
#	define FRPTP_ERROR_PING 0
#	define FRPTP_ERROR_TIMEOUT 0
#	define FRPTP_ERROR_PROTOCOL 0
#	define FRPTP_ERROR 0
#	define FRPTP_OK 0
#	define FRPTP_TIMEOUT 0
#	define FRPTP_NOTIFY 0
#	define FOLD_RPLAY_PLAY 0
#	define FOLD_RPLAY_STOP 0
#	define FOLD_RPLAY_PAUSE 0
#	define FOLD_RPLAY_CONTINUE 0
#	define FRPTP_ASYNC_READ 0
#	define FRPTP_ASYNC_WRITE 0
#	define FRPTP_ASYNC_RAW 0
#	define FRPTP_ASYNC_ENABLE 0
#	define FRPTP_ASYNC_DISABLE 0
#	define FRPTP_EVENT_OK 0
#	define FRPTP_EVENT_ERROR 0
#	define FRPTP_EVENT_TIMEOUT 0
#	define FRPTP_EVENT_OTHER 0
#	define FRPTP_EVENT_CONTINUE 0
#	define FRPTP_EVENT_DONE 0
#	define FRPTP_EVENT_PAUSE 0
#	define FRPTP_EVENT_PLAY 0
#	define FRPTP_EVENT_SKIP 0
#	define FRPTP_EVENT_STATE 0
#	define FRPTP_EVENT_STOP 0
#	define FRPTP_EVENT_VOLUME 0
#	define FRPTP_EVENT_CLOSE 0
#	define FRPTP_EVENT_FLOW 0
#	define FRPTP_EVENT_MODIFY 0
#	define FRPTP_EVENT_LEVEL 0
#	define FRPTP_EVENT_POSITION 0
#	define FRPTP_EVENT_ALL 0
#	define FRPTP_MAX_ARGS 0
#	define FRPTP_MAX_LINE 0
#	define FPLAY_PORT 0
#	define FRPTP_PORT 0
#	define FOLD_RPLAY_PORT 0
#	define FOLD_RPTP_PORT 0
#	define FPLAY_PACKET_ID 0

#	define Fplay_errno 0
#	define Frptp_errno 0


#	define Fplay(a,b) 0
#	define Fplay_create(a) 0
#	define Fplay_perror(a)
/* variadict macros appeared in C99, so we can't use them. */
	static void Fplay_set(FPLAY *a, ...) {}

/*  TA:  20100403:  These aren't being used anymore. */
/*	static int Fplay_get(FPLAY *a, ...) {return 0;}
	static int Frptp_putline(int a, ...) {return 0;}
*/
#	define Frptp_async_putline Frptp_putline
/*
#	define Fplay_set(...)
#	define Fplay_get(...) 0
#	define Frptp_putline(...) 0
#	define Frptp_async_putline(...) 0
*/
#	define Fplay_destroy(a)
#	define Fplay_default_host() ""
#	define Fplay_display(a) 0
#	define Fplay_host(a,b) 0
#	define Fplay_host_volume(a,b,c) 0
#	define Fplay_local(a) 0
#	define Fplay_open(a) -1
#	define Fplay_open_default() -1
#	define Fplay_open_display() -1
#	define Fplay_open_port(a,b) -1
#	define Fplay_open_sockaddr_in(a) -1
#	define Fplay_close(a) 0
#	define Fplay_ping(a) 0
#	define Fplay_ping_sockaddr_in(a) 0
#	define Fplay_ping_sockfd(a) 0
#	define Fplay_sound(a,b) 0
#	define Fplay_default(a) 0
#	define Fplay_convert(a) NULL
#	define Fplay_pack(a) 0
#	define Fplay_unpack(a) 0
#	define Frptp_open(a,b,c,d) -1
#	define Frptp_read(a,b,c) 0
#	define Frptp_write(a,b,c) 0
#	define Frptp_close(a) 0
#	define Frptp_perror(a)
#	define Frptp_getline(a,b,c) 0
#	define Frptp_command(a,b,c,d) 0
#	define Frptp_parse(a,b) NULL
#	define Frptp_async_write(a,b,c,d) 0
#	define Frptp_async_register(a,b,c)
#	define Frptp_async_notify(a,b,c)
#	define Frptp_async_process(a,b)
#	define Frptp_main_loop() 0
#	define Frptp_stop_main_loop(a)
#endif

#endif
