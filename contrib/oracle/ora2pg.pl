#!/usr/bin/perl
#------------------------------------------------------------------------------
# Project  : Oracle2Postgresql
# Name     : ora2pg.pl
# Language : 5.006 built for i686-linux
# OS       : linux RedHat 6.2 kernel 2.2.14-5
# Author   : Gilles Darold, gilles@darold.net
# Copyright: Copyright (c) 2000 : Gilles Darold - All rights reserved -
# Function : Script used to convert Oracle Database schema to PostgreSQL
#------------------------------------------------------------------------------
# Version  : 1.0
#------------------------------------------------------------------------------

BEGIN {
        $ENV{ORACLE_HOME} = '/usr/local/oracle/oracle816';
}

use strict;

use Ora2Pg;

# Initialyze the database connection
my $dbsrc = 'dbi:Oracle:host=test.mydomain.com;sid=TEST;port=1521';
my $dbuser = 'system';
my $dbpwd = 'manager';

# Create an instance of the XSD::DBISchema perl module
my $schema = new Ora2Pg (
	datasource => $dbsrc,		# Database DBD datasource
	user => $dbuser,		# Database user
	password => $dbpwd,		# Database password
	debug => 1,			# Verbose mode
#	type => 'VIEW',			# Extract views
#	tables => [('MY_TABLE1','MY_TABLE2')],	# Extract only these table
#	showtableid => 1,		# Display only table indice during extraction
#	min => 1,			# Extract begin at indice 1
#	max => 10			# Extract ended at indice 10
);

# Create the POSTGRESQL representation of all objects in the database
$schema->export_schema("output.sql");

exit(0);

