typedef union {
	double                  dval;
        int                     ival;
	char *                  str;
	struct when             action;
	struct index		index;
	int			tagname;
	enum ECPGttype		type_enum;
} YYSTYPE;
#define	SQL_BREAK	258
#define	SQL_CALL	259
#define	SQL_CONNECT	260
#define	SQL_CONNECTION	261
#define	SQL_CONTINUE	262
#define	SQL_DISCONNECT	263
#define	SQL_FOUND	264
#define	SQL_GO	265
#define	SQL_GOTO	266
#define	SQL_IDENTIFIED	267
#define	SQL_IMMEDIATE	268
#define	SQL_INDICATOR	269
#define	SQL_OPEN	270
#define	SQL_RELEASE	271
#define	SQL_SECTION	272
#define	SQL_SEMI	273
#define	SQL_SQLERROR	274
#define	SQL_SQLPRINT	275
#define	SQL_START	276
#define	SQL_STOP	277
#define	SQL_WHENEVER	278
#define	S_ANYTHING	279
#define	S_AUTO	280
#define	S_BOOL	281
#define	S_CHAR	282
#define	S_CONST	283
#define	S_DOUBLE	284
#define	S_EXTERN	285
#define	S_FLOAT	286
#define	S_INT	287
#define	S_LONG	288
#define	S_REGISTER	289
#define	S_SHORT	290
#define	S_SIGNED	291
#define	S_STATIC	292
#define	S_STRUCT	293
#define	S_UNSIGNED	294
#define	S_VARCHAR	295
#define	TYPECAST	296
#define	ACTION	297
#define	ADD	298
#define	ALL	299
#define	ALTER	300
#define	AND	301
#define	ANY	302
#define	AS	303
#define	ASC	304
#define	BEGIN_TRANS	305
#define	BETWEEN	306
#define	BOTH	307
#define	BY	308
#define	CASCADE	309
#define	CAST	310
#define	CHAR	311
#define	CHARACTER	312
#define	CHECK	313
#define	CLOSE	314
#define	COLLATE	315
#define	COLUMN	316
#define	COMMIT	317
#define	CONSTRAINT	318
#define	CREATE	319
#define	CROSS	320
#define	CURRENT	321
#define	CURRENT_DATE	322
#define	CURRENT_TIME	323
#define	CURRENT_TIMESTAMP	324
#define	CURRENT_USER	325
#define	CURSOR	326
#define	DAY_P	327
#define	DECIMAL	328
#define	DECLARE	329
#define	DEFAULT	330
#define	DELETE	331
#define	DESC	332
#define	DISTINCT	333
#define	DOUBLE	334
#define	DROP	335
#define	END_TRANS	336
#define	EXECUTE	337
#define	EXISTS	338
#define	EXTRACT	339
#define	FETCH	340
#define	FLOAT	341
#define	FOR	342
#define	FOREIGN	343
#define	FROM	344
#define	FULL	345
#define	GRANT	346
#define	GROUP	347
#define	HAVING	348
#define	HOUR_P	349
#define	IN	350
#define	INNER_P	351
#define	INSERT	352
#define	INTERVAL	353
#define	INTO	354
#define	IS	355
#define	JOIN	356
#define	KEY	357
#define	LANGUAGE	358
#define	LEADING	359
#define	LEFT	360
#define	LIKE	361
#define	LOCAL	362
#define	MATCH	363
#define	MINUTE_P	364
#define	MONTH_P	365
#define	NATIONAL	366
#define	NATURAL	367
#define	NCHAR	368
#define	NO	369
#define	NOT	370
#define	NOTIFY	371
#define	NULL_P	372
#define	NUMERIC	373
#define	ON	374
#define	OPTION	375
#define	OR	376
#define	ORDER	377
#define	OUTER_P	378
#define	PARTIAL	379
#define	POSITION	380
#define	PRECISION	381
#define	PRIMARY	382
#define	PRIVILEGES	383
#define	PROCEDURE	384
#define	PUBLIC	385
#define	REFERENCES	386
#define	REVOKE	387
#define	RIGHT	388
#define	ROLLBACK	389
#define	SECOND_P	390
#define	SELECT	391
#define	SET	392
#define	SUBSTRING	393
#define	TABLE	394
#define	TIME	395
#define	TIMESTAMP	396
#define	TIMEZONE_HOUR	397
#define	TIMEZONE_MINUTE	398
#define	TO	399
#define	TRAILING	400
#define	TRANSACTION	401
#define	TRIM	402
#define	UNION	403
#define	UNIQUE	404
#define	UPDATE	405
#define	USER	406
#define	USING	407
#define	VALUES	408
#define	VARCHAR	409
#define	VARYING	410
#define	VIEW	411
#define	WHERE	412
#define	WITH	413
#define	WORK	414
#define	YEAR_P	415
#define	ZONE	416
#define	FALSE_P	417
#define	TRIGGER	418
#define	TRUE_P	419
#define	TYPE_P	420
#define	ABORT_TRANS	421
#define	AFTER	422
#define	AGGREGATE	423
#define	ANALYZE	424
#define	BACKWARD	425
#define	BEFORE	426
#define	BINARY	427
#define	CACHE	428
#define	CLUSTER	429
#define	COPY	430
#define	CYCLE	431
#define	DATABASE	432
#define	DELIMITERS	433
#define	DO	434
#define	EACH	435
#define	EXPLAIN	436
#define	EXTEND	437
#define	FORWARD	438
#define	FUNCTION	439
#define	HANDLER	440
#define	INCREMENT	441
#define	INDEX	442
#define	INHERITS	443
#define	INSTEAD	444
#define	ISNULL	445
#define	LANCOMPILER	446
#define	LISTEN	447
#define	LOAD	448
#define	LOCK_P	449
#define	LOCATION	450
#define	MAXVALUE	451
#define	MINVALUE	452
#define	MOVE	453
#define	NEW	454
#define	NONE	455
#define	NOTHING	456
#define	NOTNULL	457
#define	OIDS	458
#define	OPERATOR	459
#define	PROCEDURAL	460
#define	RECIPE	461
#define	RENAME	462
#define	RESET	463
#define	RETURNS	464
#define	ROW	465
#define	RULE	466
#define	SEQUENCE	467
#define	SETOF	468
#define	SHOW	469
#define	START	470
#define	STATEMENT	471
#define	STDIN	472
#define	STDOUT	473
#define	TRUSTED	474
#define	VACUUM	475
#define	VERBOSE	476
#define	VERSION	477
#define	ARCHIVE	478
#define	PASSWORD	479
#define	CREATEDB	480
#define	NOCREATEDB	481
#define	CREATEUSER	482
#define	NOCREATEUSER	483
#define	VALID	484
#define	UNTIL	485
#define	IDENT	486
#define	SCONST	487
#define	Op	488
#define	CSTRING	489
#define	CVARIABLE	490
#define	ICONST	491
#define	PARAM	492
#define	FCONST	493
#define	OP	494
#define	UMINUS	495
#define	REDUCE	496


extern YYSTYPE yylval;
