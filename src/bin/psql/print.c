/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright (c) 2000-2006, PostgreSQL Global Development Group
 *
 * $PostgreSQL: pgsql/src/bin/psql/print.c,v 1.86 2006/06/07 22:24:45 momjian Exp $
 */
#include "postgres_fe.h"
#include "common.h"
#include "print.h"

#include <math.h>
#include <signal.h>
#include <unistd.h>

#ifndef WIN32
#include <sys/ioctl.h>			/* for ioctl() */
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include <locale.h>

#include "pqsignal.h"
#include "libpq-fe.h"

#include "mbprint.h"

static char *decimal_point;
static char *grouping;
static char *thousands_sep;

static void *
pg_local_malloc(size_t size)
{
	void	   *tmp;

	tmp = malloc(size);
	if (!tmp)
	{
		fprintf(stderr, _("out of memory\n"));
		exit(EXIT_FAILURE);
	}
	return tmp;
}

static void *
pg_local_calloc(int count, size_t size)
{
	void	   *tmp;

	tmp = calloc(count, size);
	if (!tmp)
	{
		fprintf(stderr, _("out of memory\n"));
		exit(EXIT_FAILURE);
	}
	return tmp;
}

static int
integer_digits(const char *my_str)
{
	int			frac_len;

	if (my_str[0] == '-')
		my_str++;

	frac_len = strchr(my_str, '.') ? strlen(strchr(my_str, '.')) : 0;

	return strlen(my_str) - frac_len;
}

/* Return additional length required for locale-aware numeric output */
static int
additional_numeric_locale_len(const char *my_str)
{
	int			int_len = integer_digits(my_str),
				len = 0;
	int			groupdigits = atoi(grouping);

	if (int_len > 0)
		/* Don't count a leading separator */
		len = (int_len / groupdigits - (int_len % groupdigits == 0)) *
			strlen(thousands_sep);

	if (strchr(my_str, '.') != NULL)
		len += strlen(decimal_point) - strlen(".");

	return len;
}

static int
strlen_with_numeric_locale(const char *my_str)
{
	return strlen(my_str) + additional_numeric_locale_len(my_str);
}

/* Returns the appropriately formatted string in a new allocated block, caller must free */
static char *
format_numeric_locale(const char *my_str)
{
	int			i,
				j,
				int_len = integer_digits(my_str),
				leading_digits;
	int			groupdigits = atoi(grouping);
	int			new_str_start = 0;
	char	   *new_str = new_str = pg_local_malloc(
									 strlen_with_numeric_locale(my_str) + 1);

	leading_digits = (int_len % groupdigits != 0) ?
		int_len % groupdigits : groupdigits;

	if (my_str[0] == '-')		/* skip over sign, affects grouping
								 * calculations */
	{
		new_str[0] = my_str[0];
		my_str++;
		new_str_start = 1;
	}

	for (i = 0, j = new_str_start;; i++, j++)
	{
		/* Hit decimal point? */
		if (my_str[i] == '.')
		{
			strcpy(&new_str[j], decimal_point);
			j += strlen(decimal_point);
			/* add fractional part */
			strcpy(&new_str[j], &my_str[i] + 1);
			break;
		}

		/* End of string? */
		if (my_str[i] == '\0')
		{
			new_str[j] = '\0';
			break;
		}

		/* Add separator? */
		if (i != 0 && (i - leading_digits) % groupdigits == 0)
		{
			strcpy(&new_str[j], thousands_sep);
			j += strlen(thousands_sep);
		}

		new_str[j] = my_str[i];
	}

	return new_str;
}

/*************************/
/* Unaligned text		 */
/*************************/


static void
print_unaligned_text(const char *title, const char *const * headers,
					 const char *const * cells, const char *const * footers,
					 const char *opt_align, const char *opt_fieldsep,
					 const char *opt_recordsep, bool opt_tuples_only,
					 bool opt_numeric_locale, FILE *fout)
{
	unsigned int col_count = 0;
	unsigned int i;
	const char *const * ptr;
	bool		need_recordsep = false;

	if (!opt_fieldsep)
		opt_fieldsep = "";
	if (!opt_recordsep)
		opt_recordsep = "";

	/* print title */
	if (!opt_tuples_only && title)
		fprintf(fout, "%s%s", title, opt_recordsep);

	/* print headers and count columns */
	for (ptr = headers; *ptr; ptr++)
	{
		col_count++;
		if (!opt_tuples_only)
		{
			if (col_count > 1)
				fputs(opt_fieldsep, fout);
			fputs(*ptr, fout);
		}
	}
	if (!opt_tuples_only)
		need_recordsep = true;

	/* print cells */
	i = 0;
	for (ptr = cells; *ptr; ptr++)
	{
		if (need_recordsep)
		{
			fputs(opt_recordsep, fout);
			need_recordsep = false;
		}
		if (opt_align[i % col_count] == 'r' && opt_numeric_locale)
		{
			char	   *my_cell = format_numeric_locale(*ptr);

			fputs(my_cell, fout);
			free(my_cell);
		}
		else
			fputs(*ptr, fout);

		if ((i + 1) % col_count)
			fputs(opt_fieldsep, fout);
		else
			need_recordsep = true;
		i++;
	}

	/* print footers */

	if (!opt_tuples_only && footers)
		for (ptr = footers; *ptr; ptr++)
		{
			if (need_recordsep)
			{
				fputs(opt_recordsep, fout);
				need_recordsep = false;
			}
			fputs(*ptr, fout);
			need_recordsep = true;
		}

	/* the last record needs to be concluded with a newline */
	if (need_recordsep)
		fputc('\n', fout);
}



