package postgresql;

import java.sql.*;

/**
 * @version 1.0 15-APR-1997
 * @author <A HREF="mailto:adrian@hottub.org">Adrian Hall</A>
 *
 * This class provides information about the database as a whole.
 *
 * Many of the methods here return lists of information in ResultSets.  You
 * can use the normal ResultSet methods such as getString and getInt to 
 * retrieve the data from these ResultSets.  If a given form of metadata is
 * not available, these methods should throw a SQLException.
 *
 * Some of these methods take arguments that are String patterns.  These
 * arguments all have names such as fooPattern.  Within a pattern String,
 * "%" means match any substring of 0 or more characters, and "_" means
 * match any one character.  Only metadata entries matching the search
 * pattern are returned.  if a search pattern argument is set to a null
 * ref, it means that argument's criteria should be dropped from the
 * search.
 *
 * A SQLException will be throws if a driver does not support a meta
 * data method.  In the case of methods that return a ResultSet, either
 * a ResultSet (which may be empty) is returned or a SQLException is
 * thrown.
 *
 * @see java.sql.DatabaseMetaData
 */
public class DatabaseMetaData implements java.sql.DatabaseMetaData 
{
	Connection connection;		// The connection association

	public DatabaseMetaData(Connection conn)
	{
		this.connection = conn;
	}

	/**
	 * Can all the procedures returned by getProcedures be called
	 * by the current user?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean allProceduresAreCallable() throws SQLException
	{
		return true;		// For now...
	}

	/**
	 * Can all the tables returned by getTable be SELECTed by
	 * the current user?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean allTablesAreSelectable() throws SQLException
	{
		return true;		// For now...
	}

	/**
	 * What is the URL for this database?
	 *
	 * @return the url or null if it cannott be generated
	 * @exception SQLException if a database access error occurs
	 */
	public String getURL() throws SQLException
	{
		return connection.getURL();
	}

	/**
	 * What is our user name as known to the database?
	 *
	 * @return our database user name
	 * @exception SQLException if a database access error occurs
	 */
	public String getUserName() throws SQLException
	{
		return connection.getUserName();
	}

	/**
	 * Is the database in read-only mode?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean isReadOnly() throws SQLException
	{
		return connection.isReadOnly();
	}

	/**
	 * Are NULL values sorted high?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean nullsAreSortedHigh() throws SQLException
	{
		return false;
	}

	/**
	 * Are NULL values sorted low?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean nullsAreSortedLow() throws SQLException
	{
		return false;
	}

	/**
	 * Are NULL values sorted at the start regardless of sort order?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean nullsAreSortedAtStart() throws SQLException
	{
		return false;
	}

	/**
	 * Are NULL values sorted at the end regardless of sort order?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean nullsAreSortedAtEnd() throws SQLException
	{
		return true;
	}

	/**
	 * What is the name of this database product - we hope that it is
	 * PostgreSQL, so we return that explicitly.
	 *
	 * @return the database product name
	 * @exception SQLException if a database access error occurs
	 */
	public String getDatabaseProductName() throws SQLException
	{
		return new String("PostgreSQL");
	}

	/**
	 * What is the version of this database product.  Note that
	 * PostgreSQL 6.1 has a system catalog called pg_version - 
	 * however, select * from pg_version on any database retrieves
	 * no rows.  For now, we will return the version 6.1 (in the
	 * hopes that we change this driver as often as we change the
	 * database)
	 *
	 * @return the database version
	 * @exception SQLException if a database access error occurs
	 */
	public String getDatabaseProductVersion() throws SQLException
	{
		return ("6.1");
	}

	/**
	 * What is the name of this JDBC driver?  If we don't know this
	 * we are doing something wrong!
	 *
	 * @return the JDBC driver name
	 * @exception SQLException why?
	 */
	public String getDriverName() throws SQLException
	{
		return new String("PostgreSQL Native Driver");
	}

	/**
	 * What is the version string of this JDBC driver?  Again, this is
	 * static.
	 *
	 * @return the JDBC driver name.
	 * @exception SQLException why?
	 */
	public String getDriverVersion() throws SQLException
	{
		return new String("1.0");
	}

	/**
	 * What is this JDBC driver's major version number?
	 *
	 * @return the JDBC driver major version
	 */
	public int getDriverMajorVersion()
	{
		return 1;
	}

