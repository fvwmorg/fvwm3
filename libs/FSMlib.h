/* -*-c-*- */

#ifndef FSMlib_H
#define FSMlib_H

/* ---------------------------- included header files ---------------------- */
#include "config.h"

#ifdef SESSION
#define SessionSupport 1
#else
#define SessionSupport 0
#endif

#if SessionSupport
#include <X11/SM/SMlib.h>
#include <X11/ICE/ICEutil.h>
#endif

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

#if SessionSupport

/* SM.h */
#define FSmProtoMajor	SmProtoMajor
#define FSmProtoMinor	SmProtoMinor

#define FSmInteractStyleNone	SmInteractStyleNone
#define FSmInteractStyleErrors	SmInteractStyleErrors
#define FSmInteractStyleAny	SmInteractStyleAny

#define FSmDialogError		SmDialogError
#define FSmDialogNormal		SmDialogNormal

#define FSmSaveGlobal	SmSaveGlobal
#define FSmSaveLocal	SmSaveLocal
#define FSmSaveBoth	SmSaveBoth

#define FSmRestartIfRunning	SmRestartIfRunning
#define FSmRestartAnyway	SmRestartAnyway
#define FSmRestartImmediately	SmRestartImmediately
#define FSmRestartNever		SmRestartNever

#define FSmCloneCommand		SmCloneCommand
#define FSmCurrentDirectory	SmCurrentDirectory
#define FSmDiscardCommand	SmDiscardCommand
#define FSmEnvironment		SmEnvironment
#define FSmProcessID		SmProcessID
#define FSmProgram		SmProgram
#define FSmRestartCommand	SmRestartCommand
#define FSmResignCommand	SmResignCommand
#define FSmRestartStyleHint	SmRestartStyleHint
#define FSmShutdownCommand	SmShutdownCommand
#define FSmUserID		SmUserID

#define FSmCARD8		SmCARD8
#define FSmARRAY8		SmARRAY8
#define FSmLISTofARRAY8		SmLISTofARRAY8

#define FSM_Error			SM_Error
#define FSM_RegisterClient 		SM_RegisterClient
#define FSM_RegisterClientReply 	SM_RegisterClientReply
#define FSM_SaveYourself 		SM_SaveYourself
#define FSM_SaveYourselfRequest 	SM_SaveYourselfRequest
#define FSM_InteractRequest 		SM_InteractRequest
#define FSM_Interact 			SM_Interact
#define FSM_InteractDone 		SM_InteractDone
#define FSM_SaveYourselfDone 		SM_SaveYourselfDone
#define FSM_Die 			SM_Die
#define FSM_ShutdownCancelled		SM_ShutdownCancelled
#define FSM_CloseConnection 		SM_CloseConnection
#define FSM_SetProperties 		SM_SetProperties
#define FSM_DeleteProperties 		SM_DeleteProperties
#define FSM_GetProperties 		SM_GetProperties
#define FSM_PropertiesReply 		SM_PropertiesReply
#define FSM_SaveYourselfPhase2Request	SM_SaveYourselfPhase2Request
#define FSM_SaveYourselfPhase2		SM_SaveYourselfPhase2
#define FSM_SaveComplete		SM_SaveComplete

/* ICE.h */
#define FIceProtoMajor IceProtoMajor
#define FIceProtoMinor IceProtoMinor

#define FIceLSBfirst		IceLSBfirst
#define FIceMSBfirst		IceMSBfirst

#define FICE_Error 		ICE_Error
#define FICE_ByteOrder		ICE_ByteOrder
#define FICE_ConnectionSetup	ICE_ConnectionSetup
#define FICE_AuthRequired	ICE_AuthRequired
#define FICE_AuthReply 		ICE_AuthReply
#define FICE_AuthNextPhase	ICE_AuthNextPhase
#define FICE_ConnectionReply	ICE_ConnectionReply
#define FICE_ProtocolSetup	ICE_ProtocolSetup
#define FICE_ProtocolReply	ICE_ProtocolReply
#define FICE_Ping		ICE_Ping
#define FICE_PingReply		ICE_PingReply
#define FICE_WantToClose	ICE_WantToClose
#define FICE_NoClose		ICE_NoClose

#define FIceCanContinue		IceCanContinue
#define FIceFatalToProtocol	IceFatalToProtocol
#define FIceFatalToConnection	IceFatalToConnection

