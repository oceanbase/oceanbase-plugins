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

package com.oceanbase.external.mysql;

import com.mysql.cj.conf.PropertyKey;
import com.oceanbase.external.jdbc.JdbcDataSource;
import com.oceanbase.external.jdbc.JdbcTypeMapping;
import com.oceanbase.external.jdbc.QueryBuilder;
import org.apache.arrow.adapter.jdbc.JdbcFieldInfo;
import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.vector.ipc.ArrowReader;
import org.apache.arrow.vector.types.Types;
import org.apache.arrow.vector.types.pojo.ArrowType;

import java.io.IOException;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Calendar;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;
import java.util.function.Function;

import static java.sql.Types.*;
import static java.sql.Types.TIMESTAMP;

@SuppressWarnings("unused")
public class MysqlJdbcDataSource extends JdbcDataSource {
    private final static String DRIVER_CLASS_NAME = "com.mysql.cj.jdbc.Driver";

    public MysqlJdbcDataSource(BufferAllocator allocator, String parameters) {
        super(allocator, parameters);
    }

    @Override
    public ArrowReader createScanner(Map<String, Object> scanParameterMap) throws IOException {
        // mysql jdbc driver connection cleanup is conflict with something others
        System.setProperty("com.mysql.cj.disableAbandonedConnectionCleanup", "false");
        try {
            Class.forName(DRIVER_CLASS_NAME);
        } catch (ClassNotFoundException e) {
            throw new IOException(e);
        }
        return super.createScanner(scanParameterMap);
    }

    @Override
    protected QueryBuilder getQueryBuilder() {
        return new QueryBuilder("`");
    }

    @Override
    protected Connection getConnection() throws SQLException {
        Map<String, String> defaultProperties = new HashMap<String, String>() {{
            put(PropertyKey.connectionTimeZone.getKeyName(), "UTC");
            put(PropertyKey.forceConnectionTimeZoneToSession.getKeyName(), "true");
            put(PropertyKey.zeroDateTimeBehavior.getKeyName(), "round");
            put(PropertyKey.tinyInt1isBit.getKeyName(), "false");
            put(PropertyKey.characterEncoding.getKeyName(), "UTF-8");
        }};
        Properties connProperties = new Properties();
        if (config.user != null) {
            connProperties.put(PropertyKey.USER.getKeyName(), config.user);
        }
        if (config.password != null) {
            connProperties.put(PropertyKey.PASSWORD.getKeyName(), config.password);
        }
        defaultProperties.forEach((key, value) -> {
            if (!config.jdbc_url.contains(key)) {
                connProperties.put(key, value);
            }
        });
        return DriverManager.getConnection(config.jdbc_url, connProperties);
    }

    @Override
    protected Function<JdbcFieldInfo, ArrowType> getReadTypeMapping(Calendar calendar) {
        return (jdbcFieldInfo -> {
            switch (jdbcFieldInfo.getJdbcType()) {
                // the range of date in MySQL exceed JDBC, so we use String to handle these types
                case DATE:
                case TIME:
                case TIMESTAMP:
                    return Types.MinorType.VARCHAR.getType();

                default: return JdbcTypeMapping.getDefaultTypeMapping(calendar).apply(jdbcFieldInfo);
            }
        });
    }
}