	/**
	 * What is this JDBC driver's minor version number?
	 *
	 * @return the JDBC driver minor version
	 */
	public int getDriverMinorVersion()
	{
		return 0;
	}

	/**
	 * Does the database store tables in a local file?  No - it
	 * stores them in a file on the server.
	 * 
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean usesLocalFiles() throws SQLException
	{
		return false;
	}

	/**
	 * Does the database use a file for each table?  Well, not really,
	 * since it doesnt use local files. 
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean usesLocalFilePerTable() throws SQLException
	{
		return false;
	}

	/**
	 * Does the database treat mixed case unquoted SQL identifiers
	 * as case sensitive and as a result store them in mixed case?
	 * A JDBC-Compliant driver will always return false.
	 *
	 * Predicament - what do they mean by "SQL identifiers" - if it
	 * means the names of the tables and columns, then the answers
	 * given below are correct - otherwise I don't know.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsMixedCaseIdentifiers() throws SQLException
	{
		return true;
	}

	/**
	 * Does the database treat mixed case unquoted SQL identifiers as
	 * case insensitive and store them in upper case?
	 *
	 * @return true if so
	 */
	public boolean storesUpperCaseIdentifiers() throws SQLException
	{
		return false;
	}

	/**
	 * Does the database treat mixed case unquoted SQL identifiers as
	 * case insensitive and store them in lower case?
	 *
	 * @return true if so
	 */
	public boolean storesLowerCaseIdentifiers() throws SQLException
	{
		return false;
	}

	/**
	 * Does the database treat mixed case unquoted SQL identifiers as
	 * case insensitive and store them in mixed case?
	 *
	 * @return true if so
	 */
	public boolean storesMixedCaseIdentifiers() throws SQLException
	{
		return false;
	}

	/**
	 * Does the database treat mixed case quoted SQL identifiers as
	 * case sensitive and as a result store them in mixed case?  A
	 * JDBC compliant driver will always return true. 
	 *
	 * Predicament - what do they mean by "SQL identifiers" - if it
	 * means the names of the tables and columns, then the answers
	 * given below are correct - otherwise I don't know.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsMixedCaseQuotedIdentifiers() throws SQLException
	{
		return true;
	}

	/**
	 * Does the database treat mixed case quoted SQL identifiers as
	 * case insensitive and store them in upper case?
	 *
	 * @return true if so
	 */
	public boolean storesUpperCaseQuotedIdentifiers() throws SQLException
	{
		return false;
	}

	/**
	 * Does the database treat mixed case quoted SQL identifiers as case
	 * insensitive and store them in lower case?
	 *
	 * @return true if so
	 */
	public boolean storesLowerCaseQuotedIdentifiers() throws SQLException
	{
		return false;
	}

	/**
	 * Does the database treat mixed case quoted SQL identifiers as case
	 * insensitive and store them in mixed case?
	 *
	 * @return true if so
	 */
	public boolean storesMixedCaseQuotedIdentifiers() throws SQLException
	{
		return false;
	}

	/**
	 * What is the string used to quote SQL identifiers?  This returns
	 * a space if identifier quoting isn't supported.  A JDBC Compliant
	 * driver will always use a double quote character.
	 *
	 * If an SQL identifier is a table name, column name, etc. then
	 * we do not support it.
	 *
	 * @return the quoting string
	 * @exception SQLException if a database access error occurs
	 */
	public String getIdentifierQuoteString() throws SQLException
	{
		return new String(" ");
	}

	/**
	 * Get a comma separated list of all a database's SQL keywords that
	 * are NOT also SQL92 keywords.
	 *
	 * Within PostgreSQL, the keywords are found in
	 * 	src/backend/parser/keywords.c
	 * For SQL Keywords, I took the list provided at
	 * 	http://web.dementia.org/~shadow/sql/sql3bnf.sep93.txt
	 * which is for SQL3, not SQL-92, but it is close enough for
	 * this purpose.
	 *
	 * @return a comma separated list of keywords we use
	 * @exception SQLException if a database access error occurs
	 */
	public String getSQLKeywords() throws SQLException
	{
		return new String("abort,acl,add,aggregate,append,archive,arch_store,backward,binary,change,cluster,copy,database,delimiters,do,extend,explain,forward,heavy,index,inherits,isnull,light,listen,load,merge,nothing,notify,notnull,oids,purge,rename,replace,retrieve,returns,rule,recipe,setof,stdin,stdout,store,vacuum,verbose,version");
	}

