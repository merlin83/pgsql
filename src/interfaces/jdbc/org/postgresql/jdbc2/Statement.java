package org.postgresql.jdbc2;

// IMPORTANT NOTE: This file implements the JDBC 2 version of the driver.
// If you make any modifications to this file, you must make sure that the
// changes are also made (if relevent) to the related JDBC 1 class in the
// org.postgresql.jdbc1 package.

import java.sql.*;
import java.util.Vector;
import org.postgresql.util.*;

/**
 * A Statement object is used for executing a static SQL statement and
 * obtaining the results produced by it.
 *
 * <p>Only one ResultSet per Statement can be open at any point in time.
 * Therefore, if the reading of one ResultSet is interleaved with the
 * reading of another, each must have been generated by different
 * Statements.  All statement execute methods implicitly close a
 * statement's current ResultSet if an open one exists.
 *
 * @see java.sql.Statement
 * @see ResultSet
 */
public class Statement extends org.postgresql.Statement implements java.sql.Statement
{
    private Connection connection;		// The connection who created us
    private Vector batch=null;
    private int resultsettype;                // the resultset type to return
    private int concurrency;         // is it updateable or not?

	/**
	 * Constructor for a Statement.  It simply sets the connection
	 * that created us.
	 *
	 * @param c the Connection instantation that creates us
	 */
	public Statement (Connection c)
	{
		connection = c;
                resultsettype = java.sql.ResultSet.TYPE_SCROLL_INSENSITIVE;
                concurrency = java.sql.ResultSet.CONCUR_READ_ONLY;
	}

	/**
	 * Execute a SQL statement that retruns a single ResultSet
	 *
	 * @param sql typically a static SQL SELECT statement
	 * @return a ResulSet that contains the data produced by the query
	 * @exception SQLException if a database access error occurs
	 */
	public java.sql.ResultSet executeQuery(String sql) throws SQLException
	{
	    this.execute(sql);
	    while (result != null && !((org.postgresql.ResultSet)result).reallyResultSet())
		result = ((org.postgresql.ResultSet)result).getNext();
	    if (result == null)
		throw new PSQLException("postgresql.stat.noresult");
	    return result;
	}

	/**
	 * Execute a SQL INSERT, UPDATE or DELETE statement.  In addition
	 * SQL statements that return nothing such as SQL DDL statements
	 * can be executed
	 *
	 * @param sql a SQL statement
	 * @return either a row count, or 0 for SQL commands
	 * @exception SQLException if a database access error occurs
	 */
	public int executeUpdate(String sql) throws SQLException
	{
		this.execute(sql);
		if (((org.postgresql.ResultSet)result).reallyResultSet())
			throw new PSQLException("postgresql.stat.result");
		return this.getUpdateCount();
	}

	/**
	 * setCursorName defines the SQL cursor name that will be used by
	 * subsequent execute methods.  This name can then be used in SQL
	 * positioned update/delete statements to identify the current row
	 * in the ResultSet generated by this statement.  If a database
	 * doesn't support positioned update/delete, this method is a
	 * no-op.
	 *
	 * <p><B>Note:</B> By definition, positioned update/delete execution
	 * must be done by a different Statement than the one which
	 * generated the ResultSet being used for positioning.  Also, cursor
	 * names must be unique within a Connection.
	 *
	 * <p>We throw an additional constriction.  There can only be one
	 * cursor active at any one time.
	 *
	 * @param name the new cursor name
	 * @exception SQLException if a database access error occurs
	 */
	public void setCursorName(String name) throws SQLException
	{
		connection.setCursorName(name);
	}