#define FIceBadMinor	IceBadMinor
#define FIceBadState	IceBadState
#define FIceBadLength	IceBadLength
#define FIceBadValue	IceBadValue

#define FIceBadMajor			IceBadMajor
#define FIceNoAuth			IceNoAuth
#define FIceNoVersion			IceNoVersion
#define FIceSetupFailed			IceSetupFailed
#define FIceAuthRejected		IceAuthRejected
#define FIceAuthFailed			IceAuthFailed
#define FIceProtocolDuplicate		IceProtocolDuplicate
#define FIceMajorOpcodeDuplicate	IceMajorOpcodeDuplicate
#define FIceUnknownProtocol		IceUnknownProtocol

/* SMlib.h */
#define FSmsRegisterClientProcMask		SmsRegisterClientProcMask
#define FSmsInteractRequestProcMask		SmsInteractRequestProcMask
#define FSmsInteractDoneProcMask		SmsInteractDoneProcMask
#define FSmsSaveYourselfRequestProcMask  	SmsSaveYourselfRequestProcMask
#define FSmsSaveYourselfP2RequestProcMask	SmsSaveYourselfP2RequestProcMask
#define FSmsSaveYourselfDoneProcMask		SmsSaveYourselfDoneProcMask
#define FSmsCloseConnectionProcMask		SmsCloseConnectionProcMask
#define FSmsSetPropertiesProcMask		SmsSetPropertiesProcMask
#define FSmsDeletePropertiesProcMask		SmsDeletePropertiesProcMask
#define FSmsGetPropertiesProcMask		SmsGetPropertiesProcMask

#define FSmcSaveYourselfProcMask	SmcSaveYourselfProcMask
#define FSmcDieProcMask			SmcDieProcMask
#define FSmcSaveCompleteProcMask	SmcSaveCompleteProcMask
#define FSmcShutdownCancelledProcMask	SmcShutdownCancelledProcMask

/* ICEutil.h */
#define FIceAuthLockSuccess IceAuthLockSuccess
#define FIceAuthLockError IceAuthLockError
#define FIceAuthLockTimeout IceAuthLockTimeout

#else /* !SessionSupport */

/* SM.h */
#define FSmProtoMajor	1
#define FSmProtoMinor	0

#define FSmInteractStyleNone	0
#define FSmInteractStyleErrors	1
#define FSmInteractStyleAny	2

#define FSmDialogError		0
#define FSmDialogNormal		1

#define FSmSaveGlobal	0
#define FSmSaveLocal	1
#define FSmSaveBoth	2

#define FSmRestartIfRunning	0
#define FSmRestartAnyway	1
#define FSmRestartImmediately	2
#define FSmRestartNever		3

#define FSmCloneCommand		"CloneCommand"
#define FSmCurrentDirectory	"CurrentDirectory"
#define FSmDiscardCommand	"DiscardCommand"
#define FSmEnvironment		"Environment"
#define FSmProcessID		"ProcessID"
#define FSmProgram		"Program"
#define FSmRestartCommand	"RestartCommand"
#define FSmResignCommand	"ResignCommand"
#define FSmRestartStyleHint	"RestartStyleHint"
#define FSmShutdownCommand	"ShutdownCommand"
#define FSmUserID		"UserID"

#define FSmCARD8		"CARD8"
#define FSmARRAY8		"ARRAY8"
#define FSmLISTofARRAY8		"LISTofARRAY8"

#define FSM_Error			0
#define FSM_RegisterClient 		1
#define FSM_RegisterClientReply 	2
#define FSM_SaveYourself 		3
#define FSM_SaveYourselfRequest 	4
#define FSM_InteractRequest 		5
#define FSM_Interact 			6
#define FSM_InteractDone 		7
#define FSM_SaveYourselfDone 		8
#define FSM_Die 			9
#define FSM_ShutdownCancelled		10
#define FSM_CloseConnection 		11
#define FSM_SetProperties 		12
#define FSM_DeleteProperties 		13
#define FSM_GetProperties 		14
#define FSM_PropertiesReply 		15
#define FSM_SaveYourselfPhase2Request	16
#define FSM_SaveYourselfPhase2		17
#define FSM_SaveComplete		18

/* ICE.h */
#define FIceProtoMajor 1
#define FIceProtoMinor 0

