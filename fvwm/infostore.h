/* -*-c-*- */

#ifndef FVWM_INFOSTORE_H
#define FVWM_INFOSTORE_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef struct MetaInfo
{
    char *key;
    char *value;

    struct MetaInfo *next;
} MetaInfo;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

MetaInfo *new_metainfo(void);
void insert_metainfo(char *, char *);
char *get_metainfo_value(const char *);
int get_metainfo_length(void);
MetaInfo *get_metainfo(void);
void print_infostore(void);

#endif /* FVWM_INFOSTORE_H */
