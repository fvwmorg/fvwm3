/* -*-c-*- */
#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {  char *str;
          int number;
       } yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	STR	257
# define	GSTR	258
# define	VAR	259
# define	FONT	260
# define	NUMBER	261
# define	WINDOWTITLE	262
# define	WINDOWLOCALETITLE	263
# define	WINDOWSIZE	264
# define	WINDOWPOSITION	265
# define	USEGETTEXT	266
# define	FORECOLOR	267
# define	BACKCOLOR	268
# define	SHADCOLOR	269
# define	LICOLOR	270
# define	COLORSET	271
# define	OBJECT	272
# define	INIT	273
# define	PERIODICTASK	274
# define	QUITFUNC	275
# define	MAIN	276
# define	END	277
# define	PROP	278
# define	TYPE	279
# define	SIZE	280
# define	POSITION	281
# define	VALUE	282
# define	VALUEMIN	283
# define	VALUEMAX	284
# define	TITLE	285
# define	SWALLOWEXEC	286
# define	ICON	287
# define	FLAGS	288
# define	WARP	289
# define	WRITETOFILE	290
# define	LOCALETITLE	291
# define	HIDDEN	292
# define	NOFOCUS	293
# define	NORELIEFSTRING	294
# define	CENTER	295
# define	LEFT	296
# define	RIGHT	297
# define	CASE	298
# define	SINGLECLIC	299
# define	DOUBLECLIC	300
# define	BEG	301
# define	POINT	302
# define	EXEC	303
# define	HIDE	304
# define	SHOW	305
# define	CHFONT	306
# define	CHFORECOLOR	307
# define	CHBACKCOLOR	308
# define	CHCOLORSET	309
# define	KEY	310
# define	GETVALUE	311
# define	GETMINVALUE	312
# define	GETMAXVALUE	313
# define	GETFORE	314
# define	GETBACK	315
# define	GETHILIGHT	316
# define	GETSHADOW	317
# define	CHVALUE	318
# define	CHVALUEMAX	319
# define	CHVALUEMIN	320
# define	ADD	321
# define	DIV	322
# define	MULT	323
# define	GETTITLE	324
# define	GETOUTPUT	325
# define	STRCOPY	326
# define	NUMTOHEX	327
# define	HEXTONUM	328
# define	QUIT	329
# define	LAUNCHSCRIPT	330
# define	GETSCRIPTFATHER	331
# define	SENDTOSCRIPT	332
# define	RECEIVFROMSCRIPT	333
# define	GET	334
# define	SET	335
# define	SENDSIGN	336
# define	REMAINDEROFDIV	337
# define	GETTIME	338
# define	GETSCRIPTARG	339
# define	GETPID	340
# define	SENDMSGANDGET	341
# define	PARSE	342
# define	LASTSTRING	343
# define	GETTEXT	344
# define	IF	345
# define	THEN	346
# define	ELSE	347
# define	FOR	348
# define	TO	349
# define	DO	350
# define	WHILE	351
# define	BEGF	352
# define	ENDF	353
# define	EQUAL	354
# define	INFEQ	355
# define	SUPEQ	356
# define	INF	357
# define	SUP	358
# define	DIFF	359


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