static void
print_unaligned_vertical(const char *title, const char *const * headers,
						 const char *const * cells,
						 const char *const * footers, const char *opt_align,
						 const char *opt_fieldsep, const char *opt_recordsep,
				   bool opt_tuples_only, bool opt_numeric_locale, FILE *fout)
{
	unsigned int col_count = 0;
	unsigned int i;
	const char *const * ptr;

	if (!opt_fieldsep)
		opt_fieldsep = "";
	if (!opt_recordsep)
		opt_recordsep = "";

	/* print title */
	if (!opt_tuples_only && title)
		fputs(title, fout);

	/* count columns */
	for (ptr = headers; *ptr; ptr++)
		col_count++;

	/* print records */
	for (i = 0, ptr = cells; *ptr; i++, ptr++)
	{
		if (i != 0 || (!opt_tuples_only && title))
		{
			fputs(opt_recordsep, fout);
			if (i % col_count == 0)
				fputs(opt_recordsep, fout);		/* another one */
		}

		fputs(headers[i % col_count], fout);
		fputs(opt_fieldsep, fout);
		if (opt_align[i % col_count] == 'r' && opt_numeric_locale)
		{
			char	   *my_cell = format_numeric_locale(*ptr);

			fputs(my_cell, fout);
			free(my_cell);
		}
		else
			fputs(*ptr, fout);
	}

	/* print footers */
	if (!opt_tuples_only && footers && *footers)
	{
		fputs(opt_recordsep, fout);
		for (ptr = footers; *ptr; ptr++)
		{
			fputs(opt_recordsep, fout);
			fputs(*ptr, fout);
		}
	}

	fputc('\n', fout);
}



/********************/
/* Aligned text		*/
/********************/


/* draw "line" */
static void
_print_horizontal_line(const unsigned int col_count, const unsigned int *widths, unsigned short border, FILE *fout)
{
	unsigned int i,
				j;

	if (border == 1)
		fputc('-', fout);
	else if (border == 2)
		fputs("+-", fout);

	for (i = 0; i < col_count; i++)
	{
		for (j = 0; j < widths[i]; j++)
			fputc('-', fout);

		if (i < col_count - 1)
		{
			if (border == 0)
				fputc(' ', fout);
			else
				fputs("-+-", fout);
		}
	}

	if (border == 2)
		fputs("-+", fout);
	else if (border == 1)
		fputc('-', fout);

	fputc('\n', fout);
}