	public String getNumericFunctions() throws SQLException
	{
		// XXX-Not Implemented
	}

	public String getStringFunctions() throws SQLException
	{
		// XXX-Not Implemented
	}

	public String getSystemFunctions() throws SQLException
	{
		// XXX-Not Implemented
	}

	public String getTimeDateFunctions() throws SQLException
	{
		// XXX-Not Implemented
	}

	/**
	 * This is the string that can be used to escape '_' and '%' in
	 * a search string pattern style catalog search parameters
	 *
	 * @return the string used to escape wildcard characters
	 * @exception SQLException if a database access error occurs
	 */
	public String getSearchStringEscape() throws SQLException
	{
		return new String("\\");
	}

	/**
	 * Get all the "extra" characters that can bew used in unquoted
	 * identifier names (those beyond a-zA-Z0-9 and _)
	 *
	 * From the file src/backend/parser/scan.l, an identifier is
	 * {letter}{letter_or_digit} which makes it just those listed
	 * above.
	 *
	 * @return a string containing the extra characters
	 * @exception SQLException if a database access error occurs
	 */
	public String getExtraNameCharacters() throws SQLException
	{
		return new String("");
	}

	/**
	 * Is "ALTER TABLE" with an add column supported?
	 * Yes for PostgreSQL 6.1
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsAlterTableWithAddColumn() throws SQLException
	{
		return true;
	}

	/**
	 * Is "ALTER TABLE" with a drop column supported?
	 * Yes for PostgreSQL 6.1
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsAlterTableWithDropColumn() throws SQLException
	{
		return true;
	}

	/**
	 * Is column aliasing supported?
	 *
	 * If so, the SQL AS clause can be used to provide names for
	 * computed columns or to provide alias names for columns as
	 * required.  A JDBC Compliant driver always returns true.
	 *
	 * e.g.
	 *
	 * select count(C) as C_COUNT from T group by C;
	 *
	 * should return a column named as C_COUNT instead of count(C)
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsColumnAliasing() throws SQLException
	{
		return true;
	}

	/**
	 * Are concatenations between NULL and non-NULL values NULL?  A
	 * JDBC Compliant driver always returns true
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean nullPlusNonNullIsNull() throws SQLException
	{
		return true;
	}

	public boolean supportsConvert() throws SQLException
	{
		// XXX-Not Implemented
	}

	public boolean supportsConvert(int fromType, int toType) throws SQLException
	{
		// XXX-Not Implemented
	}

	public boolean supportsTableCorrelationNames() throws SQLException
	{
		// XXX-Not Implemented
	}

	public boolean supportsDifferentTableCorrelationNames() throws SQLException
	{
		// XXX-Not Implemented
	}

	/**
	 * Are expressions in "ORCER BY" lists supported?
	 * 
	 * e.g. select * from t order by a + b;
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsExpressionsInOrderBy() throws SQLException
	{
		return false;
	}

	/**
	 * Can an "ORDER BY" clause use columns not in the SELECT?
	 * I checked it, and you can't.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsOrderByUnrelated() throws SQLException
	{
		return false;
	}

	/**
	 * Is some form of "GROUP BY" clause supported?
	 * I checked it, and yes it is.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsGroupBy() throws SQLException
	{
		return true;
	}

	/**
	 * Can a "GROUP BY" clause use columns not in the SELECT?
	 * I checked it - it seems to allow it
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsGroupByUnrelated() throws SQLException
	{
		return true;
	}

	/**
	 * Can a "GROUP BY" clause add columns not in the SELECT provided
	 * it specifies all the columns in the SELECT?  Does anyone actually
	 * understand what they mean here?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsGroupByBeyondSelect() throws SQLException
	{
		return true;		// For now...
	}

	/**
	 * Is the escape character in "LIKE" clauses supported?  A
	 * JDBC compliant driver always returns true.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsLikeEscapeClause() throws SQLException
	{
		return true;
	}

	/**
	 * Are multiple ResultSets from a single execute supported?
	 * Well, I implemented it, but I dont think this is possible from
	 * the back ends point of view.
	 * 
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsMultipleResultSets() throws SQLException
	{
		return false;
	}

	/**
	 * Can we have multiple transactions open at once (on different
	 * connections?)
	 * I guess we can have, since Im relying on it.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsMultipleTransactions() throws SQLException
	{
		return true;
	}

	/**
	 * Can columns be defined as non-nullable.  A JDBC Compliant driver
	 * always returns true.  We dont support NOT NULL, so we are not
	 * JDBC compliant.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsNonNullableColumns() throws SQLException
	{
		return false;
	}

	/**
	 * Does this driver support the minimum ODBC SQL grammar.  This
	 * grammar is defined at:
	 *
	 * http://www.microsoft.com/msdn/sdk/platforms/doc/odbc/src/intropr.htm
	 *
	 * In Appendix C.  From this description, we seem to support the
	 * ODBC minimal (Level 0) grammar.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsMinimumSQLGrammar() throws SQLException
	{
		return true;
	}

	/**
	 * Does this driver support the Core ODBC SQL grammar.  We need
	 * SQL-92 conformance for this.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsCoreSQLGrammar() throws SQLException
	{
		return false;
	}
	
	/**
	 * Does this driver support the Extended (Level 2) ODBC SQL
	 * grammar.  We don't conform to the Core (Level 1), so we can't
	 * conform to the Extended SQL Grammar.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsExtendedSQLGrammar() throws SQLException
	{
		return false;
	}

	/**
	 * Does this driver support the ANSI-92 entry level SQL grammar?
	 * All JDBC Compliant drivers must return true.  I think we have
	 * to support outer joins for this to be true.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsANSI92EntryLevelSQL() throws SQLException
	{
		return false;
	}

	/**
	 * Does this driver support the ANSI-92 intermediate level SQL
	 * grammar?  Anyone who does not support Entry level cannot support
	 * Intermediate level.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsANSI92IntermediateSQL() throws SQLException
	{
		return false;
	}

	/**
	 * Does this driver support the ANSI-92 full SQL grammar?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsANSI92FullSQL() throws SQLException
	{
		return false;
	}

	/**
	 * Is the SQL Integrity Enhancement Facility supported?
	 * I haven't seen this mentioned anywhere, so I guess not
	 * 
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsIntegrityEnhancementFacility() throws SQLException
	{
		return false;
	}

	/**
	 * Is some form of outer join supported?  From my knowledge, nope.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsOuterJoins() throws SQLException
	{
		return false;
	}

	/**
	 * Are full nexted outer joins supported?  Well, we dont support any
	 * form of outer join, so this is no as well
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsFullOuterJoins() throws SQLException
	{
		return false;
	}

	/**
	 * Is there limited support for outer joins?  (This will be true if
	 * supportFullOuterJoins is true)
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsLimitedOuterJoins() throws SQLException
	{
		return false;
	}

	/**
	 * What is the database vendor's preferred term for "schema" - well,
	 * we do not provide support for schemas, so lets just use that
	 * term.
	 *
	 * @return the vendor term
	 * @exception SQLException if a database access error occurs
	 */
	public String getSchemaTerm() throws SQLException
	{
		return new String("Schema");
	}

