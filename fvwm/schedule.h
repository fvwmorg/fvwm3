/* -*-c-*- */

#ifndef SCHEDULE_H
#define SCHEDULE_H

void squeue_execute(void);
int squeue_get_next_ms(void);
int squeue_get_next_id(void);
int squeue_get_last_id(void);

#endif /* SCHEDULE_H */