static void
print_aligned_text(const char *title, const char *const * headers,
				   const char *const * cells, const char *const * footers,
		const char *opt_align, bool opt_tuples_only, bool opt_numeric_locale,
				   unsigned short int opt_border, int encoding,
				   FILE *fout)
{
	unsigned int col_count = 0;
	unsigned int cell_count = 0;
	unsigned int i;
	int tmp;
	unsigned int *widths,
				total_w;
	unsigned int *heights;
	unsigned int *format_space;
	unsigned char **format_buf;
	
	const char *const *ptr;
	
	struct lineptr **col_lineptrs;   /* pointers to line pointer for each column */
	struct lineptr *lineptr_list;    /* complete list of linepointers */
	
	int *complete;			/* Array remembering which columns have completed output */

	/* count columns */
	for (ptr = headers; *ptr; ptr++)
		col_count++;

	if (col_count > 0)
	{
		widths = pg_local_calloc(col_count, sizeof(*widths));
		heights = pg_local_calloc(col_count, sizeof(*heights));
		col_lineptrs = pg_local_calloc(col_count, sizeof(*col_lineptrs));
		format_space = pg_local_calloc(col_count, sizeof(*format_space));
		format_buf = pg_local_calloc(col_count, sizeof(*format_buf));
		complete = pg_local_calloc(col_count, sizeof(*complete));
	
	}
	else
	{
		widths = NULL;
		heights = NULL;
		col_lineptrs = NULL;
		format_space = NULL;
		format_buf = NULL;
		complete = NULL;
	}
	
	/* count cells (rows * cols) */
	for (ptr = cells; *ptr; ptr++)
		cell_count++;

	/* calc column widths */
	for (i = 0; i < col_count; i++)
	{
		/* Get width & height */
		int height, space;
		pg_wcssize((unsigned char *) headers[i], strlen(headers[i]), encoding, &tmp, &height, &space);
		if (tmp > widths[i])
			widths[i] = tmp;
		if (height > heights[i])
			heights[i] = height;
		if (space > format_space[i])
			format_space[i] = space;
	}

	for (i = 0, ptr = cells; *ptr; ptr++, i++)
	{
		int numeric_locale_len;
		int height, space;

		if (opt_align[i % col_count] == 'r' && opt_numeric_locale)
			numeric_locale_len = additional_numeric_locale_len(*ptr);
		else 
			numeric_locale_len = 0;
		
		/* Get width, ignore height */
		pg_wcssize((unsigned char *) *ptr, strlen(*ptr), encoding, &tmp, &height, &space);
		tmp += numeric_locale_len;
		if (tmp > widths[i % col_count])
			widths[i % col_count] = tmp;
		if (height > heights[i % col_count])
			heights[i % col_count] = height;
		if (space > format_space[i % col_count])
			format_space[i % col_count] = space;
	}

	if (opt_border == 0)
		total_w = col_count - 1;
	else if (opt_border == 1)
		total_w = col_count * 3 - 1;
	else
		total_w = col_count * 3 + 1;

	for (i = 0; i < col_count; i++)
		total_w += widths[i];

	/* At this point:
	 *  widths contains the max width of each column
	 *  heights contains the max height of a cell of each column
	 *  format_space contains maximum space required to store formatted string
	 * so we prepare the formatting structures
	 */
	if (col_count > 0)
	{
		int heights_total = 0;
		struct lineptr *lineptr;
		
		for (i = 0; i < col_count; i++)
			heights_total += heights[i];
			
		lineptr = lineptr_list = pg_local_calloc(heights_total, sizeof(*lineptr_list));
	   		
		for (i = 0; i < col_count; i++)
		{
			col_lineptrs[i] = lineptr;
			lineptr += heights[i];
			
			format_buf[i] = pg_local_malloc(format_space[i]);
			
			col_lineptrs[i]->ptr = format_buf[i];
		}
	}
	else
		lineptr_list = NULL;
	   		
	/* print title */
	if (title && !opt_tuples_only)
	{
		/* Get width & height */
		int height;
		pg_wcssize((unsigned char *)title, strlen(title), encoding, &tmp, &height, NULL);
		if (tmp >= total_w)
			fprintf(fout, "%s\n", title);
		else
			fprintf(fout, "%-*s%s\n", (total_w - tmp) / 2, "", title);
	}

	/* print headers */
	if (!opt_tuples_only)
	{
		int cols_todo;
		int line_count;
		
		if (opt_border == 2)
			_print_horizontal_line(col_count, widths, opt_border, fout);

		for (i = 0; i < col_count; i++)
			pg_wcsformat((unsigned char *)headers[i], strlen(headers[i]), encoding, col_lineptrs[i], heights[i]);
	
		cols_todo = col_count;
		line_count = 0;
		memset(complete, 0, col_count*sizeof(int));
		while (cols_todo)
		{
			if (opt_border == 2)
				fprintf(fout, "|%c", line_count ? '+' : ' ');
			else if (opt_border == 1)
				fputc(line_count ? '+' : ' ', fout);

			for (i = 0; i < col_count; i++)
			{
				unsigned int nbspace;

				struct lineptr *this_line = col_lineptrs[i] + line_count;
				if (!complete[i])
				{
					nbspace = widths[i] - this_line->width;

					/* centered */
					fprintf(fout, "%-*s%s%-*s",
							nbspace / 2, "", this_line->ptr, (nbspace + 1) / 2, "");

					if (line_count == (heights[i]-1) || !(this_line+1)->ptr)
					{
						cols_todo--;
						complete[i] = 1;
					}
				}
				else
					fprintf(fout, "%*s", widths[i], "");
				if (i < col_count - 1)
				{
					if (opt_border == 0)
						fputc(line_count ? '+' : ' ', fout);
					else
						fprintf(fout, " |%c", line_count ? '+' : ' ');
				}
			}
			line_count++;

			if (opt_border == 2)
				fputs(" |", fout);
			else if (opt_border == 1)
				fputc(' ', fout);;
			fputc('\n', fout);
		}


		_print_horizontal_line(col_count, widths, opt_border, fout);
	}

	/* print cells */
	for (i = 0, ptr = cells; *ptr; i+=col_count, ptr+=col_count)
	{
		int j;
		int cols_todo = col_count;
		int line_count;			/* Number of lines output so far in row */
		
		for (j = 0; j < col_count; j++)
			pg_wcsformat((unsigned char*)ptr[j], strlen(ptr[j]), encoding, col_lineptrs[j], heights[j]);
	
		line_count = 0;
		memset(complete, 0, col_count*sizeof(int));
		while (cols_todo)
		{
			/* beginning of line */
			if (opt_border == 2)
				fputs("| ", fout);
			else if (opt_border == 1)
				fputc(' ', fout);

			for (j = 0; j < col_count; j++)
			{
				struct lineptr *this_line = col_lineptrs[j] + line_count;
				bool	finalspaces = (opt_border == 2 || j != col_count-1);

				if (complete[j])  /* Just print spaces... */
				{
					if (finalspaces)
						fprintf(fout, "%*s", widths[j], "");
				}
				else
				{
					/* content */
					if (opt_align[j] == 'r')
					{
						if (opt_numeric_locale)
						{
							/*
							 * Assumption: This code used only on strings
							 * without multibyte characters, otherwise
							 * this_line->width < strlen(this_ptr) and we
							 * get an overflow
							 */
							char *my_cell = format_numeric_locale((char *) this_line->ptr);
							fprintf(fout, "%*s%s",
									(int) (widths[i % col_count] - strlen(my_cell)), "",
									my_cell);
							free(my_cell);
						}
						else
							fprintf(fout, "%*s%s",
									widths[j] - this_line->width, "",
									this_line->ptr);
					}
					else
						fprintf(fout, "%-s%*s", this_line->ptr,
								finalspaces ? (widths[j] - this_line->width) : 0, "");
					/* If at the right height, done this col */
					if (line_count == heights[j]-1 || !this_line[1].ptr)
					{
						complete[j] = 1;
						cols_todo--;
					}
				}
	
				/* divider */
				if ((j + 1) % col_count)
				{
					if (opt_border == 0)
						fputc(' ', fout);
					else if (line_count == 0)
						fputs(" | ", fout);
					else
						fprintf(fout, " %c ", complete[j+1] ? ' ' : ':');
				}
			}
			if (opt_border == 2)
				fputs(" |", fout);
			fputc('\n', fout);
			line_count++;
		}
	}

	if (opt_border == 2)
		_print_horizontal_line(col_count, widths, opt_border, fout);

	/* print footers */
	if (footers && !opt_tuples_only)
		for (ptr = footers; *ptr; ptr++)
			fprintf(fout, "%s\n", *ptr);

#ifndef WIN32

	/*
	 * for some reason MinGW (and MSVC) outputs an extra newline, so this supresses it
	 */
	fputc('\n', fout);
#endif

	/* clean up */
	free(widths);
	free(heights);
	free(col_lineptrs);
	free(format_space);
	free(complete);
	free(lineptr_list);
	for (i= 0; i < col_count; i++)
		free(format_buf[i]);
	free(format_buf);
}