	/**
	 * What is the database vendor's preferred term for "procedure" - 
	 * I kind of like "Procedure" myself.
	 *
	 * @return the vendor term
	 * @exception SQLException if a database access error occurs
	 */
	public String getProcedureTerm() throws SQLException
	{
		return new String("Procedure");
	}

	/**
	 * What is the database vendor's preferred term for "catalog"? -
	 * we dont have a preferred term, so just use Catalog
	 *
	 * @return the vendor term
	 * @exception SQLException if a database access error occurs
	 */
	public String getCatalogTerm() throws SQLException
	{
		return new String("Catalog");
	}

	/**
	 * Does a catalog appear at the start of a qualified table name?
	 * (Otherwise it appears at the end).
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean isCatalogAtStart() throws SQLException
	{
		return false;
	}

	/**
	 * What is the Catalog separator.  Hmmm....well, I kind of like
	 * a period (so we get catalog.table definitions). - I don't think
	 * PostgreSQL supports catalogs anyhow, so it makes no difference.
	 *
	 * @return the catalog separator string
	 * @exception SQLException if a database access error occurs
	 */
	public String getCatalogSeparator() throws SQLException
	{
		return new String(".");
	}

	/**
	 * Can a schema name be used in a data manipulation statement?  Nope.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsSchemasInDataManipulation() throws SQLException
	{
		return false;
	}

	/**
	 * Can a schema name be used in a procedure call statement?  Nope.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsSchemasInProcedureCalls() throws SQLException
	{
		return false;
	}

	/**
	 * Can a schema be used in a table definition statement?  Nope.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsSchemasInTableDefinitions() throws SQLException
	{
		return false;
	}

	/**
	 * Can a schema name be used in an index definition statement?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsSchemasInIndexDefinitions() throws SQLException
	{
		return false;
	}

	/**
	 * Can a schema name be used in a privilege definition statement?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsSchemasInPrivilegeDefinitions() throws SQLException
	{
		return false;
	}

	/**
	 * Can a catalog name be used in a data manipulation statement?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsCatalogsInDataManipulation() throws SQLException
	{
		return false;
	}

	/**
	 * Can a catalog name be used in a procedure call statement?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsCatalogsInProcedureCalls() throws SQLException
	{
		return false;
	}

	/**
	 * Can a catalog name be used in a table definition statement?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsCatalogsInTableDefinitions() throws SQLException
	{
		return false;
	}

	/**
	 * Can a catalog name be used in an index definition?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsCatalogsInIndexDefinitions() throws SQLException
	{
		return false;
	}

	/**
	 * Can a catalog name be used in a privilege definition statement?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsCatalogsInPrivilegeDefinitions() throws SQLException
	{
		return false;
	}

	/**
	 * We support cursors for gets only it seems.  I dont see a method
	 * to get a positioned delete.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsPositionedDelete() throws SQLException
	{
		return false;			// For now...
	}

	/**
	 * Is positioned UPDATE supported?
	 * 
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsPositionedUpdate() throws SQLException
	{
		return false;			// For now...
	}

	public boolean supportsSelectForUpdate() throws SQLException
	{
		// XXX-Not Implemented
	}

	public boolean supportsStoredProcedures() throws SQLException
	{
		// XXX-Not Implemented
	}

	public boolean supportsSubqueriesInComparisons() throws SQLException
	{
		// XXX-Not Implemented
	}

	public boolean supportsSubqueriesInExists() throws SQLException
	{
		// XXX-Not Implemented
	}

	public boolean supportsSubqueriesInIns() throws SQLException
	{
		// XXX-Not Implemented
	}

	public boolean supportsSubqueriesInQuantifieds() throws SQLException
	{
		// XXX-Not Implemented
	}

	public boolean supportsCorrelatedSubqueries() throws SQLException
	{
		// XXX-Not Implemented
	}

	/**
	 * Is SQL UNION supported?  Nope.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsUnion() throws SQLException
	{
		return false;
	}

	/**
	 * Is SQL UNION ALL supported?  Nope.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsUnionAll() throws SQLException
	{
		return false;
	}

	/**
	 * In PostgreSQL, Cursors are only open within transactions.
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsOpenCursorsAcrossCommit() throws SQLException
	{
		return false;
	}

	/**
	 * Do we support open cursors across multiple transactions?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsOpenCursorsAcrossRollback() throws SQLException
	{
		return false;
	}

	/**
	 * Can statements remain open across commits?  They may, but
	 * this driver cannot guarentee that.  In further reflection.
	 * we are talking a Statement object jere, so the answer is
	 * yes, since the Statement is only a vehicle to ExecSQL()
	 *
	 * @return true if they always remain open; false otherwise
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsOpenStatementsAcrossCommit() throws SQLException
	{
		return true;
	}

	/**
	 * Can statements remain open across rollbacks?  They may, but
	 * this driver cannot guarentee that.  In further contemplation,
	 * we are talking a Statement object here, so the answer is yes,
	 * since the Statement is only a vehicle to ExecSQL() in Connection
	 *
	 * @return true if they always remain open; false otherwise
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsOpenStatementsAcrossRollback() throws SQLException
	{
		return true;
	}

	/**
	 * How many hex characters can you have in an inline binary literal
	 *
	 * @return the max literal length
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxBinaryLiteralLength() throws SQLException
	{
		return 0;				// For now...
	}

	/**
	 * What is the maximum length for a character literal
	 * I suppose it is 8190 (8192 - 2 for the quotes)
	 *
	 * @return the max literal length
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxCharLiteralLength() throws SQLException
	{
		return 8190;
	}

	/**
	 * Whats the limit on column name length.  The description of
	 * pg_class would say '32' (length of pg_class.relname) - we
	 * should probably do a query for this....but....
	 *
	 * @return the maximum column name length
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxColumnNameLength() throws SQLException
	{
		return 32;
	}

	/**
	 * What is the maximum number of columns in a "GROUP BY" clause?
	 *
	 * @return the max number of columns
	 * @exception SQLException if a database access error occurs	
	 */
	public int getMaxColumnsInGroupBy() throws SQLException
	{
		return getMaxColumnsInTable();
	}

