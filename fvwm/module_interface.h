/* -*-c-*- */

#ifndef FVWM_MODULE_INTERFACE_H
#define FVWM_MODULE_INTERFACE_H

#include "Module.h"

struct fmodule;

/* Packet sending functions */
void BroadcastPacket(unsigned long event_type, unsigned long num_datum, ...);
void BroadcastConfig(unsigned long event_type, const FvwmWindow *t);
void BroadcastName(
	unsigned long event_type, unsigned long data1, unsigned long data2,
	unsigned long data3, const char *name);
void BroadcastWindowIconNames(FvwmWindow *t, Bool window, Bool icon);
void BroadcastFvwmPicture(
	unsigned long event_type, unsigned long data1, unsigned long data2,
	unsigned long data3, FvwmPicture *picture, char *name);
void BroadcastPropertyChange(
	unsigned long argument, unsigned long data1,
	unsigned long data2, char *string);
void BroadcastColorset(int n);
void BroadcastConfigInfoString(char *string);
void broadcast_xinerama_state(void);
void broadcast_ignore_modifiers(void);
void SendPacket(
	struct fmodule *module, unsigned long event_type,
	unsigned long num_datum, ...);
void SendConfig(
	struct fmodule *module, unsigned long event_type, const FvwmWindow *t);
void SendName(
	struct fmodule *module, unsigned long event_type, unsigned long data1,
	unsigned long data2, unsigned long data3, const char *name);

/* command queue - module input */
/*static*/ void AddToCommandQueue(
		Window window, struct fmodule *module, char *command);
/*static*/ void ExecuteModuleCommand(
		Window w, struct fmodule *module, char *text);
void ExecuteCommandQueue(void);

#endif /* MODULE_INTERFACE_H */