#define FIceLSBfirst		0
#define FIceMSBfirst		1

#define FICE_Error 		0
#define FICE_ByteOrder		1
#define FICE_ConnectionSetup	2
#define FICE_AuthRequired	3
#define FICE_AuthReply 		4
#define FICE_AuthNextPhase	5
#define FICE_ConnectionReply	6
#define FICE_ProtocolSetup	7
#define FICE_ProtocolReply	8
#define FICE_Ping		9
#define FICE_PingReply		10
#define FICE_WantToClose	11
#define FICE_NoClose		12

#define FIceCanContinue		0
#define FIceFatalToProtocol	1
#define FIceFatalToConnection	2

#define FIceBadMinor	0x8000
#define FIceBadState	0x8001
#define FIceBadLength	0x8002
#define FIceBadValue	0x8003

#define FIceBadMajor			0
#define FIceNoAuth			1
#define FIceNoVersion			2
#define FIceSetupFailed			3
#define FIceAuthRejected		4
#define FIceAuthFailed			5
#define FIceProtocolDuplicate		6
#define FIceMajorOpcodeDuplicate	7
#define FIceUnknownProtocol		8

/* SMlib.h */
#define FSmsRegisterClientProcMask		(1L << 0)
#define FSmsInteractRequestProcMask		(1L << 1)
#define FSmsInteractDoneProcMask		(1L << 2)
#define FSmsSaveYourselfRequestProcMask  	(1L << 3)
#define FSmsSaveYourselfP2RequestProcMask	(1L << 4)
#define FSmsSaveYourselfDoneProcMask		(1L << 5)
#define FSmsCloseConnectionProcMask		(1L << 6)
#define FSmsSetPropertiesProcMask		(1L << 7)
#define FSmsDeletePropertiesProcMask		(1L << 8)
#define FSmsGetPropertiesProcMask		(1L << 9)

#define FSmcSaveYourselfProcMask	(1L << 0)
#define FSmcDieProcMask			(1L << 1)
#define FSmcSaveCompleteProcMask	(1L << 2)
#define FSmcShutdownCancelledProcMask	(1L << 3)

/* ICEutil.h */
#define IceAuthLockSuccess	0
#define IceAuthLockError	1
#define IceAuthLockTimeout	2

#endif /* SessionSupport */

/* ---------------------------- type definitions --------------------------- */

#if SessionSupport

/* ICElib.h */
typedef IcePointer FIcePointer;

#define FIcePoAuthHaveReply IcePoAuthHaveReply
#define FIcePoAuthRejected IcePoAuthRejected
#define FIcePoAuthFailed IcePoAuthFailed
#define FIcePoAuthDoneCleanup IcePoAuthDoneCleanup
typedef IcePoAuthStatus FIcePoAuthStatus;

#define FIcePaAuthContinue IcePaAuthContinue
#define FIcePaAuthAccepted IcePaAuthAccepted
#define FIcePaAuthRejected IcePaAuthRejected
#define FIcePaAuthFailed IcePaAuthFailed
typedef IcePaAuthStatus FIcePaAuthStatus;

#define FIceConnectPending IceConnectPending
#define FIceConnectAccepted IceConnectAccepted
#define FIceConnectRejected IceConnectRejected
#define FIceConnectIOError IceConnectIOError
typedef IceConnectStatus FIceConnectStatus;

#define FIceProtocolSetupSuccess IceProtocolSetupSuccess
#define FIceProtocolSetupFailure IceProtocolSetupFailure
#define FIceProtocolSetupIOError IceProtocolSetupIOError
#define FIceProtocolAlreadyActive IceProtocolAlreadyActive
typedef IceProtocolSetupStatus FIceProtocolSetupStatus;

#define FIceAcceptSuccess IceAcceptSuccess
#define FIceAcceptFailure IceAcceptFailure
#define FIceAcceptBadMalloc IceAcceptBadMalloc
typedef IceAcceptStatus FIceAcceptStatus;

#define FIceClosedNow IceClosedNow
#define FIceClosedASAP IceClosedASAP
#define FIceConnectionInUse IceConnectionInUse
#define FIceStartedShutdownNegotiation IceStartedShutdownNegotiation
typedef IceCloseStatus FIceCloseStatus;