	/**
	 * What's the maximum number of columns allowed in an index?
	 * 6.0 only allowed one column, but 6.1 introduced multi-column
	 * indices, so, theoretically, its all of them.
	 *
	 * @return max number of columns
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxColumnsInIndex() throws SQLException
	{
		return getMaxColumnsInTable();
	}

	/**
	 * What's the maximum number of columns in an "ORDER BY clause?
	 * Theoretically, all of them!
	 *
	 * @return the max columns
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxColumnsInOrderBy() throws SQLException
	{
		return getMaxColumnsInTable();
	}

	/**
	 * What is the maximum number of columns in a "SELECT" list?
	 * Theoretically, all of them!
	 *
	 * @return the max columns
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxColumnsInSelect() throws SQLException
	{
		return getMaxColumnsInTable();
	}

	/**
	 * What is the maximum number of columns in a table? From the
	 * create_table(l) manual page...
	 *
	 * "The new class is created as a heap with no initial data.  A
	 * class can have no more than 1600 attributes (realistically,
	 * this is limited by the fact that tuple sizes must be less than
	 * 8192 bytes)..."
	 *
	 * @return the max columns
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxColumnsInTable() throws SQLException
	{
		return 1600;
	}

	/**
	 * How many active connection can we have at a time to this
	 * database?  Well, since it depends on postmaster, which just
	 * does a listen() followed by an accept() and fork(), its
	 * basically very high.  Unless the system runs out of processes,
	 * it can be 65535 (the number of aux. ports on a TCP/IP system).
	 * I will return 8192 since that is what even the largest system
	 * can realistically handle,
	 *
	 * @return the maximum number of connections
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxConnections() throws SQLException
	{
		return 8192;
	}

	/**
	 * What is the maximum cursor name length (the same as all
	 * the other F***** identifiers!)
	 *
	 * @return max cursor name length in bytes
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxCursorNameLength() throws SQLException
	{
		return 32;
	}

	/**
	 * What is the maximum length of an index (in bytes)?  Now, does
	 * the spec. mean name of an index (in which case its 32, the 
	 * same as a table) or does it mean length of an index element
	 * (in which case its 8192, the size of a row) or does it mean
	 * the number of rows it can access (in which case it 2^32 - 
	 * a 4 byte OID number)?  I think its the length of an index
	 * element, personally, so Im setting it to 8192.
	 *
	 * @return max index length in bytes
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxIndexLength() throws SQLException
	{
		return 8192;
	}

	public int getMaxSchemaNameLength() throws SQLException
	{
		// XXX-Not Implemented
	}

	/**
	 * What is the maximum length of a procedure name?
	 * (length of pg_proc.proname used) - again, I really
	 * should do a query here to get it.
	 *
	 * @return the max name length in bytes
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxProcedureNameLength() throws SQLException
	{
		return 32;
	}

	public int getMaxCatalogNameLength() throws SQLException
	{
		// XXX-Not Implemented
	}

	/**
	 * What is the maximum length of a single row?  (not including
	 * blobs).  8192 is defined in PostgreSQL.
	 *
	 * @return max row size in bytes
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxRowSize() throws SQLException
	{
		return 8192;
	}

	/**
	 * Did getMaxRowSize() include LONGVARCHAR and LONGVARBINARY
	 * blobs?  We don't handle blobs yet
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean doesMaxRowSizeIncludeBlobs() throws SQLException
	{
		return false;
	}

	/**
	 * What is the maximum length of a SQL statement?
	 *
	 * @return max length in bytes
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxStatementLength() throws SQLException
	{
		return 8192;
	}

	/**
	 * How many active statements can we have open at one time to
	 * this database?  Basically, since each Statement downloads
	 * the results as the query is executed, we can have many.  However,
	 * we can only really have one statement per connection going
	 * at once (since they are executed serially) - so we return
	 * one.
	 *
	 * @return the maximum
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxStatements() throws SQLException
	{
		return 1;
	}

	/**
	 * What is the maximum length of a table name?  This was found
	 * from pg_class.relname length
	 *
	 * @return max name length in bytes
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxTableNameLength() throws SQLException
	{
		return 32;
	}

	/**
	 * What is the maximum number of tables that can be specified
	 * in a SELECT?  Theoretically, this is the same number as the
	 * number of tables allowable.  In practice tho, it is much smaller
	 * since the number of tables is limited by the statement, we
	 * return 1024 here - this is just a number I came up with (being
	 * the number of tables roughly of three characters each that you
	 * can fit inside a 8192 character buffer with comma separators).
	 *
	 * @return the maximum
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxTablesInSelect() throws SQLException
	{
		return 1024;
	}

	/**
	 * What is the maximum length of a user name?  Well, we generally
	 * use UNIX like user names in PostgreSQL, so I think this would
	 * be 8.  However, showing the schema for pg_user shows a length
	 * for username of 32.
	 *
	 * @return the max name length in bytes
	 * @exception SQLException if a database access error occurs
	 */
	public int getMaxUserNameLength() throws SQLException
	{
		return 32;
	}

	
	/**
	 * What is the database's default transaction isolation level?  We
	 * do not support this, so all transactions are SERIALIZABLE.
	 *
	 * @return the default isolation level
	 * @exception SQLException if a database access error occurs
	 * @see Connection
	 */
	public int getDefaultTransactionIsolation() throws SQLException
	{
		return Connection.TRANSACTION_SERIALIZABLE;
	}

