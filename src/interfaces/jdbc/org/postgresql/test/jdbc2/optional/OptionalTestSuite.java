package org.postgresql.test.jdbc2.optional;

import junit.framework.TestSuite;

/**
 * Test suite for the JDBC 2.0 Optional Package implementation.  This
 * includes the DataSource, ConnectionPoolDataSource, and
 * PooledConnection implementations.
 *
 * @author Aaron Mulder (ammulder@chariotsolutions.com)
 * @version $Revision: 1.3 $
 */
public class OptionalTestSuite extends TestSuite
{
	/**
	 * Gets the test suite for the entire JDBC 2.0 Optional Package
	 * implementation.
	 */
	public static TestSuite suite()
	{
		TestSuite suite = new TestSuite();
		suite.addTestSuite(SimpleDataSourceTest.class);
		suite.addTestSuite(ConnectionPoolTest.class);
        suite.addTestSuite(ConnectionPoolTest.class);
		return suite;
	}
}