	/**
	 * Execute a SQL statement that may return multiple results. We
	 * don't have to worry about this since we do not support multiple
	 * ResultSets.   You can use getResultSet or getUpdateCount to
	 * retrieve the result.
	 *
	 * @param sql any SQL statement
	 * @return true if the next result is a ResulSet, false if it is
	 * 	an update count or there are no more results
	 * @exception SQLException if a database access error occurs
	 */
    public boolean execute(String sql) throws SQLException
    {
	if (escapeProcessing)
	    sql = escapeSQL(sql);

        // New in 7.1, if we have a previous resultset then force it to close
        // This brings us nearer to compliance, and helps memory management.
        // Internal stuff will call ExecSQL directly, bypassing this.
        if(result!=null) {
          java.sql.ResultSet rs = getResultSet();
          if(rs!=null)
            rs.close();
        }

        // New in 7.1, pass Statement so that ExecSQL can customise to it
	result = connection.ExecSQL(sql,this);

        // New in 7.1, required for ResultSet.getStatement() to work
        ((org.postgresql.jdbc2.ResultSet)result).setStatement(this);

	return (result != null && ((org.postgresql.ResultSet)result).reallyResultSet());
    }

	/**
	 * getUpdateCount returns the current result as an update count,
	 * if the result is a ResultSet or there are no more results, -1
	 * is returned.  It should only be called once per result.
	 *
	 * @return the current result as an update count.
	 * @exception SQLException if a database access error occurs
	 */
	public int getUpdateCount() throws SQLException
	{
		if (result == null) 		return -1;
		if (((org.postgresql.ResultSet)result).reallyResultSet())	return -1;
		return ((org.postgresql.ResultSet)result).getResultCount();
	}

	/**
	 * getMoreResults moves to a Statement's next result.  If it returns
	 * true, this result is a ResulSet.
	 *
	 * @return true if the next ResultSet is valid
	 * @exception SQLException if a database access error occurs
	 */
	public boolean getMoreResults() throws SQLException
	{
		result = ((org.postgresql.ResultSet)result).getNext();
		return (result != null && ((org.postgresql.ResultSet)result).reallyResultSet());
	}

    // ** JDBC 2 Extensions **

    public void addBatch(String sql) throws SQLException
    {
	if(batch==null)
	    batch=new Vector();
	batch.addElement(sql);
    }

    public void clearBatch() throws SQLException
    {
	if(batch!=null)
	    batch.removeAllElements();
    }

    public int[] executeBatch() throws SQLException
    {
	if(batch==null || batch.isEmpty())
	    throw new PSQLException("postgresql.stat.batch.empty");

	int size=batch.size();
	int[] result=new int[size];
	int i=0;
	this.execute("begin"); // PTM: check this when autoCommit is false
	try {
	    for(i=0;i<size;i++)
		result[i]=this.executeUpdate((String)batch.elementAt(i));
	    this.execute("commit"); // PTM: check this
	} catch(SQLException e) {
	    this.execute("abort"); // PTM: check this
	    throw new PSQLException("postgresql.stat.batch.error",new Integer(i),batch.elementAt(i));
	}
	return result;
    }

    public java.sql.Connection getConnection() throws SQLException
    {
	return (java.sql.Connection)connection;
    }

    public int getFetchDirection() throws SQLException
    {
      throw new PSQLException("postgresql.psqlnotimp");
    }

    public int getFetchSize() throws SQLException
    {
      // This one can only return a valid value when were a cursor?
	throw org.postgresql.Driver.notImplemented();
    }

    public int getResultSetConcurrency() throws SQLException
    {
      // new in 7.1
      return concurrency;
    }

    public int getResultSetType() throws SQLException
    {
      // new in 7.1
      return resultsettype;
    }

    public void setFetchDirection(int direction) throws SQLException
    {
	throw org.postgresql.Driver.notImplemented();
    }

    public void setFetchSize(int rows) throws SQLException
    {
	throw org.postgresql.Driver.notImplemented();
    }

    /**
     * New in 7.1
     */
    public void setResultSetConcurrency(int value) throws SQLException
    {
      concurrency=value;
    }

    /**
     * New in 7.1
     */
    public void setResultSetType(int value) throws SQLException
    {
      resultsettype=value;
    }
}