	/**
	 * Are transactions supported?  If not, commit and rollback are noops
	 * and the isolation level is TRANSACTION_NONE.  We do support
	 * transactions.	
	 *
	 * @return true if transactions are supported
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsTransactions() throws SQLException
	{
		return true;
	}

	/**
	 * Does the database support the given transaction isolation level?
	 * We only support TRANSACTION_SERIALIZABLE
	 * 
	 * @param level the values are defined in java.sql.Connection
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 * @see Connection
	 */
	public boolean supportsTransactionIsolationLevel(int level) throws SQLException
	{
		if (level == Connection.TRANSACTION_SERIALIZABLE)
			return true;
		else
			return false;
	}

	/**
	 * Are both data definition and data manipulation transactions 
	 * supported?  I checked it, and could not do a CREATE TABLE
	 * within a transaction, so I am assuming that we don't
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsDataDefinitionAndDataManipulationTransactions() throws SQLException
	{
		return false;
	}

	/**
	 * Are only data manipulation statements withing a transaction
	 * supported?
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean supportsDataManipulationTransactionsOnly() throws SQLException
	{
		return true;
	}

	/**
	 * Does a data definition statement within a transaction force
	 * the transaction to commit?  I think this means something like:
	 *
	 * CREATE TABLE T (A INT);
	 * INSERT INTO T (A) VALUES (2);
	 * BEGIN;
	 * UPDATE T SET A = A + 1;
	 * CREATE TABLE X (A INT);
	 * SELECT A FROM T INTO X;
	 * COMMIT;
	 *
	 * does the CREATE TABLE call cause a commit?  The answer is no.  
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean dataDefinitionCausesTransactionCommit() throws SQLException
	{
		return false;
	}

	/**
	 *  Is a data definition statement within a transaction ignored?
	 * It seems to be (from experiment in previous method)
	 *
	 * @return true if so
	 * @exception SQLException if a database access error occurs
	 */
	public boolean dataDefinitionIgnoredInTransactions() throws SQLException
	{
		return false;
	}

