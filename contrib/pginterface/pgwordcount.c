/*
 * wordcount.c
 *
*/

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include "halt.h"
#include <libpq-fe.h>
#include "pginterface.h"

int main(int argc, char **argv)
{
	char query[4000];
	int row = 0;
	int count;
	char line[4000];
	
	if (argc != 2)
		halt("Usage:  %s database\n",argv[0]);

	connectdb(argv[1],NULL,NULL,NULL,NULL);
	on_error_continue();
	doquery("DROP TABLE words");
	on_error_stop();

	doquery("\
		CREATE TABLE words( \
			matches	int4, \
			word	text ) \
		");
	doquery("\
		CREATE INDEX i_words_1 ON words USING btree ( \
			word text_ops )\
		");

	while(1)
	{
		if (scanf("%s",line) != 1)
			break;
		doquery("BEGIN WORK");
		sprintf(query,"\
				DECLARE c_words BINARY CURSOR FOR \
				SELECT count(*) \
				FROM words \
				WHERE word = '%s'", line);
		doquery(query);
		doquery("FETCH ALL IN c_words");
		
		while (fetch(&count) == END_OF_TUPLES)
			count = 0;
		doquery("CLOSE c_words");
		doquery("COMMIT WORK");

		if (count == 0)
			sprintf(query,"\
				INSERT INTO words \
				VALUES (1, '%s')",	line);
		else
			sprintf(query,"\
				UPDATE words \
				SET matches = matches + 1
				WHERE word = '%s'",	line);
		doquery(query);
		row++;
	}

	disconnectdb();
	return 0;
}