#define FIceProcessMessagesSuccess IceProcessMessagesSuccess
#define FIceProcessMessagesIOError IceProcessMessagesIOError
#define FIceProcessMessagesConnectionClosed IceProcessMessagesConnectionClosed
typedef IceProcessMessagesStatus FIceProcessMessagesStatus;

typedef IceReplyWaitInfo FIceReplyWaitInfo;
typedef IceConn FIceConn;
typedef IceListenObj FIceListenObj;

typedef IceWatchProc FIceWatchProc;
typedef IcePoProcessMsgProc FIcePoProcessMsgProc;
typedef IcePaProcessMsgProc FIcePaProcessMsgProc;
typedef IcePoVersionRec FIcePoVersionRec;
typedef IcePaVersionRec FIcePaVersionRec;
typedef IcePoAuthProc FIcePoAuthProc;
typedef IcePaAuthProc FIcePaAuthProc;

typedef IceHostBasedAuthProc FIceHostBasedAuthProc;
typedef IceProtocolSetupProc FIceProtocolSetupProc;
typedef IceProtocolActivateProc FIceProtocolActivateProc;
typedef IceIOErrorProc FIceIOErrorProc;
typedef IcePingReplyProc FIcePingReplyProc;
typedef IceErrorHandler FIceErrorHandler;
typedef IceIOErrorHandler FIceIOErrorHandler;

/* SMlib.h */
typedef SmPointer FSmPointer;

typedef SmcConn FSmcConn;
typedef SmsConn FSmsConn;

typedef SmPropValue FSmPropValue;

typedef SmProp FSmProp;

#define FSmcClosedNow SmcClosedNow
#define FSmcClosedASAP FSmcClosedASAP
#define FSmcConnectionInUse FSmcConnectionInUse
typedef SmcCloseStatus FSmcCloseStatus;

typedef SmcSaveYourselfProc FSmcSaveYourselfProc;
typedef SmcSaveYourselfPhase2Proc FSmcSaveYourselfPhase2Proc;
typedef SmcInteractProc FSmcInteractProc;
typedef SmcDieProc FSmcDieProc;
typedef SmcShutdownCancelledProc  FSmcShutdownCancelledProc;
typedef SmcSaveCompleteProc FSmcSaveCompleteProc;
typedef SmcPropReplyProc FSmcPropReplyProc;

typedef SmcCallbacks FSmcCallbacks;

typedef SmsRegisterClientProc FSmsRegisterClientProc;
typedef SmsInteractRequestProc FSmsInteractRequestProc;
typedef SmsInteractDoneProc FSmsInteractDoneProc;
typedef SmsSaveYourselfRequestProc FSmsSaveYourselfRequestProc;
typedef SmsSaveYourselfPhase2RequestProc FSmsSaveYourselfPhase2RequestProc;
typedef SmsSaveYourselfDoneProc FSmsSaveYourselfDoneProc;
typedef SmsCloseConnectionProc FSmsCloseConnectionProc;
typedef SmsSetPropertiesProc FSmsSetPropertiesProc;
typedef SmsDeletePropertiesProc FSmsDeletePropertiesProc;
typedef SmsGetPropertiesProc FSmsGetPropertiesProc;

typedef SmsCallbacks FSmsCallbacks;

typedef SmsNewClientProc FSmsNewClientProc;
typedef SmcErrorHandler FSmcErrorHandler;
typedef SmsErrorHandler FSmsErrorHandler;

/* ICEutil.h */
typedef IceAuthFileEntry FIceAuthFileEntry;
typedef IceAuthDataEntry FIceAuthDataEntry;

#else  /* !SessionSupport */

#ifdef __STDC__
typedef void *FIcePointer;
#else
typedef char *FIcePointer;
#endif


