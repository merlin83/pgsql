#! /usr/bin/perl

#################################################################
# create_help.pl -- converts SGML docs to internal psql help
#
# Copyright (c) 2000-2003, PostgreSQL Global Development Group
#
# $PostgreSQL: pgsql/src/bin/psql/create_help.pl,v 1.11 2004/06/11 03:56:46 momjian Exp $
#################################################################

#
# This script automatically generates the help on SQL in psql from
# the SGML docs. So far the format of the docs was consistent
# enough that this worked, but this here is by no means an SGML
# parser.
#
# Call: perl create_help.pl docdir sql_help.h
# The name of the header file doesn't matter to this script, but it
# sure does matter to the rest of the source.
#

$docdir = $ARGV[0] || die "$0: missing required argument: docdir\n";
$outputfile = $ARGV[1] || die "$0: missing required argument: output file\n";

if ($outputfile =~ m!.*/([^/]+)$!) {
    $outputfilebasename = $1;
}
else {
    $outputfilebasename = $outputfile;
}

$define = $outputfilebasename;
$define =~ tr/a-z/A-Z/;
$define =~ s/\W/_/g;

opendir(DIR, $docdir)
    || die "$0: could not open documentation source dir '$docdir': $!\n";
open(OUT, ">$outputfile")
    || die "$0: could not open output file '$outputfile': $!\n";

print OUT
"/*
 * *** Do not change this file by hand. It is automatically
 * *** generated from the DocBook documentation.
 *
 * generated by
 *     $^X $0 @ARGV
 *
 */

#ifndef $define
#define $define

#define N_(x) (x) /* gettext noop */

struct _helpStruct
{
    char	   *cmd;	   /* the command name */
    char	   *help;	   /* the help associated with it */
    char	   *syntax;	   /* the syntax associated with it */
};


static struct _helpStruct QL_HELP[] = {
";

$count = 0;

foreach $file (sort readdir DIR) {
    local ($cmdname, $cmddesc, $cmdsynopsis);
    $file =~ /\.sgml$/ || next;

    open(FILE, "$docdir/$file") || next;
    $filecontent = join('', <FILE>);
    close FILE;

    # Ignore files that are not for SQL language statements
    $filecontent =~ m!<refmiscinfo>\s*SQL - Language Statements\s*</refmiscinfo>!i
	|| next;

    # Extract <refname>, <refpurpose>, and <synopsis> fields, taking the
    # first one if there are more than one.  NOTE: we cannot just say
    # "<synopsis>(.*)</synopsis>", because that will match the first
    # occurrence of <synopsis> and the last one of </synopsis>!  Under
    # Perl 5 we could use a non-greedy wildcard, .*?, to ensure we match
    # the first </synopsis>, but we want this script to run under Perl 4
    # too, and Perl 4 hasn't got that feature.  So, do it the hard way.
    # Also, use [\000-\377] where we want to match anything including
    # newline --- Perl 4 does not have Perl 5's /s modifier.
    $filecontent =~ m!<refname>\s*([a-z ]*[a-z])\s*</refname>!i && ($cmdname = $1);
    if ($filecontent =~ m!<refpurpose>\s*([\000-\377]+)$!i) {
	$tmp = $1;		# everything after first <refpurpose>
	if ($tmp =~ s!\s*</refpurpose>[\000-\377]*$!!i) {
	    $cmddesc = $tmp;
	}
    }
    if ($filecontent =~ m!<synopsis>\s*([\000-\377]+)$!i) {
	$tmp = $1;		# everything after first <synopsis>
	if ($tmp =~ s!\s*</synopsis>[\000-\377]*$!!i) {
	    $cmdsynopsis = $tmp;
	}
    }

    if ($cmdname && $cmddesc && $cmdsynopsis) {
        $cmdname =~ s/\"/\\"/g;

	$cmddesc =~ s/<[^>]+>//g;
	$cmddesc =~ s/\s+/ /g;
        $cmddesc =~ s/\"/\\"/g;

	$cmdsynopsis =~ s/<[^>]+>//g;
	$cmdsynopsis =~ s/\r?\n/\\n/g;
        $cmdsynopsis =~ s/\"/\\"/g;

	print OUT "    { \"$cmdname\",\n      N_(\"$cmddesc\"),\n      N_(\"$cmdsynopsis\") },\n\n";
        $count++;
    }
    else {
	print STDERR "$0: parsing file '$file' failed (N='$cmdname' D='$cmddesc')\n";
    }
}

print OUT "
    { NULL, NULL, NULL }    /* End of list marker */
};


#define QL_HELP_COUNT $count


#endif /* $define */
";

close OUT;
closedir DIR;