static void
print_aligned_vertical(const char *title, const char *const * headers,
					   const char *const * cells, const char *const * footers,
					   const char *opt_align, bool opt_tuples_only,
					   bool opt_numeric_locale, unsigned short int opt_border,
					   int encoding, FILE *fout)
{
	unsigned int col_count = 0;
	unsigned int record = 1;
	const char *const * ptr;
	unsigned int i,
				hwidth = 0,
				dwidth = 0,
				hheight = 1,
				dheight = 1,
				hformatsize = 0,
				dformatsize = 0;
	int tmp = 0;
	char	   *divider;
	unsigned int cell_count = 0;
	struct lineptr *hlineptr, *dlineptr;
	
	if (cells[0] == NULL)
	{
		fprintf(fout, _("(No rows)\n"));
		return;
	}

	/* count headers and find longest one */
	for (ptr = headers; *ptr; ptr++)
		col_count++;

	/* Find the maximum dimensions for the headers */
	for (i = 0; i < col_count; i++)
	{
		int height, fs;
		pg_wcssize((unsigned char *) headers[i], strlen(headers[i]), encoding, &tmp, &height, &fs);
		if (tmp > hwidth)
			hwidth = tmp;
		if (height > hheight)
			hheight = height;
		if (fs > hformatsize)
			hformatsize = fs;
	}

	/* Count cells, find their lengths */
	for (ptr = cells; *ptr; ptr++)
		cell_count++;

	/* find longest data cell */
	for (i = 0, ptr = cells; *ptr; ptr++, i++)
	{
		int numeric_locale_len;
		int height, fs;

		if (opt_align[i % col_count] == 'r' && opt_numeric_locale)
			numeric_locale_len = additional_numeric_locale_len(*ptr);
		else 
			numeric_locale_len = 0;

		pg_wcssize((unsigned char *) *ptr, strlen(*ptr), encoding, &tmp, &height, &fs);
		tmp += numeric_locale_len;
		if (tmp > dwidth)
			dwidth = tmp;
		if (height > dheight)
			dheight = height;
		if (fs > dformatsize)
			dformatsize = fs;
	}
	
	/* We now have all the information we need to setup the formatting structures */
	dlineptr = pg_local_malloc(sizeof(*dlineptr) * dheight);
	hlineptr = pg_local_malloc(sizeof(*hlineptr) * hheight);
	
	dlineptr->ptr = pg_local_malloc(dformatsize);
	hlineptr->ptr = pg_local_malloc(hformatsize);
	
	/* print title */
	if (!opt_tuples_only && title)
		fprintf(fout, "%s\n", title);

	/* make horizontal border */
	divider = pg_local_malloc(hwidth + dwidth + 10);
	divider[0] = '\0';
	if (opt_border == 2)
		strcat(divider, "+-");
	for (i = 0; i < hwidth; i++)
		strcat(divider, opt_border > 0 ? "-" : " ");
	if (opt_border > 0)
		strcat(divider, "-+-");
	else
		strcat(divider, " ");
	for (i = 0; i < dwidth; i++)
		strcat(divider, opt_border > 0 ? "-" : " ");
	if (opt_border == 2)
		strcat(divider, "-+");

	/* print records */
	for (i = 0, ptr = cells; *ptr; i++, ptr++)
	{
		int line_count, dcomplete, hcomplete;
		
		if (i % col_count == 0)
		{
			if (!opt_tuples_only)
			{
				char	   *record_str = pg_local_malloc(32);
				size_t		record_str_len;

				if (opt_border == 0)
					snprintf(record_str, 32, "* Record %d", record++);
				else
					snprintf(record_str, 32, "[ RECORD %d ]", record++);
				record_str_len = strlen(record_str);

				if (record_str_len + opt_border > strlen(divider))
					fprintf(fout, "%.*s%s\n", opt_border, divider, record_str);
				else
				{
					char	   *div_copy = strdup(divider);

					if (!div_copy)
					{
						fprintf(stderr, _("out of memory\n"));
						exit(EXIT_FAILURE);
					}

					strncpy(div_copy + opt_border, record_str, record_str_len);
					fprintf(fout, "%s\n", div_copy);
					free(div_copy);
				}
				free(record_str);
			}
			else if (i != 0 || opt_border == 2)
				fprintf(fout, "%s\n", divider);
		}

		/* Format the header */
		pg_wcsformat((unsigned char*)headers[i % col_count],
					 strlen(headers[i % col_count]), encoding, hlineptr, hheight);
		/* Format the data */
		pg_wcsformat((unsigned char*)*ptr, strlen(*ptr), encoding, dlineptr, dheight);
		
		line_count = 0;
		dcomplete = hcomplete = 0;
		while (!dcomplete || !hcomplete)
		{
			if (opt_border == 2)
				fputs("| ", fout);
			if (!hcomplete)
			{
				fprintf(fout, "%-s%*s", hlineptr[line_count].ptr,
						hwidth - hlineptr[line_count].width, "");
						
				if (line_count == (hheight-1) || !hlineptr[line_count+1].ptr)
					hcomplete = 1;
			}
			else
				fprintf(fout, "%*s", hwidth, "");
	
			if (opt_border > 0)
				fprintf(fout, " %c ", (line_count==0)?'|':':');
			else
				fputs(" ", fout);

			if (!dcomplete)
			{
	 			if (opt_align[i % col_count] == 'r' && opt_numeric_locale)
 				{
 					char *my_cell = format_numeric_locale((char *) dlineptr[line_count].ptr);
 					if (opt_border < 2)
 						fprintf(fout, "%s\n", my_cell);
 					else
 						fprintf(fout, "%-s%*s |\n", my_cell,
								(int) (dwidth - strlen(my_cell)), "");
 					free(my_cell);
 				}
 				else
 				{
 					if (opt_border < 2)
 						fprintf(fout, "%s\n", dlineptr[line_count].ptr);
 					else
 						fprintf(fout, "%-s%*s |\n", dlineptr[line_count].ptr,
								dwidth - dlineptr[line_count].width, "");
 				}
 				
 				if (line_count == dheight - 1 || !dlineptr[line_count+1].ptr)
 					dcomplete = 1;
 			}
 			else
 			{
 				if (opt_border < 2)
	 				fputc('\n', fout);
	 			else
	 				fprintf(fout, "%*s |\n", dwidth, "");
 			}
 			line_count++;
 		}
	}

	if (opt_border == 2)
		fprintf(fout, "%s\n", divider);

	/* print footers */

	if (!opt_tuples_only && footers && *footers)
	{
		if (opt_border < 2)
			fputc('\n', fout);
		for (ptr = footers; *ptr; ptr++)
			fprintf(fout, "%s\n", *ptr);
	}

	fputc('\n', fout);
	free(divider);
	free(hlineptr->ptr);
	free(dlineptr->ptr);
	free(hlineptr);
	free(dlineptr);
}





/**********************/
/* HTML printing ******/
/**********************/