/* ICElib.h */
typedef enum {
	FIcePoAuthHaveReply, FIcePoAuthRejected, FIcePoAuthFailed,
	FIcePoAuthDoneCleanup
} FIcePoAuthStatus;
typedef enum {
	FIcePaAuthContinue, FIcePaAuthAccepted, FIcePaAuthRejected,
	FIcePaAuthFailed
} FIcePaAuthStatus;
typedef enum {
    FIceConnectPending,FIceConnectAccepted,FIceConnectRejected,FIceConnectIOError
} FIceConnectStatus;
typedef enum {
    FIceProtocolSetupSuccess,FIceProtocolSetupFailure,FIceProtocolSetupIOError,
    FIceProtocolAlreadyActive
} FIceProtocolSetupStatus;
typedef enum {
    FIceAcceptSuccess,FIceAcceptFailure,FIceAcceptBadMalloc
} FIceAcceptStatus;
typedef enum {
    FIceClosedNow,FIceClosedASAP,FIceConnectionInUse,
    FIceStartedShutdownNegotiation
} FIceCloseStatus;
typedef enum {
    FIceProcessMessagesSuccess,FIceProcessMessagesIOError,
    FIceProcessMessagesConnectionClosed
} FIceProcessMessagesStatus;
typedef struct {
	unsigned long sequence_of_request; int major_opcode_of_request;
	int minor_opcode_of_request; FIcePointer reply;
} FIceReplyWaitInfo;
typedef void *FIceConn;
typedef void *FIceListenObj;
typedef void (*FIceWatchProc) (
#ifdef __STDC__
    FIceConn, FIcePointer, Bool, FIcePointer *
#endif
);
typedef void (*FIcePoProcessMsgProc) (
#ifdef __STDC__
	FIceConn, FIcePointer, int, unsigned long, Bool, FIceReplyWaitInfo *,
	Bool *
#endif
);
typedef void (*FIcePaProcessMsgProc) (
#ifdef __STDC__
	FIceConn, FIcePointer, int, unsigned long, Bool
#endif
);
typedef struct {
    int major_version; int minor_version; FIcePoProcessMsgProc process_msg_proc;
} FIcePoVersionRec;
typedef struct {
    int major_version; int minor_version; FIcePaProcessMsgProc process_msg_proc;
} FIcePaVersionRec;
typedef FIcePoAuthStatus (*FIcePoAuthProc) (
#ifdef __STDC__
	FIceConn, FIcePointer *, Bool, Bool, int, FIcePointer, int *,
	FIcePointer *, char **
#endif
);
typedef FIcePaAuthStatus (*FIcePaAuthProc) (
#ifdef __STDC__
	FIceConn, FIcePointer *, Bool, int, FIcePointer, int *,
	FIcePointer *, char **
#endif
);
typedef Bool (*FIceHostBasedAuthProc) (
#ifdef __STDC__
    char *
#endif
);
typedef Status (*FIceProtocolSetupProc) (
#ifdef __STDC__
	FIceConn, int, int, char *, char *, FIcePointer *, char **
#endif
);
typedef void (*FIceProtocolActivateProc) (
#ifdef __STDC__
    FIceConn, FIcePointer
#endif
);
typedef void (*FIceIOErrorProc) (
#ifdef __STDC__
    FIceConn
#endif
);
typedef void (*FIcePingReplyProc) (
#ifdef __STDC__
    FIceConn, FIcePointer
#endif
);
typedef void (*FIceErrorHandler) (
#ifdef __STDC__
    FIceConn, Bool, int, unsigned long, int, int, FIcePointer
#endif
);
typedef void (*FIceIOErrorHandler) (
#ifdef __STDC__
    FIceConn 		/* iceConn */
#endif
);

/* SMlib.h */
typedef FIcePointer FSmPointer;

typedef void *FSmcConn;
typedef void *FSmsConn;

typedef struct { int length; FSmPointer value;
} FSmPropValue;
typedef struct { char *name; char *type; int num_vals; FSmPropValue *vals;
} FSmProp;

typedef enum {
    FSmcClosedNow,
    FSmcClosedASAP,
    FSmcConnectionInUse
} FSmcCloseStatus;

typedef void (*FSmcSaveYourselfProc) (
#ifdef __STDC__
    FSmcConn, FSmPointer, int, Bool, int, Bool
#endif
);
typedef void (*FSmcSaveYourselfPhase2Proc) (
#ifdef __STDC__
    FSmcConn, FSmPointer
#endif
);
typedef void (*FSmcInteractProc) (
#ifdef __STDC__
    FSmcConn, FSmPointer
#endif
);
typedef void (*FSmcDieProc) (
#ifdef __STDC__
    FSmcConn, FSmPointer
#endif
);
typedef void (*FSmcShutdownCancelledProc) (
#ifdef __STDC__
    FSmcConn, FSmPointer
#endif
);
typedef void (*FSmcSaveCompleteProc) (
#ifdef __STDC__
    FSmcConn, FSmPointer
#endif
);
typedef void (*FSmcPropReplyProc) (
#ifdef __STDC__
    FSmcConn, FSmPointer, int, FSmProp **
#endif
);

