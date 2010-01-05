#! /usr/bin/perl -w
#-------------------------------------------------------------------------
#
# Gen_fmgrtab.pl
#    Perl script that generates fmgroids.h and fmgrtab.c from pg_proc.h
#
# Portions Copyright (c) 1996-2010, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
#
# IDENTIFICATION
#    $PostgreSQL: pgsql/src/backend/utils/Gen_fmgrtab.pl,v 1.4 2010/01/05 01:06:56 tgl Exp $
#
#-------------------------------------------------------------------------

use Catalog;

use strict;
use warnings;

# Collect arguments
my $infile;        # pg_proc.h
my $output_path = '';
while (@ARGV)
{
    my $arg = shift @ARGV;
    if ($arg !~ /^-/)
    {
        $infile = $arg;
    }
    elsif ($arg =~ /^-o/)
    {
        $output_path = length($arg) > 2 ? substr($arg, 2) : shift @ARGV;
    }
    else
    {
        usage();
    }
}

# Make sure output_path ends in a slash.
if ($output_path ne '' && substr($output_path, -1) ne '/')
{
    $output_path .= '/';
}

# Read all the data from the include/catalog files.
my $catalogs = Catalog::Catalogs($infile);

# Collect the raw data from pg_proc.h.
my @fmgr = ();
my @attnames;
foreach my $column ( @{ $catalogs->{pg_proc}->{columns} } )
{
    push @attnames, keys %$column;
}

my $data = $catalogs->{pg_proc}->{data};
foreach my $row (@$data)
{

    # To construct fmgroids.h and fmgrtab.c, we need to inspect some
    # of the individual data fields.  Just splitting on whitespace
    # won't work, because some quoted fields might contain internal
    # whitespace.  We handle this by folding them all to a simple
    # "xxx". Fortunately, this script doesn't need to look at any
    # fields that might need quoting, so this simple hack is
    # sufficient.
    $row->{bki_values} =~ s/"[^"]*"/"xxx"/g;
    @{$row}{@attnames} = split /\s+/, $row->{bki_values};

    # Select out just the rows for internal-language procedures.
    # Note assumption here that INTERNALlanguageId is 12.
    next if $row->{prolang} ne '12';

    push @fmgr,
      {
        oid    => $row->{oid},
        strict => $row->{proisstrict},
        retset => $row->{proretset},
        nargs  => $row->{pronargs},
        prosrc => $row->{prosrc},
      };
}

# Emit headers for both files
open H, '>', $output_path . 'fmgroids.h.tmp' || die "Could not open fmgroids.h.tmp: $!";
print H 
qq|/*-------------------------------------------------------------------------
 *
 * fmgroids.h
 *    Macros that define the OIDs of built-in functions.
 *
 * These macros can be used to avoid a catalog lookup when a specific
 * fmgr-callable function needs to be referenced.
 *
 * Portions Copyright (c) 1996-2010, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * NOTES
 *	******************************
 *	*** DO NOT EDIT THIS FILE! ***
 *	******************************
 *
 *	It has been GENERATED by $0
 *	from $infile
 *
 *-------------------------------------------------------------------------
 */
#ifndef FMGROIDS_H
#define FMGROIDS_H

/*
 *	Constant macros for the OIDs of entries in pg_proc.
 *
 *	NOTE: macros are named after the prosrc value, ie the actual C name
 *	of the implementing function, not the proname which may be overloaded.
 *	For example, we want to be able to assign different macro names to both
 *	char_text() and name_text() even though these both appear with proname
 *	'text'.  If the same C function appears in more than one pg_proc entry,
 *	its equivalent macro will be defined with the lowest OID among those
 *	entries.
 */
|;

open T, '>', $output_path . 'fmgrtab.c.tmp' || die "Could not open fmgrtab.c.tmp: $!";
print T
qq|/*-------------------------------------------------------------------------
 *
 * fmgrtab.c
 *    The function manager's table of internal functions.
 *
 * Portions Copyright (c) 1996-2010, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * NOTES
 *
 *	******************************
 *	*** DO NOT EDIT THIS FILE! ***
 *	******************************
 *
 *	It has been GENERATED by $0
 *	from $infile
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "utils/fmgrtab.h"

|;

# Emit #define's and extern's -- only one per prosrc value
my %seenit;
foreach my $s (sort {$a->{oid} <=> $b->{oid}} @fmgr)
{
    next if $seenit{$s->{prosrc}};
    $seenit{$s->{prosrc}} = 1;
    print H "#define F_" . uc $s->{prosrc} . " $s->{oid}\n";
    print T "extern Datum $s->{prosrc} (PG_FUNCTION_ARGS);\n";
}

# Create the fmgr_builtins table
print T "\nconst FmgrBuiltin fmgr_builtins[] = {\n";
my %bmap;
$bmap{'t'} = 'true';
$bmap{'f'} = 'false';
foreach my $s (sort {$a->{oid} <=> $b->{oid}} @fmgr)
{
    print T
	"  { $s->{oid}, \"$s->{prosrc}\", $s->{nargs}, $bmap{$s->{strict}}, $bmap{$s->{retset}}, $s->{prosrc} },\n";
}

# And add the file footers.
print H "\n#endif /* FMGROIDS_H */\n";
close(H);

print T
qq|  /* dummy entry is easier than getting rid of comma after last real one */
  /* (not that there has ever been anything wrong with *having* a
     comma after the last field in an array initializer) */
  { 0, NULL, 0, false, false, NULL }
};

/* Note fmgr_nbuiltins excludes the dummy entry */
const int fmgr_nbuiltins = (sizeof(fmgr_builtins) / sizeof(FmgrBuiltin)) - 1;
|;

close(T);

# Finally, rename the completed files into place.
Catalog::RenameTempFile($output_path . 'fmgroids.h');
Catalog::RenameTempFile($output_path . 'fmgrtab.c');

sub usage
{
    die <<EOM;
Usage: perl -I [directory of Catalog.pm] Gen_fmgrtab.pl [path to pg_proc.h]

Gen_fmgrtab.pl generates fmgroids.h and fmgrtab.c from pg_proc.h

Report bugs to <pgsql-bugs\@postgresql.org>.
EOM
}

exit 0;
