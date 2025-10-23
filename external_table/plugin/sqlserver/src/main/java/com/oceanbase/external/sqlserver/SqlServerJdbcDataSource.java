// Copyright (c) 2025 OceanBase.
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

package com.oceanbase.external.sqlserver;

import com.oceanbase.external.jdbc.JdbcDataSource;
import com.oceanbase.external.jdbc.JdbcTypeMapping;
import com.oceanbase.external.jdbc.QueryBuilder;
import org.apache.arrow.adapter.jdbc.JdbcFieldInfo;
import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.vector.ipc.ArrowReader;
import org.apache.arrow.vector.types.Types;
import org.apache.arrow.vector.types.pojo.ArrowType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.Calendar;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;
import java.util.function.Function;

import static java.sql.Types.DATE;
import static java.sql.Types.TIME;
import static java.sql.Types.TIMESTAMP;
import static java.sql.Types.LONGVARCHAR;
import static java.sql.Types.LONGVARBINARY;
import static java.sql.Types.BLOB;
import static java.sql.Types.BINARY;
import static java.sql.Types.VARBINARY;


@SuppressWarnings("unused")
public class SqlServerJdbcDataSource extends JdbcDataSource {
    private final static Logger logger = LoggerFactory.getLogger(SqlServerJdbcDataSource.class);
    private final static String DRIVER_CLASS_NAME = "com.microsoft.sqlserver.jdbc.SQLServerDriver";

    public SqlServerJdbcDataSource(BufferAllocator allocator, Map<String, String> properties) {
        super(allocator, properties);
    }

    @Override
    public ArrowReader createScanner(Map<String, Object> scanParameterMap) throws IOException {
        logger.debug("Loading driver...");
        try {
            Class.forName(DRIVER_CLASS_NAME);
        } catch (ClassNotFoundException e) {
            throw new IOException(e);
        }
        logger.debug("Driver loaded, calling super.createScanner...");
        try {
            ArrowReader reader = super.createScanner(scanParameterMap);
            logger.debug("Scanner created successfully!");
            return reader;
        } catch (Exception e) {
            logger.debug("Failed in super.createScanner: {}", e.getMessage());
            throw e;
        }
    }

    @Override
    protected QueryBuilder getQueryBuilder() {
        // Don't use quotes for SQL Server - let it handle identifiers naturally
        return new QueryBuilder("");
    }

    @Override
    protected Connection getConnection() throws SQLException {
        // ref to Microsoft SQL Server JDBC driver documentation for property details.
        Map<String, String> defaultProperties = new HashMap<String, String>() {{
            put("socketTimeout", "3600000"); // millisecond - timeout for read operations
            put("loginTimeout", "30"); // second - timeout for connection establishment
            put("sendStringParametersAsUnicode", "true"); // ensure proper Unicode handling
            // Critical: trustServerCertificate=true to avoid SSL handshake issues
            // This matches the behavior of sqlcmd which doesn't validate certificates by default
            put("trustServerCertificate", "true");
            put("encrypt", "false"); // disable encryption to match sqlcmd default behavior
            // Enable adaptive response buffering for streaming large result sets
            put("responseBuffering", "adaptive");
        }};
        Properties connProperties = new Properties();
        if (config.user != null) {
            connProperties.put("user", config.user);
        }
        if (config.password != null) {
            connProperties.put("password", config.password);
        }
        if (!config.jdbc_url.contains("transparentNetworkIPResolution")) {
            connProperties.put("transparentNetworkIPResolution", "false");
        }
        defaultProperties.forEach((key, value) -> {
            if (!config.jdbc_url.contains(key)) {
                connProperties.put(key, value);
            }
        });
        return DriverManager.getConnection(config.jdbc_url, connProperties);
    }

    @Override
    public void setOptimalFetchSize(Statement statement, Connection connection) throws SQLException {
        // SQL Server specific streaming optimization
        // Use larger fetch size with responseBuffering=adaptive for better performance
        statement.setFetchSize(2048);
        logger.info("Set SQL Server optimized fetch size: 2048 (with responseBuffering=adaptive)");
    }

    @Override
    protected Function<JdbcFieldInfo, ArrowType> getReadTypeMapping(Calendar calendar) {
        return (jdbcFieldInfo -> {
            switch (jdbcFieldInfo.getJdbcType()) {
                // SQL Server datetime types - handle precision and timezone offset issues
                case DATE:
                case TIME:
                case TIMESTAMP:
                    return Types.MinorType.VARCHAR.getType();
                
                // SQL Server specific types that don't have direct JDBC equivalents
                case -155: // DATETIMEOFFSET - SQL Server specific
                case -151: // DATETIME2 - SQL Server specific  
                case -154: // TIME - SQL Server specific with higher precision
                    return Types.MinorType.VARCHAR.getType();
                
                // SQL Server spatial and special types - handle as string
                case -157: // GEOGRAPHY - SQL Server spatial type
                case -158: // GEOMETRY - SQL Server spatial type
                case -150: // SQL_VARIANT - SQL Server variant type
                case -152: // HIERARCHYID - SQL Server hierarchical type
                    return Types.MinorType.VARCHAR.getType();
                
                // XML data type
                case 2009: // SQLXML
                    return Types.MinorType.VARCHAR.getType();
                
                // UNIQUEIDENTIFIER (GUID) - handle as string for compatibility
                case -11: // UNIQUEIDENTIFIER when mapped as string
                    return Types.MinorType.VARCHAR.getType();
                
                // Binary data types - convert to hex string for readability  
                case BINARY:        // Fixed-length binary data
                case VARBINARY:     // Variable-length binary data
                case LONGVARBINARY: // IMAGE type (deprecated)
                case BLOB:          // Binary large object
                    return Types.MinorType.VARCHAR.getType();
                
                // Text types
                case LONGVARCHAR:   // TEXT type (deprecated)
                    return Types.MinorType.VARCHAR.getType();

                default: 
                    return JdbcTypeMapping.getDefaultTypeMapping(calendar).apply(jdbcFieldInfo);
            }
        });
    }
}