typedef struct {
    struct {
	FSmcSaveYourselfProc	 callback;
	FSmPointer		 client_data;
    } save_yourself;
    struct {
	FSmcDieProc		 callback;
	FSmPointer		 client_data;
    } die;
    struct {
	FSmcSaveCompleteProc	 callback;
	FSmPointer		 client_data;
    } save_complete;
    struct {
	FSmcShutdownCancelledProc callback;
	FSmPointer		 client_data;
    } shutdown_cancelled;
} FSmcCallbacks;

typedef Status (*FSmsRegisterClientProc) (
#ifdef __STDC__
	FSmsConn, FSmPointer, char *
#endif
);
typedef void (*FSmsInteractRequestProc) (
#ifdef __STDC__
	FSmsConn, FSmPointer, int
#endif
);
typedef void (*FSmsInteractDoneProc) (
#ifdef __STDC__
	FSmsConn, FSmPointer, Bool
#endif
);
typedef void (*FSmsSaveYourselfRequestProc) (
#ifdef __STDC__
	FSmsConn, FSmPointer, int, Bool, int, Bool, Bool
#endif
);
typedef void (*FSmsSaveYourselfPhase2RequestProc) (
#ifdef __STDC__
	FSmsConn, FSmPointer
#endif
);
typedef void (*FSmsSaveYourselfDoneProc) (
#ifdef __STDC__
	FSmsConn, FSmPointer, Bool
#endif
);
typedef void (*FSmsCloseConnectionProc) (
#ifdef __STDC__
	FSmsConn, FSmPointer, int, char **
#endif
);
typedef void (*FSmsSetPropertiesProc) (
#ifdef __STDC__
	FSmsConn, FSmPointer, int, FSmProp **
#endif
);
typedef void (*FSmsDeletePropertiesProc) (
#ifdef __STDC__
	FSmsConn, FSmPointer, int, char **
#endif
);
typedef void (*FSmsGetPropertiesProc) (
#ifdef __STDC__
	FSmsConn, FSmPointer
#endif
);

typedef struct {
    struct {
	FSmsRegisterClientProc	callback;
	FSmPointer		manager_data;
    } register_client;
    struct {
	FSmsInteractRequestProc	callback;
	FSmPointer		manager_data;
    } interact_request;
    struct {
	FSmsInteractDoneProc	callback;
	FSmPointer		manager_data;
    } interact_done;
    struct {
	FSmsSaveYourselfRequestProc	callback;
	FSmPointer			manager_data;
    } save_yourself_request;
    struct {
	FSmsSaveYourselfPhase2RequestProc	callback;
	FSmPointer				manager_data;
    } save_yourself_phase2_request;
    struct {
	FSmsSaveYourselfDoneProc	callback;
	FSmPointer		manager_data;
    } save_yourself_done;
    struct {
	FSmsCloseConnectionProc	callback;
	FSmPointer		manager_data;
    } close_connection;
    struct {
	FSmsSetPropertiesProc	callback;
	FSmPointer		manager_data;
    } set_properties;
    struct {
	FSmsDeletePropertiesProc	callback;
	FSmPointer		manager_data;
    } delete_properties;
    struct {
	FSmsGetPropertiesProc	callback;
	FSmPointer		manager_data;
    } get_properties;
} FSmsCallbacks;

typedef Status (*FSmsNewClientProc) (
#ifdef __STDC__
    FSmsConn,FSmPointer, unsigned long *, FSmsCallbacks *, char **
#endif
);
typedef void (*FSmcErrorHandler) (
#ifdef __STDC__
    FSmcConn, Bool, int, unsigned long, int, int, FSmPointer
#endif
);
typedef void (*FSmsErrorHandler) (
#ifdef __STDC__
    FSmsConn, Bool, int, unsigned long, int, int, FSmPointer
#endif
);

/* ICEutil.h */
typedef struct {
    char *protocol_name;
    unsigned short protocol_data_length;
    char *protocol_data;
    char *network_id;
    char *auth_name;
    unsigned short auth_data_length;
    char *auth_data;
} FIceAuthFileEntry;

typedef struct {
    char *protocol_name;
    char *network_id;
    char *auth_name;
    unsigned short auth_data_length;
    char *auth_data;
} FIceAuthDataEntry;

