#! /usr/bin/perl
#
# Copyright 2001 by PostgreSQL Global Development Group
#
# $Id: UCS_to_SJIS.pl,v 1.1 2000-10-30 10:40:29 ishii Exp $
#
# Generate UTF-8 <--> SJIS code conversion tables from
# map files provided by Unicode organization.
# Unfortunately it is prohibited by the organization
# to distribute the map files. So if you try to use this script,
# you have to obtain SHIFTJIS.TXT from 
# the organization's ftp site.
#
# SHIFTJIS.TXT format:
#		 SHIFTJIS code in hex
#		 UCS-2 code in hex
#		 # and Unicode name (not used in this script)
# Warning: SHIFTJIS.TXT contains only JIS0201 and JIS0208. no JIS0212.

require "ucs2utf.pl";

# first generate UTF-8 --> SJIS table

$in_file = "SHIFTJIS.TXT";

open( FILE, $in_file ) || die( "cannot open $in_file" );

while( <FILE> ){
	chop;
	if( /^#/ ){
		next;
	}
	( $c, $u, $rest ) = split;
	$ucs = hex($u);
	$code = hex($c);
	if( $code >= 0x80 && $ucs >= 0x100 ){
		$utf = &ucs2utf($ucs);
		if( $array{ $utf } ne "" ){
			printf STDERR "Warning: duplicate unicode: %04x\n",$ucs;
			next;
		}
		$count++;

		$array{ $utf } = $code;
	}
}
close( FILE );

#
# first, generate UTF8 --> SJIS table
#

$file = "utf8_to_sjis.map";
open( FILE, "> $file" ) || die( "cannot open $file" );
print FILE "static pg_utf_to_local ULmapSJIS[ $count ] = {\n";

for $index ( sort {$a <=> $b} keys( %array ) ){
	$code = $array{ $index };
	$count--;
	if( $count == 0 ){
		printf FILE "  {0x%04x, 0x%04x}\n", $index, $code;
	} else {
		printf FILE "  {0x%04x, 0x%04x},\n", $index, $code;
	}
}

print FILE "};\n";
close(FILE);

#
# then generate EUC_JP --> UTF8 table
#

open( FILE, $in_file ) || die( "cannot open $in_file" );

reset 'array';

while( <FILE> ){
	chop;
	if( /^#/ ){
		next;
	}
	( $c, $u, $rest ) = split;
	$ucs = hex($u);
	$code = hex($c);
	if( $code >= 0x80 && $ucs >= 0x100 ){
		$utf = &ucs2utf($ucs);
		if( $array{ $code } ne "" ){
			printf STDERR "Warning: duplicate code: %04x\n",$ucs;
			next;
		}
		$count++;

		$array{ $code } = $utf;
	}
}
close( FILE );

$file = "sjis_to_utf8.map";
open( FILE, "> $file" ) || die( "cannot open $file" );
print FILE "static pg_local_to_utf LUmapSJIS[ $count ] = {\n";
for $index ( sort {$a <=> $b} keys( %array ) ){
	$utf = $array{ $index };
	$count--;
	if( $count == 0 ){
		printf FILE "  {0x%04x, 0x%04x}\n", $index, $utf;
	} else {
		printf FILE "  {0x%04x, 0x%04x},\n", $index, $utf;
	}
}

print FILE "};\n";
close(FILE);
