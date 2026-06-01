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

package com.oceanbase.external.odps;

import com.oceanbase.external.jdbc.JdbcDataSource;
import com.oceanbase.external.jdbc.JdbcTypeMapping;
import com.oceanbase.external.jdbc.QueryBuilder;
import org.apache.arrow.adapter.jdbc.JdbcFieldInfo;
import org.apache.arrow.adapter.jdbc.JdbcToArrowConfig;
import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.vector.DecimalVector;
import org.apache.arrow.vector.ValueVector;
import org.apache.arrow.vector.ipc.ArrowReader;
import org.apache.arrow.vector.types.Types;
import org.apache.arrow.vector.types.pojo.ArrowType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.Calendar;
import java.util.Map;
import java.util.Properties;
import java.util.function.Function;

import static java.sql.Types.BIGINT;
import static java.sql.Types.DATE;
import static java.sql.Types.DECIMAL;
import static java.sql.Types.NUMERIC;
import static java.sql.Types.TIMESTAMP;

@SuppressWarnings("unused")
public class OdpsJdbcDataSource extends JdbcDataSource {
    private static final Logger logger = LoggerFactory.getLogger(OdpsJdbcDataSource.class);
    private static final String DRIVER_CLASS_NAME = "com.aliyun.odps.jdbc.OdpsDriver";

    // ODPS JDBC driver-specific type codes for complex types
    private static final int ODPS_TYPE_ARRAY  = 2003; // java.sql.Types.ARRAY
    private static final int ODPS_TYPE_MAP    = 2000; // java.sql.Types.JAVA_OBJECT
    private static final int ODPS_TYPE_STRUCT = 2002; // java.sql.Types.STRUCT

    public OdpsJdbcDataSource(BufferAllocator allocator, Map<String, String> properties) {
        super(allocator, properties);
    }

    @Override
    public ArrowReader createScanner(Map<String, Object> scanParameterMap) throws IOException {
        try {
            Class.forName(DRIVER_CLASS_NAME);
        } catch (ClassNotFoundException e) {
            throw new IOException("ODPS JDBC driver not found on classpath: " + DRIVER_CLASS_NAME, e);
        }
        return super.createScanner(scanParameterMap);
    }

    @Override
    protected Connection getConnection() throws SQLException {
        Properties props = new Properties();
        // JdbcConfig.user / JdbcConfig.password map to ODPS accessId / accessKey
        if (config.user != null) {
            props.setProperty("accessId", config.user);
        }
        if (config.password != null) {
            props.setProperty("accessKey", config.password);
        }
        props.setProperty("charset", "UTF-8");
        props.setProperty("interactiveMode", "false");
        logger.info("Connecting to ODPS");
        return DriverManager.getConnection(config.jdbc_url, props);
    }

    @Override
    protected QueryBuilder getQueryBuilder() {
        // ODPS uses backtick to quote identifiers (same as MySQL)
        return new QueryBuilder("`");
    }

    @Override
    public void setOptimalFetchSize(Statement statement, Connection connection) throws SQLException {
        // ODPS JDBC supports batch fetching; 1000 rows per fetch is a reasonable default.
        // Do NOT use Integer.MIN_VALUE (MySQL streaming mode) — it is not supported by ODPS.
        statement.setFetchSize(1000);
        logger.info("Set ODPS fetch size: 1000");
    }

    @Override
    protected Function<JdbcFieldInfo, ArrowType> getReadTypeMapping(Calendar calendar) {
        return jdbcFieldInfo -> {
            switch (jdbcFieldInfo.getJdbcType()) {
                // ODPS BIGINT: the base class maps BIGINT to Decimal(precision,scale,128),
                // but ODPS may return precision=0, making it an invalid Decimal type.
                case BIGINT:
                    return Types.MinorType.BIGINT.getType();

                // ODPS DATE / DATETIME: expose as VARCHAR to avoid timezone/range issues.
                case DATE:
                case TIMESTAMP:
                    return Types.MinorType.VARCHAR.getType();

                // ODPS DECIMAL / NUMERIC: keep as Decimal(p,s,128) so C++ can receive the
                // correct type. Reading is handled by OdpsDecimalConsumer via getString().
                // If ODPS reports precision=0 (unspecified DECIMAL), fall back to (38,18).
                case DECIMAL:
                case NUMERIC: {
                    int precision = jdbcFieldInfo.getPrecision();
                    int scale     = jdbcFieldInfo.getScale();
                    if (precision <= 0) precision = 38;
                    if (scale < 0)      scale = 18;
                    return new ArrowType.Decimal(precision, scale, 128);
                }

                // Complex types serialised as JSON strings by ODPS JDBC.
                case ODPS_TYPE_ARRAY:
                case ODPS_TYPE_MAP:
                case ODPS_TYPE_STRUCT:
                    return Types.MinorType.VARCHAR.getType();

                default:
                    return JdbcTypeMapping.getDefaultTypeMapping(calendar).apply(jdbcFieldInfo);
            }
        };
    }

    @Override
    protected JdbcToArrowConfig.JdbcConsumerFactory getJdbcConsumerFactory() {
        return (arrowType, columnIndex, nullable, fieldVector, config) -> {
            // Use a custom consumer for DECIMAL that reads via getString() instead of
            // getBigDecimal(). ODPS JDBC's getBigDecimal() may return BigDecimal values
            // whose scale doesn't match the declared column scale, causing Arrow's default
            // DecimalConsumer to throw ArithmeticException with UNNECESSARY rounding mode.
            if (arrowType.getTypeID() == ArrowType.ArrowTypeID.Decimal
                    && fieldVector instanceof DecimalVector) {
                return new OdpsDecimalConsumer((DecimalVector) fieldVector, columnIndex);
            }
            return JdbcTypeMapping.getJdbcConsumerFactory().apply(arrowType, columnIndex, nullable, fieldVector, config);
        };
    }

    /** Reads DECIMAL values via getString() to avoid scale-mismatch in getBigDecimal(). */
    private static class OdpsDecimalConsumer implements org.apache.arrow.adapter.jdbc.consumer.JdbcConsumer {
        private DecimalVector vector;
        private final int columnIndex;
        private int currentIndex = 0;

        OdpsDecimalConsumer(DecimalVector vector, int columnIndex) {
            this.vector = vector;
            this.columnIndex = columnIndex;
        }

        @Override
        public void consume(ResultSet rs) throws SQLException {
            String str = rs.getString(columnIndex);
            if (str != null && !rs.wasNull()) {
                BigDecimal bd = new BigDecimal(str);
                int targetScale = vector.getScale();
                if (bd.scale() > targetScale) {
                    bd = bd.stripTrailingZeros();
                }
                if (bd.scale() > targetScale) {
                    throw new SQLException("Scale mismatch between BigDecimal and DecimalVector");
                }
                bd = bd.setScale(targetScale, RoundingMode.UNNECESSARY);
                vector.setSafe(currentIndex, bd);
            } else {
                vector.setNull(currentIndex);
            }
            currentIndex++;
        }

        @Override
        public void close() {}

        @Override
        public void resetValueVector(ValueVector newVector) {
            this.vector = (DecimalVector) newVector;
            this.currentIndex = 0;
        }
    }
}