void
html_escaped_print(const char *in, FILE *fout)
{
	const char *p;
	bool		leading_space = true;

	for (p = in; *p; p++)
	{
		switch (*p)
		{
			case '&':
				fputs("&amp;", fout);
				break;
			case '<':
				fputs("&lt;", fout);
				break;
			case '>':
				fputs("&gt;", fout);
				break;
			case '\n':
				fputs("<br />\n", fout);
				break;
			case '"':
				fputs("&quot;", fout);
				break;
			case ' ':
				/* protect leading space, for EXPLAIN output */
				if (leading_space)
					fputs("&nbsp;", fout);
				else
					fputs(" ", fout);
				break;
			default:
				fputc(*p, fout);
		}
		if (*p != ' ')
			leading_space = false;
	}
}



static void
print_html_text(const char *title, const char *const * headers,
				const char *const * cells, const char *const * footers,
				const char *opt_align, bool opt_tuples_only,
				bool opt_numeric_locale, unsigned short int opt_border,
				const char *opt_table_attr, FILE *fout)
{
	unsigned int col_count = 0;
	unsigned int i;
	const char *const * ptr;

	fprintf(fout, "<table border=\"%d\"", opt_border);
	if (opt_table_attr)
		fprintf(fout, " %s", opt_table_attr);
	fputs(">\n", fout);

	/* print title */
	if (!opt_tuples_only && title)
	{
		fputs("  <caption>", fout);
		html_escaped_print(title, fout);
		fputs("</caption>\n", fout);
	}

	/* print headers and count columns */
	if (!opt_tuples_only)
		fputs("  <tr>\n", fout);
	for (i = 0, ptr = headers; *ptr; i++, ptr++)
	{
		col_count++;
		if (!opt_tuples_only)
		{
			fputs("    <th align=\"center\">", fout);
			html_escaped_print(*ptr, fout);
			fputs("</th>\n", fout);
		}
	}
	if (!opt_tuples_only)
		fputs("  </tr>\n", fout);

	/* print cells */
	for (i = 0, ptr = cells; *ptr; i++, ptr++)
	{
		if (i % col_count == 0)
			fputs("  <tr valign=\"top\">\n", fout);

		fprintf(fout, "    <td align=\"%s\">", opt_align[(i) % col_count] == 'r' ? "right" : "left");
		/* is string only whitespace? */
		if ((*ptr)[strspn(*ptr, " \t")] == '\0')
			fputs("&nbsp; ", fout);
		else if (opt_align[i % col_count] == 'r' && opt_numeric_locale)
		{
			char	   *my_cell = format_numeric_locale(*ptr);

			html_escaped_print(my_cell, fout);
			free(my_cell);
		}
		else
			html_escaped_print(*ptr, fout);

		fputs("</td>\n", fout);

		if ((i + 1) % col_count == 0)
			fputs("  </tr>\n", fout);
	}

	fputs("</table>\n", fout);

	/* print footers */

	if (!opt_tuples_only && footers && *footers)
	{
		fputs("<p>", fout);
		for (ptr = footers; *ptr; ptr++)
		{
			html_escaped_print(*ptr, fout);
			fputs("<br />\n", fout);
		}
		fputs("</p>", fout);
	}
	fputc('\n', fout);
}



static void
print_html_vertical(const char *title, const char *const * headers,
					const char *const * cells, const char *const * footers,
					const char *opt_align, bool opt_tuples_only,
					bool opt_numeric_locale, unsigned short int opt_border,
					const char *opt_table_attr, FILE *fout)
{
	unsigned int col_count = 0;
	unsigned int i;
	unsigned int record = 1;
	const char *const * ptr;

	fprintf(fout, "<table border=\"%d\"", opt_border);
	if (opt_table_attr)
		fprintf(fout, " %s", opt_table_attr);
	fputs(">\n", fout);

	/* print title */
	if (!opt_tuples_only && title)
	{
		fputs("  <caption>", fout);
		html_escaped_print(title, fout);
		fputs("</caption>\n", fout);
	}

	/* count columns */
	for (ptr = headers; *ptr; ptr++)
		col_count++;

	/* print records */
	for (i = 0, ptr = cells; *ptr; i++, ptr++)
	{
		if (i % col_count == 0)
		{
			if (!opt_tuples_only)
				fprintf(fout, "\n  <tr><td colspan=\"2\" align=\"center\">Record %d</td></tr>\n", record++);
			else
				fputs("\n  <tr><td colspan=\"2\">&nbsp;</td></tr>\n", fout);
		}
		fputs("  <tr valign=\"top\">\n"
			  "    <th>", fout);
		html_escaped_print(headers[i % col_count], fout);
		fputs("</th>\n", fout);

		fprintf(fout, "    <td align=\"%s\">", opt_align[i % col_count] == 'r' ? "right" : "left");
		/* is string only whitespace? */
		if ((*ptr)[strspn(*ptr, " \t")] == '\0')
			fputs("&nbsp; ", fout);
		else if (opt_align[i % col_count] == 'r' && opt_numeric_locale)
		{
			char	   *my_cell = format_numeric_locale(*ptr);

			html_escaped_print(my_cell, fout);
			free(my_cell);
		}
		else
			html_escaped_print(*ptr, fout);

		fputs("</td>\n  </tr>\n", fout);
	}

	fputs("</table>\n", fout);

	/* print footers */
	if (!opt_tuples_only && footers && *footers)
	{
		fputs("<p>", fout);
		for (ptr = footers; *ptr; ptr++)
		{
			html_escaped_print(*ptr, fout);
			fputs("<br />\n", fout);
		}
		fputs("</p>", fout);
	}
	fputc('\n', fout);
}



/*************************/
/* LaTeX		 */
/*************************/


static void
latex_escaped_print(const char *in, FILE *fout)
{
	const char *p;

	for (p = in; *p; p++)
		switch (*p)
		{
			case '&':
				fputs("\\&", fout);
				break;
			case '%':
				fputs("\\%", fout);
				break;
			case '$':
				fputs("\\$", fout);
				break;
			case '_':
				fputs("\\_", fout);
				break;
			case '{':
				fputs("\\{", fout);
				break;
			case '}':
				fputs("\\}", fout);
				break;
			case '\\':
				fputs("\\backslash", fout);
				break;
			case '\n':
				fputs("\\\\", fout);
				break;
			default:
				fputc(*p, fout);
		}
}