#endif

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

#if SessionSupport

/* ICElib.h (no all) */
#define FIceSetIOErrorHandler(a) IceSetIOErrorHandler(a)
#define FIceFreeListenObjs(a,b) IceFreeListenObjs(a,b)
#define FIceConnectionNumber(a) IceConnectionNumber(a)
#define FIceSetShutdownNegotiation(a,b) IceSetShutdownNegotiation(a,b)
#define FIceCloseConnection(a) IceCloseConnection(a)
#define FIceConnectionStatus(a) IceConnectionStatus(a)
#define FIceAcceptConnection(a,b) IceAcceptConnection(a,b)
#define FIceProcessMessages(a,b,c) IceProcessMessages(a,b,c)
#define FIceListenForConnections(a,b,c,d) IceListenForConnections(a,b,c,d)
#define FIceAddConnectionWatch(a,b) IceAddConnectionWatch(a,b)
#define FIceGetListenConnectionNumber(a) IceGetListenConnectionNumber(a)
#define FIceComposeNetworkIdList(a,b) IceComposeNetworkIdList(a,b)
#define FIceGetListenConnectionString(a) IceGetListenConnectionString(a)
#define FIceSetHostBasedAuthProc(a,b) IceSetHostBasedAuthProc(a,b)
#define FIceConnectionString(a) IceConnectionString(a)

/* SMlib.h */
#define FSmcOpenConnection(a,b,c,d,e,f,g,h,i,k) \
        SmcOpenConnection(a,b,c,d,e,f,g,h,i,k)
#define FSmcCloseConnection(a,b,c) SmcCloseConnection(a,b,c)
#define FSmcModifyCallbacks(a,b,c) SmcModifyCallbacks(a,b,c)
#define FSmcSetProperties(a,b,c) SmcSetProperties(a,b,c)
#define FSmcDeleteProperties(a,b,c) SmcDeleteProperties(a,b,c)
#define FSmcGetProperties(a,b,c) SmcGetProperties(a,b,c)
#define FSmcInteractRequest(a,b,c,d) SmcInteractRequest(a,b,c,d)
#define FSmcInteractDone(a,b) SmcInteractDone(a,b)
#define FSmcRequestSaveYourself(a,b,c,d,e,f) SmcRequestSaveYourself(a,b,c,d,e,f)
#define FSmcRequestSaveYourselfPhase2(a,b,c) SmcRequestSaveYourselfPhase2(a,b,c)
#define FSmcSaveYourselfDone(a,b)  SmcSaveYourselfDone(a,b)
#define FSmcProtocolVersion(a) SmcProtocolVersion(a)
#define FSmcProtocolRevision(a) SmcProtocolRevision(a)
#define FSmcVendor(a) SmcVendor(a)
#define FSmcRelease(a) SmcRelease(a)
#define FSmcClientID(a) SmcClientID(a)
#define FSmcGetIceConnection(a) SmcGetIceConnection(a)
#define FSmsInitialize(a,b,c,d,e,f,g) SmsInitialize(a,b,c,d,e,f,g)
#define FSmsClientHostName(a) SmsClientHostName(a)
#define FSmsGenerateClientID(a) SmsGenerateClientID(a)
#define FSmsRegisterClientReply(a,b) SmsRegisterClientReply(a,b)
#define FSmsSaveYourself(a,b,c,d,e) SmsSaveYourself(a,b,c,d,e)
#define FSmsSaveYourselfPhase2(a) SmsSaveYourselfPhase2(a)
#define FSmsInteract(a) SmsInteract(a)
#define FSmsDie(a) SmsDie(a)
#define FSmsSaveComplete(a) SmsSaveComplete(a)
#define FSmsShutdownCancelled(a) SmsShutdownCancelled(a)
#define FSmsReturnProperties(a,b,c) SmsReturnProperties(a,b,c)
#define FSmsCleanUp(a) SmsCleanUp(a)
#define FSmsProtocolVersion(a) SmsProtocolVersion(a)
#define FSmsProtocolRevision(a) SmsProtocolRevision(a)
#define FSmsClientID(a) SmsClientID(a)
#define FSmsGetIceConnection(a) SmsGetIceConnection(a)
#define FSmcSetErrorHandler(a) SmcSetErrorHandler(a)
#define FSmsSetErrorHandler(a) SmsSetErrorHandler(a)
#define FSmFreeProperty(a) SmFreeProperty(a)
#define FSmFreeReasons(a,b) SmFreeReasons(a,b)

