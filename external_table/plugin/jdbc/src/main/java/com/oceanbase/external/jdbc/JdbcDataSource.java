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

package com.oceanbase.external.jdbc;

import java.io.IOException;
import java.sql.*;
import java.util.*;
import java.util.function.Function;

import com.oceanbase.external.api.Constants;
import com.oceanbase.external.api.DataSource;
import com.oceanbase.external.api.SqlFilter;
import com.oceanbase.external.api.TableScanParameter;
import org.apache.arrow.adapter.jdbc.JdbcFieldInfo;
import org.apache.arrow.adapter.jdbc.JdbcToArrowConfig;
import org.apache.arrow.adapter.jdbc.JdbcToArrowConfigBuilder;
import org.apache.arrow.vector.ipc.ArrowReader;
import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.vector.types.pojo.ArrowType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static java.sql.ResultSet.CONCUR_READ_ONLY;
import static java.sql.ResultSet.TYPE_FORWARD_ONLY;

public class JdbcDataSource extends DataSource {
    private final static Logger logger = LoggerFactory.getLogger(JdbcDataSource.class);

    protected final JdbcConfig config;

    public JdbcDataSource(BufferAllocator allocator, Map<String, String> properties) {
        super(allocator, properties);
        this.config = JdbcConfig.of(properties.getOrDefault(Constants.PARAMETERS_KEY, ""));
    }

    @Override
    public String toDisplayString() {
        return config.toDisplayString();
    }

    @Override
    public List<String> pushdownFilters(List<SqlFilter> filters) {
        QueryBuilder queryBuilder = getQueryBuilder();
        List<String> acceptFilters = new ArrayList<>();
        for (SqlFilter sqlFilter : filters) {
            String str = queryBuilder.buildQueryFilter(sqlFilter);
            acceptFilters.add(str);
        }
        return acceptFilters;
    }

    @Override
    public ArrowReader createScanner(Map<String, Object> scanParameterMap) throws IOException {
        TableScanParameter scanParameter = TableScanParameter.of(scanParameterMap);
        QueryBuilder queryBuilder = getQueryBuilder();
        String querySql = queryBuilder.buildSelectQuery(scanParameter, config);
        logger.info("jdbc query sql is '{}'", querySql);

        Connection connection = null;
        Statement statement = null;

        /*
         we don't user ADBC here even though ADBC-JDBC supports JDBC very well and has a better design.
         Because ADBC-JDBC doesn't support type mapping.
         */
        Calendar utcCalendar = Calendar.getInstance(TimeZone.getTimeZone("UTC"));

        try {
            connection = getConnection();
            statement = connection.createStatement(TYPE_FORWARD_ONLY, CONCUR_READ_ONLY);
            statement.setFetchSize(Integer.MIN_VALUE);
            ResultSet resultSet = statement.executeQuery(querySql);

            final int batchSize = calcBatchSize(resultSet);
            logger.info("use batch size: {}", batchSize);
            JdbcToArrowConfig jdbcToArrowConfig = new JdbcToArrowConfigBuilder(allocator, utcCalendar)
                    .setReuseVectorSchemaRoot(true)
                    .setJdbcToArrowTypeConverter(getReadTypeMapping(utcCalendar))
                    .setJdbcConsumerGetter(JdbcTypeMapping.getJdbcConsumerFactory())
                    .setTargetBatchSize(batchSize)
                    .build();
            jdbcToArrowConfig.setMaxBufferSize(2L * 1024 * 1024 * 1024);

            return new JdbcScanner(connection, statement, resultSet, jdbcToArrowConfig);
        } catch (SQLException e) {
            throw new IOException(e);
        }
    }

    protected Connection getConnection() throws SQLException {
        return DriverManager.getConnection(config.jdbc_url, config.user, config.password);
    }

    protected QueryBuilder getQueryBuilder() {
        return new QueryBuilder();
    }

    protected Function<JdbcFieldInfo, ArrowType> getReadTypeMapping(Calendar calendar) {
        return JdbcTypeMapping.getDefaultTypeMapping(calendar);
    }

    protected int calcBatchSize(ResultSet resultSet) throws SQLException {
        final int defaultBatchSize = 256;
        int batchSize = defaultBatchSize;
        ResultSetMetaData metaData = resultSet.getMetaData();
        for (int i = 1; i <= metaData.getColumnCount(); i++) {
            int columnType = metaData.getColumnType(i);
            switch (columnType) {
                case Types.LONGNVARCHAR:
                case Types.LONGVARCHAR:
                case Types.LONGVARBINARY:
                case Types.BLOB:
                case Types.CLOB:
                case Types.NCLOB:
                    batchSize = 1;
                    break;
            }
            if (batchSize < defaultBatchSize) {
                break;
            }
        }
        return batchSize;
    }
}