static void
print_latex_text(const char *title, const char *const * headers,
				 const char *const * cells, const char *const * footers,
				 const char *opt_align, bool opt_tuples_only,
				 bool opt_numeric_locale, unsigned short int opt_border,
				 FILE *fout)
{
	unsigned int col_count = 0;
	unsigned int i;
	const char *const * ptr;


	/* print title */
	if (!opt_tuples_only && title)
	{
		fputs("\\begin{center}\n", fout);
		latex_escaped_print(title, fout);
		fputs("\n\\end{center}\n\n", fout);
	}

	/* count columns */
	for (ptr = headers; *ptr; ptr++)
		col_count++;

	/* begin environment and set alignments and borders */
	fputs("\\begin{tabular}{", fout);

	if (opt_border == 2)
		fputs("| ", fout);
	for (i = 0; i < col_count; i++)
	{
		fputc(*(opt_align + i), fout);
		if (opt_border != 0 && i < col_count - 1)
			fputs(" | ", fout);
	}
	if (opt_border == 2)
		fputs(" |", fout);

	fputs("}\n", fout);

	if (!opt_tuples_only && opt_border == 2)
		fputs("\\hline\n", fout);

	/* print headers and count columns */
	for (i = 0, ptr = headers; i < col_count; i++, ptr++)
	{
		if (!opt_tuples_only)
		{
			if (i != 0)
				fputs(" & ", fout);
			fputs("\\textit{", fout);
			latex_escaped_print(*ptr, fout);
			fputc('}', fout);
		}
	}

	if (!opt_tuples_only)
	{
		fputs(" \\\\\n", fout);
		fputs("\\hline\n", fout);
	}

	/* print cells */
	for (i = 0, ptr = cells; *ptr; i++, ptr++)
	{
		if (opt_numeric_locale)
		{
			char	   *my_cell = format_numeric_locale(*ptr);

			latex_escaped_print(my_cell, fout);
			free(my_cell);
		}
		else
			latex_escaped_print(*ptr, fout);

		if ((i + 1) % col_count == 0)
			fputs(" \\\\\n", fout);
		else
			fputs(" & ", fout);
	}

	if (opt_border == 2)
		fputs("\\hline\n", fout);

	fputs("\\end{tabular}\n\n\\noindent ", fout);


	/* print footers */

	if (footers && !opt_tuples_only)
		for (ptr = footers; *ptr; ptr++)
		{
			latex_escaped_print(*ptr, fout);
			fputs(" \\\\\n", fout);
		}

	fputc('\n', fout);
}



static void
print_latex_vertical(const char *title, const char *const * headers,
					 const char *const * cells, const char *const * footers,
					 const char *opt_align, bool opt_tuples_only,
					 bool opt_numeric_locale, unsigned short int opt_border,
					 FILE *fout)
{
	unsigned int col_count = 0;
	unsigned int i;
	const char *const * ptr;
	unsigned int record = 1;

	(void) opt_align;			/* currently unused parameter */

	/* print title */
	if (!opt_tuples_only && title)
	{
		fputs("\\begin{center}\n", fout);
		latex_escaped_print(title, fout);
		fputs("\n\\end{center}\n\n", fout);
	}

	/* begin environment and set alignments and borders */
	fputs("\\begin{tabular}{", fout);
	if (opt_border == 0)
		fputs("cl", fout);
	else if (opt_border == 1)
		fputs("c|l", fout);
	else if (opt_border == 2)
		fputs("|c|l|", fout);
	fputs("}\n", fout);


	/* count columns */
	for (ptr = headers; *ptr; ptr++)
		col_count++;


	/* print records */
	for (i = 0, ptr = cells; *ptr; i++, ptr++)
	{
		/* new record */
		if (i % col_count == 0)
		{
			if (!opt_tuples_only)
			{
				if (opt_border == 2)
				{
					fputs("\\hline\n", fout);
					fprintf(fout, "\\multicolumn{2}{|c|}{\\textit{Record %d}} \\\\\n", record++);
				}
				else
					fprintf(fout, "\\multicolumn{2}{c}{\\textit{Record %d}} \\\\\n", record++);
			}
			if (opt_border >= 1)
				fputs("\\hline\n", fout);
		}

		latex_escaped_print(headers[i % col_count], fout);
		fputs(" & ", fout);
		latex_escaped_print(*ptr, fout);
		fputs(" \\\\\n", fout);
	}

	if (opt_border == 2)
		fputs("\\hline\n", fout);

	fputs("\\end{tabular}\n\n\\noindent ", fout);


	/* print footers */

	if (footers && !opt_tuples_only)
		for (ptr = footers; *ptr; ptr++)
		{
			if (opt_numeric_locale)
			{
				char	   *my_cell = format_numeric_locale(*ptr);

				latex_escaped_print(my_cell, fout);
				free(my_cell);
			}
			else
				latex_escaped_print(*ptr, fout);
			fputs(" \\\\\n", fout);
		}

	fputc('\n', fout);
}



/*************************/
/* Troff -ms		 */
/*************************/


static void
troff_ms_escaped_print(const char *in, FILE *fout)
{
	const char *p;

	for (p = in; *p; p++)
		switch (*p)
		{
			case '\\':
				fputs("\\(rs", fout);
				break;
			default:
				fputc(*p, fout);
		}
}



