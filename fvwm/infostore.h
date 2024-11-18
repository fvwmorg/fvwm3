/* -*-c-*- */

#ifndef FVWM_INFOSTORE_H
#define FVWM_INFOSTORE_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef struct meta_info
{
    char *key;
    char *value;

    TAILQ_ENTRY(meta_info) entry;
} MetaInfo;
TAILQ_HEAD(meta_infos, meta_info);
extern struct meta_infos meta_info_q;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void insert_metainfo(char *, char *);
char *get_metainfo_value(const char *);
void print_infostore(void);

#endif /* FVWM_INFOSTORE_H */
