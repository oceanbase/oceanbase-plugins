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

import org.apache.arrow.adapter.jdbc.JdbcFieldInfo;
import org.apache.arrow.adapter.jdbc.JdbcToArrowConfig;
import org.apache.arrow.adapter.jdbc.JdbcToArrowUtils;
import org.apache.arrow.adapter.jdbc.consumer.VarCharConsumer;
import org.apache.arrow.vector.VarBinaryVector;
import org.apache.arrow.vector.types.Types;
import org.apache.arrow.vector.types.pojo.ArrowType;

import java.util.Calendar;
import java.util.function.Function;

import static java.sql.Types.*;

public class JdbcTypeMapping {

    public static Function<JdbcFieldInfo, ArrowType> getDefaultTypeMapping(Calendar calendar) {
        return (jdbcFieldInfo -> {
            switch (jdbcFieldInfo.getJdbcType()) {
                // use int64 can enhance the performance in OceanBase
                // because all integer are stored as an 8-bytes memory.
                case BOOLEAN:
                case BIT:
                case TINYINT:
                case SMALLINT:
                case INTEGER:
                    return Types.MinorType.BIGINT.getType();

                case BIGINT:
                    // Arrow JDBC cannot handle unsigned integer so we use a larger
                    // type to hold the value.
                    return new ArrowType.Decimal(jdbcFieldInfo.getPrecision(), jdbcFieldInfo.getScale(), 128);

                case DATE:
                case TIME:
                case TIMESTAMP:
                    return Types.MinorType.VARCHAR.getType();

                case LONGNVARCHAR:
                case LONGVARCHAR:
                case CLOB:
                case NCLOB:
                    return Types.MinorType.VARCHAR.getType(); // arrow doesn't have LargeVarcharConsumer

                case LONGVARBINARY:
                case BLOB:
                    return Types.MinorType.VARBINARY.getType(); // arrow doesn't have LargeBinaryConsumer

                default: return JdbcToArrowUtils.getArrowTypeFromJdbcType(jdbcFieldInfo, calendar);
            }
        });
    }

    public static JdbcToArrowConfig.JdbcConsumerFactory getJdbcConsumerFactory() {
        return (arrowType, columnIndex, nullable, fieldVector, jdbcToArrowConfig) -> {
            switch (arrowType.getTypeID()) {
                case Binary:
                case LargeBinary:
                    return new BinaryConsumer((VarBinaryVector) fieldVector, columnIndex);
                default:
                    return JdbcToArrowUtils.getConsumer(arrowType, columnIndex, nullable, fieldVector, jdbcToArrowConfig);
            }
        };
    }
}