static void
print_troff_ms_text(const char *title, const char *const * headers,
					const char *const * cells, const char *const * footers,
					const char *opt_align, bool opt_tuples_only,
					bool opt_numeric_locale, unsigned short int opt_border,
					FILE *fout)
{
	unsigned int col_count = 0;
	unsigned int i;
	const char *const * ptr;


	/* print title */
	if (!opt_tuples_only && title)
	{
		fputs(".LP\n.DS C\n", fout);
		troff_ms_escaped_print(title, fout);
		fputs("\n.DE\n", fout);
	}

	/* count columns */
	for (ptr = headers; *ptr; ptr++)
		col_count++;

	/* begin environment and set alignments and borders */
	fputs(".LP\n.TS\n", fout);
	if (opt_border == 2)
		fputs("center box;\n", fout);
	else
		fputs("center;\n", fout);

	for (i = 0; i < col_count; i++)
	{
		fputc(*(opt_align + i), fout);
		if (opt_border > 0 && i < col_count - 1)
			fputs(" | ", fout);
	}
	fputs(".\n", fout);

	/* print headers and count columns */
	for (i = 0, ptr = headers; i < col_count; i++, ptr++)
	{
		if (!opt_tuples_only)
		{
			if (i != 0)
				fputc('\t', fout);
			fputs("\\fI", fout);
			troff_ms_escaped_print(*ptr, fout);
			fputs("\\fP", fout);
		}
	}

	if (!opt_tuples_only)
		fputs("\n_\n", fout);

	/* print cells */
	for (i = 0, ptr = cells; *ptr; i++, ptr++)
	{
		if (opt_numeric_locale)
		{
			char	   *my_cell = format_numeric_locale(*ptr);

			troff_ms_escaped_print(my_cell, fout);
			free(my_cell);
		}
		else
			troff_ms_escaped_print(*ptr, fout);

		if ((i + 1) % col_count == 0)
			fputc('\n', fout);
		else
			fputc('\t', fout);
	}

	fputs(".TE\n.DS L\n", fout);


	/* print footers */

	if (footers && !opt_tuples_only)
		for (ptr = footers; *ptr; ptr++)
		{
			troff_ms_escaped_print(*ptr, fout);
			fputc('\n', fout);
		}

	fputs(".DE\n", fout);
}



static void
print_troff_ms_vertical(const char *title, const char *const * headers,
					  const char *const * cells, const char *const * footers,
						const char *opt_align, bool opt_tuples_only,
					  bool opt_numeric_locale, unsigned short int opt_border,
						FILE *fout)
{
	unsigned int col_count = 0;
	unsigned int i;
	const char *const * ptr;
	unsigned int record = 1;
	unsigned short current_format = 0;	/* 0=none, 1=header, 2=body */

	(void) opt_align;			/* currently unused parameter */

	/* print title */
	if (!opt_tuples_only && title)
	{
		fputs(".LP\n.DS C\n", fout);
		troff_ms_escaped_print(title, fout);
		fputs("\n.DE\n", fout);
	}

	/* begin environment and set alignments and borders */
	fputs(".LP\n.TS\n", fout);
	if (opt_border == 2)
		fputs("center box;\n", fout);
	else
		fputs("center;\n", fout);

	/* basic format */
	if (opt_tuples_only)
		fputs("c l;\n", fout);

	/* count columns */
	for (ptr = headers; *ptr; ptr++)
		col_count++;

	/* print records */
	for (i = 0, ptr = cells; *ptr; i++, ptr++)
	{
		/* new record */
		if (i % col_count == 0)
		{
			if (!opt_tuples_only)
			{
				if (current_format != 1)
				{
					if (opt_border == 2 && i > 0)
						fputs("_\n", fout);
					if (current_format != 0)
						fputs(".T&\n", fout);
					fputs("c s.\n", fout);
					current_format = 1;
				}
				fprintf(fout, "\\fIRecord %d\\fP\n", record++);
			}
			if (opt_border >= 1)
				fputs("_\n", fout);
		}

		if (!opt_tuples_only)
		{
			if (current_format != 2)
			{
				if (current_format != 0)
					fputs(".T&\n", fout);
				if (opt_border != 1)
					fputs("c l.\n", fout);
				else
					fputs("c | l.\n", fout);
				current_format = 2;
			}
		}

		troff_ms_escaped_print(headers[i % col_count], fout);
		fputc('\t', fout);
		if (opt_numeric_locale)
		{
			char	   *my_cell = format_numeric_locale(*ptr);

			troff_ms_escaped_print(my_cell, fout);
			free(my_cell);
		}
		else
			troff_ms_escaped_print(*ptr, fout);

		fputc('\n', fout);
	}

	fputs(".TE\n.DS L\n", fout);


	/* print footers */

	if (footers && !opt_tuples_only)
		for (ptr = footers; *ptr; ptr++)
		{
			troff_ms_escaped_print(*ptr, fout);
			fputc('\n', fout);
		}

	fputs(".DE\n", fout);
}



/********************************/
/* Public functions		*/
/********************************/


/*
 * PageOutput
 *
 * Tests if pager is needed and returns appropriate FILE pointer.
 */
FILE *
PageOutput(int lines, unsigned short int pager)
{
	/* check whether we need / can / are supposed to use pager */
	if (pager
#ifndef WIN32
		&&
		isatty(fileno(stdin)) &&
		isatty(fileno(stdout))
#endif
		)
	{
		const char *pagerprog;

#ifdef TIOCGWINSZ
		int			result;
		struct winsize screen_size;

		result = ioctl(fileno(stdout), TIOCGWINSZ, &screen_size);

		/* >= accounts for a one-line prompt */
		if (result == -1 || lines >= screen_size.ws_row || pager > 1)
		{
#endif
			pagerprog = getenv("PAGER");
			if (!pagerprog)
				pagerprog = DEFAULT_PAGER;
#ifndef WIN32
			pqsignal(SIGPIPE, SIG_IGN);
#endif
			return popen(pagerprog, "w");
#ifdef TIOCGWINSZ
		}
#endif
	}

	return stdout;
}



