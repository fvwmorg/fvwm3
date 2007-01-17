/* -*-c-*- */

#ifndef FVWM_MODULE_INTERFACE_H
#define FVWM_MODULE_INTERFACE_H

#include "Module.h"

struct fmodule;
struct fmodule_input;

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


void module_input_enqueue(struct fmodule_input *input);
void module_input_execute(struct fmodule_input *input);

void ExecuteCommandQueue(void);

#endif /* MODULE_INTERFACE_H */
