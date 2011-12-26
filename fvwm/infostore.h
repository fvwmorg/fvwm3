/* -*-c-*- */

#ifndef INFOSTORE_H
#define INFOSTORE_H

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
char *get_metainfo_value(char *);

#endif /* INFOSTORE_H */