void
printTable(const char *title,
		   const char *const * headers,
		   const char *const * cells,
		   const char *const * footers,
		   const char *align,
		   const printTableOpt *opt, FILE *fout, FILE *flog)
{
	const char *default_footer[] = {NULL};
	unsigned short int border = opt->border;
	FILE	   *output;
	bool		use_expanded;

	if (opt->format == PRINT_NOTHING)
		return;

	if (!footers)
		footers = default_footer;

	if (opt->format != PRINT_HTML && border > 2)
		border = 2;

	/*
	 * We only want to display the results in "expanded" format if this is a
	 * normal (user-submitted) query, not a table we're printing for a slash
	 * command.
	 */
	if (opt->expanded)
		use_expanded = true;
	else
		use_expanded = false;

	if (fout == stdout)
	{
		int			col_count = 0,
					row_count = 0,
					lines;
		const char *const * ptr;

		/* rough estimate of columns and rows */
		if (headers)
			for (ptr = headers; *ptr; ptr++)
				col_count++;
		if (cells)
			for (ptr = cells; *ptr; ptr++)
				row_count++;
		if (col_count > 0)
			row_count /= col_count;

		if (opt->expanded)
			lines = (col_count + 1) * row_count;
		else
			lines = row_count + 1;
		if (footers && !opt->tuples_only)
			for (ptr = footers; *ptr; ptr++)
				lines++;
		output = PageOutput(lines, opt->pager);
	}
	else
		output = fout;

	/* print the stuff */

	if (flog)
		print_aligned_text(title, headers, cells, footers, align, opt->tuples_only, opt->numericLocale, border, opt->encoding, flog);

	switch (opt->format)
	{
		case PRINT_UNALIGNED:
			if (use_expanded)
				print_unaligned_vertical(title, headers, cells, footers, align,
										 opt->fieldSep, opt->recordSep,
							   opt->tuples_only, opt->numericLocale, output);
			else
				print_unaligned_text(title, headers, cells, footers, align,
									 opt->fieldSep, opt->recordSep,
							   opt->tuples_only, opt->numericLocale, output);
			break;
		case PRINT_ALIGNED:
			if (use_expanded)
				print_aligned_vertical(title, headers, cells, footers, align,
								opt->tuples_only, opt->numericLocale, border,
									   opt->encoding, output);
			else
				print_aligned_text(title, headers, cells, footers, align,
								   opt->tuples_only, opt->numericLocale,
								   border, opt->encoding, output);
			break;
		case PRINT_HTML:
			if (use_expanded)
				print_html_vertical(title, headers, cells, footers, align,
									opt->tuples_only, opt->numericLocale,
									border, opt->tableAttr, output);
			else
				print_html_text(title, headers, cells, footers,
						 align, opt->tuples_only, opt->numericLocale, border,
								opt->tableAttr, output);
			break;
		case PRINT_LATEX:
			if (use_expanded)
				print_latex_vertical(title, headers, cells, footers, align,
									 opt->tuples_only, opt->numericLocale,
									 border, output);
			else
				print_latex_text(title, headers, cells, footers, align,
								 opt->tuples_only, opt->numericLocale,
								 border, output);
			break;
		case PRINT_TROFF_MS:
			if (use_expanded)
				print_troff_ms_vertical(title, headers, cells, footers, align,
										opt->tuples_only, opt->numericLocale,
										border, output);
			else
				print_troff_ms_text(title, headers, cells, footers, align,
									opt->tuples_only, opt->numericLocale,
									border, output);
			break;
		default:
			fprintf(stderr, _("invalid output format (internal error): %d"), opt->format);
			exit(EXIT_FAILURE);
	}

	/* Only close if we used the pager */
	if (fout == stdout && output != stdout)
	{
		pclose(output);
#ifndef WIN32
		pqsignal(SIGPIPE, SIG_DFL);
#endif
	}
}



void
printQuery(const PGresult *result, const printQueryOpt *opt, FILE *fout, FILE *flog)
{
	int			nfields;
	int			ncells;
	const char **headers;
	const char **cells;
	char	  **footers;
	char	   *align;
	int			i;

	/* extract headers */
	nfields = PQnfields(result);

	headers = pg_local_calloc(nfields + 1, sizeof(*headers));

	for (i = 0; i < nfields; i++)
		headers[i] = (char*) mbvalidate((unsigned char *) PQfname(result, i),
										opt->topt.encoding);

	/* set cells */
	ncells = PQntuples(result) * nfields;
	cells = pg_local_calloc(ncells + 1, sizeof(*cells));

	for (i = 0; i < ncells; i++)
	{
		if (PQgetisnull(result, i / nfields, i % nfields))
			cells[i] = opt->nullPrint ? opt->nullPrint : "";
		else
			cells[i] = (char*)
				mbvalidate((unsigned char*) PQgetvalue(result, i / nfields, i % nfields),
						   opt->topt.encoding);
	}

	/* set footers */

	if (opt->footers)
		footers = opt->footers;
	else if (!opt->topt.expanded && opt->default_footer)
	{
		footers = pg_local_calloc(2, sizeof(*footers));

		footers[0] = pg_local_malloc(100);
		if (PQntuples(result) == 1)
			snprintf(footers[0], 100, _("(1 row)"));
		else
			snprintf(footers[0], 100, _("(%d rows)"), PQntuples(result));
	}
	else
		footers = NULL;

	/* set alignment */
	align = pg_local_calloc(nfields + 1, sizeof(*align));

	for (i = 0; i < nfields; i++)
	{
		Oid			ftype = PQftype(result, i);

		if (ftype == 20 ||		/* int8 */
			ftype == 21 ||		/* int2 */
			ftype == 23 ||		/* int4 */
			(ftype >= 26 && ftype <= 30) ||		/* ?id */
			ftype == 700 ||		/* float4 */
			ftype == 701 ||		/* float8 */
			ftype == 790 ||		/* money */
			ftype == 1700		/* numeric */
			)
			align[i] = 'r';
		else
			align[i] = 'l';
	}

	/* call table printer */
	printTable(opt->title, headers, cells,
			   (const char *const *) footers,
			   align, &opt->topt, fout, flog);

	free(headers);
	free(cells);
	if (footers)
	{
		free(footers[0]);
		free(footers);
	}
	free(align);
}


void
setDecimalLocale(void)
{
	struct lconv *extlconv;

	extlconv = localeconv();

	if (*extlconv->decimal_point)
		decimal_point = strdup(extlconv->decimal_point);
	else
		decimal_point = ".";	/* SQL output standard */
	if (*extlconv->grouping && atoi(extlconv->grouping) > 0)
		grouping = strdup(extlconv->grouping);
	else
		grouping = "3";			/* most common */
	if (*extlconv->thousands_sep)
		thousands_sep = strdup(extlconv->thousands_sep);
	else if (*decimal_point != ',')
		thousands_sep = ",";
	else
		thousands_sep = ".";
}