/* ICEutils.h */
#define FIceAuthFileName(a) IceAuthFileName(a)
#define FIceLockAuthFile(a,b,c,d) IceLockAuthFile(a,b,c,d)
#define FIceUnlockAuthFile(a) IceUnlockAuthFile(a)
#define FIceReadAuthFileEntry(a) IceReadAuthFileEntry(a)
#define FIceFreeAuthFileEntry(a) IceFreeAuthFileEntry(a)
#define FIceWriteAuthFileEntry(a,b) IceWriteAuthFileEntry(a,b)
#define FIceGetAuthFileEntry(a,b,c) IceGetAuthFileEntry(a,b,c)
#define FIceGenerateMagicCookie(a) IceGenerateMagicCookie(a)
#define FIceSetPaAuthData(a,b) IceSetPaAuthData(a,b)

#else

/* ICElib.h (no all) */
#define FIceSetIOErrorHandler(a) NULL
#define FIceFreeListenObjs(a,b)
#define FIceConnectionNumber(a) 0
#define FIceSetShutdownNegotiation(a,b)
#define FIceCloseConnection(a) 0
#define FIceConnectionStatus(a) 0
#define FIceAcceptConnection(a,b) NULL
#define FIceProcessMessages(a,b,c) 0
#define FIceListenForConnections(a,b,c,d) 0
#define FIceAddConnectionWatch(a,b) 0
#define FIceGetListenConnectionNumber(a) 0
#define FIceComposeNetworkIdList(a,b) NULL
#define FIceGetListenConnectionString(a) NULL
#define FIceSetHostBasedAuthProc(a,b)
#define FIceConnectionString(a) NULL

/* SMlib.h */
#define FSmcOpenConnection(a,b,c,d,e,f,g,h,i,k) NULL
#define FSmcCloseConnection(a,b,c) 0
#define FSmcModifyCallbacks(a,b,c)
#define FSmcSetProperties(a,b,c)
#define FSmcDeleteProperties(a,b,c)
#define FSmcGetProperties(a,b,c) 0
#define FSmcInteractRequest(a,b,c,d) 0
#define FSmcInteractDone(a,b)
#define FSmcRequestSaveYourself(a,b,c,d,e,f)
#define FSmcRequestSaveYourselfPhase2(a,b,c) 0
#define FSmcSaveYourselfDone(a,b)
#define FSmcProtocolVersion(a) 0
#define FSmcProtocolRevision(a) 0
#define FSmcVendor(a) NULL
#define FSmcRelease(a) NULL
#define FSmcClientID(a) NULL
#define FSmcGetIceConnection(a) NULL
#define FSmsInitialize(a,b,c,d,e,f,g) 0
#define FSmsClientHostName(a) NULL
#define FSmsGenerateClientID(a) NULL
#define FSmsRegisterClientReply(a,b) 0
#define FSmsSaveYourself(a,b,c,d,e)
#define FSmsSaveYourselfPhase2(a)
#define FSmsInteract(a)
#define FSmsDie(a)
#define FSmsSaveComplete(a)
#define FSmsShutdownCancelled(a)
#define FSmsReturnProperties(a,b,c)
#define FSmsCleanUp(a)
#define FSmsProtocolVersion(a) 0
#define FSmsProtocolRevision(a) 0
#define FSmsClientID(a) NULL
#define FSmsGetIceConnection(a) NULL
#define FSmcSetErrorHandler(a) NULL
#define FSmsSetErrorHandler(a) NULL
#define FSmFreeProperty(a)
#define FSmFreeReasons(a,b)

/* ICEutils.h */
#define FIceAuthFileName(a) NULL
#define FIceLockAuthFile(a,b,c,d) 0
#define FIceUnlockAuthFile(a)
#define FIceReadAuthFileEntry(a) NULL
#define FIceFreeAuthFileEntry(a)
#define FIceWriteAuthFileEntry(a,b) 0
#define FIceGetAuthFileEntry(a,b,c) NULL
#define FIceGenerateMagicCookie(a) NULL
#define FIceSetPaAuthData(a,b)

#endif

#endif /* FSMlib_H */