	/**
	 * Get a description of stored procedures available in a catalog
	 * 
	 * Only procedure descriptions matching the schema and procedure
	 * name criteria are returned.  They are ordered by PROCEDURE_SCHEM
	 * and PROCEDURE_NAME
	 *
	 * Each procedure description has the following columns:
	 * PROCEDURE_CAT String => procedure catalog (may be null)
	 * PROCEDURE_SCHEM String => procedure schema (may be null)
	 * PROCEDURE_NAME String => procedure name
	 * Field 4 reserved (make it null)
	 * Field 5 reserved (make it null)
	 * Field 6 reserved (make it null)
	 * REMARKS String => explanatory comment on the procedure
	 * PROCEDURE_TYPE short => kind of procedure
	 *	* procedureResultUnknown - May return a result
	 * 	* procedureNoResult - Does not return a result
	 *	* procedureReturnsResult - Returns a result
	 *
	 * @param catalog - a catalog name; "" retrieves those without a
	 *	catalog; null means drop catalog name from criteria
	 * @param schemaParrern - a schema name pattern; "" retrieves those
	 *	without a schema - we ignore this parameter
	 * @param procedureNamePattern - a procedure name pattern
	 * @return ResultSet - each row is a procedure description
	 * @exception SQLException if a database access error occurs
	 */
	public java.sql.ResultSet getProcedures(String catalog, String schemaPattern, String procedureNamePattern) throws SQLException
	{
		Field[] f = new Field[8];		// the field descriptors for the new ResultSet
		static final int iVarcharOid = 1043;	// This is the OID for a varchar()
		static final int iInt2Oid = 21;		// This is the OID for an int2
		ResultSet r;				// ResultSet for the SQL query that we need to do
		Vector v;				// The new ResultSet tuple stuff
		String remarks = new String("no remarks");

		Field[0] = new Field(conn, new String("PROCEDURE_CAT"), iVarcharOid, 32);
		Field[1] = new Field(conn, new String("PROCEDURE_SCHEM"), iVarcharOid, 32);
		Field[2] = new Field(conn, new String("PROCEDURE_NAME"), iVarcharOid, 32);
		Field[3] = null;
		Field[4] = null;
		Field[5] = null;
		Field[6] = new Field(conn, new String("REMARKS"), iVarcharOid, 8192);
		Field[7] = new Field(conn, new String("PROCEDURE_TYPE"), iInt2Oid, 2);
		r = conn.ExecSQL("select proname, proretset from pg_proc order by proname");
		if (r.getColumnCount() != 2 || r.getTupleCount() <= 1)
			throw new SQLException("Unexpected return from query for procedure list");
		while (r.next())
		{
			byte[][] tuple = new byte[8][0];

			String name = r.getString(1);
			String remarks = new String("no remarks");
			boolean retset = r.getBoolean(2);
	
			byte[0] = null;			// Catalog name
			byte[1] = null;			// Schema name
			byte[2] = name.getBytes();	// Procedure name
			byte[3] = null;			// Reserved
			byte[4] = null;			// Reserved
			byte[5] = null;			// Reserved
			byte[6] = remarks.getBytes();	// Remarks
			if (retset)
				byte[7] = procedureReturnsResult;
			else
				byte[7] = procedureNoResult;
			v.addElement(byte);
		}			
		return new ResultSet(conn, f, v, "OK", 1);
	}

	public java.sql.ResultSet getProcedureColumns(String catalog, String schemaPattern, String procedureNamePattern, String columnNamePattern) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getTables(String catalog, String schemaPattern, String tableNamePattern, String types[]) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getSchemas() throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getCatalogs() throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getTableTypes() throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getColumns(String catalog, String schemaPattern, String tableNamePattern, String columnNamePattern) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getColumnPrivileges(String catalog, String schema, String table, String columnNamePattern) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getTablePrivileges(String catalog, String schemaPattern, String tableNamePattern) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getBestRowIdentifier(String catalog, String schema, String table, int scope, boolean nullable) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getVersionColumns(String catalog, String schema, String table) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getPrimaryKeys(String catalog, String schema, String table) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getImportedKeys(String catalog, String schema, String table) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getExportedKeys(String catalog, String schema, String table) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getCrossReference(String primaryCatalog, String primarySchema, String primaryTable, String foreignCatalog, String foreignSchema, String foreignTable) throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getTypeInfo() throws SQLException
	{
		// XXX-Not Implemented
	}

	public java.sql.ResultSet getIndexInfo(String catalog, String schema, String table, boolean unique, boolean approximate) throws SQLException
	{
		// XXX-Not Implemented
	}
}
