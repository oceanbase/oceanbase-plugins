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

package com.oceanbase.external.internal;

import com.oceanbase.external.api.PredicateSqlFilterExpr;
import com.oceanbase.external.api.SqlFilter;
import com.oceanbase.external.api.SqlFilterExpr;
import org.apache.arrow.c.ArrowArray;
import org.apache.arrow.c.ArrowArrayStream;
import org.apache.arrow.c.ArrowSchema;
import org.apache.arrow.c.Data;
import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.memory.RootAllocator;
import org.apache.arrow.vector.FieldVector;
import org.apache.arrow.vector.VectorSchemaRoot;
import org.apache.arrow.vector.ipc.ArrowReader;
import org.apache.arrow.vector.types.pojo.Field;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.slf4j.MDC;
import org.slf4j.event.Level;

import java.util.ArrayList;
import java.util.List;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.stream.Collectors;

import static org.slf4j.Logger.ROOT_LOGGER_NAME;

/**
 * This class is used by observer.
 */
@SuppressWarnings("unused")
public class JniUtils {
    private final static Logger logger = LoggerFactory.getLogger(JniUtils.class);

    private final static BufferAllocator allocator = new RootAllocator();

    private final static String oceanbaseLabelKey = "oceanbaseLabel";

    static BufferAllocator getAllocator(String name) {
        //return allocator.newChildAllocator(name, 0, Long.MAX_VALUE);
        return allocator;
    }

    public static String getExceptionMessage(Throwable throwable, boolean backtrace) {
        if (null == throwable) {
            logger.trace("throwable is null");
            return "";
        }

        try {
            StringWriter output = new StringWriter();
            output.write(String.format("%s: %s", throwable.getClass().getSimpleName(), throwable.getMessage()));
            Throwable[] suppressed = throwable.getSuppressed();
            if (suppressed != null) {
                for (Throwable suppressedEx : suppressed) {
                    output.write(String.format(", Suppressed Exception: %s: %s",
                            suppressedEx.getClass().getSimpleName(), suppressedEx.getMessage()));
                }
            }
            Throwable cause = throwable;
            while ((cause = cause.getCause()) != null) {
                output.write(String.format(", CAUSED BY: %s: %s",
                        cause.getClass().getSimpleName(), cause.getMessage()));

                suppressed = cause.getSuppressed();
                if (suppressed != null) {
                    for (Throwable suppressedEx : suppressed) {
                        output.write(String.format(", Suppressed Exception: %s: %s",
                                suppressedEx.getClass().getSimpleName(), suppressedEx.getMessage()));
                    }
                }
            }

            if (backtrace) {
                throwable.printStackTrace(new PrintWriter(output));

                suppressed = throwable.getSuppressed();
                if (suppressed != null) {
                    for (Throwable suppressedEx : suppressed) {
                        suppressedEx.printStackTrace(new PrintWriter(output));
                    }
                }
            }
            String result = output.toString();
            logger.debug("print exception message in JniUtils\n{}", result);
            return result;
        } catch (Exception e) {
            return "(got exception when printing exception)";
        }
    }

    public static List<SqlFilter> parseSqlFilterFromArrow(VectorSchemaRoot vectorSchemaRoot) {
        try {
            if (vectorSchemaRoot == null) {
                throw new IllegalArgumentException("vectorSchemaRoot is null");
            }

            if (vectorSchemaRoot.getRowCount() != 1) {
                throw new IllegalArgumentException("row count is not 1: " + vectorSchemaRoot.getRowCount());
            }

            logger.info("parse sql filter from arrow, vector schema root: schema={}, vector={}",
                vectorSchemaRoot.getSchema(), vectorSchemaRoot.contentToTSVString());

            ArrayList<SqlFilter> sqlFilters = new ArrayList<>();
            ArrayList<SqlFilterExpr> exprList = new ArrayList<>();
            List<Field> fields = vectorSchemaRoot.getSchema().getFields();
            for (int i = 0; i < fields.size(); i++) {
                FieldVector fieldVector = vectorSchemaRoot.getVector(i);
                SqlFilterExpr.Type exprType = SqlFilterUtils.getExprType(fieldVector.getField().getName());
                if (exprType == SqlFilterExpr.Type.FILTER) {
                    // this is a field to indicate one filter parsed done
                    if (exprList.isEmpty()) {
                        throw new IllegalArgumentException("got empty expr list: " + vectorSchemaRoot.toString());
                    }

                    SqlFilterExpr lastExpr = exprList.get(exprList.size() - 1);
                    if (!(lastExpr instanceof PredicateSqlFilterExpr)) {
                        throw new IllegalArgumentException("last expr should be compound expr but got " + lastExpr.getClass());
                    }

                    sqlFilters.add(new SqlFilter((PredicateSqlFilterExpr) lastExpr));
                    exprList.clear();
                } else {
                    SqlFilterExpr filterExpr = SqlFilterUtils.fromArrow(fieldVector, exprList);
                    exprList.add(filterExpr);
                }
            }

            if (!exprList.isEmpty()) {
                throw new IllegalArgumentException("all expressions have been parsed but the expr list used for parse is not empty: " + exprList.size());
            }

            return sqlFilters;
        } finally {
            if (vectorSchemaRoot != null) {
                vectorSchemaRoot.close();
            }
        }
    }

    public static List<Object> parseQuestionMarkValues(VectorSchemaRoot vectorSchemaRoot) {
        try {
            if (vectorSchemaRoot == null) {
                throw new IllegalArgumentException("vectorSchemaRoot is null");
            }

            if (vectorSchemaRoot.getRowCount() != 1) {
                throw new IllegalArgumentException("row count is not 1: " + vectorSchemaRoot.getRowCount());
            }

            logger.trace("parse question mark values from arrow, vector schema root: schema={}, vector={}",
                vectorSchemaRoot.getSchema(), vectorSchemaRoot.contentToTSVString());

            return vectorSchemaRoot.getFieldVectors().stream()
                .map(fieldVector -> fieldVector.getObject(0))
                .collect(Collectors.toList());
        } finally {
            if (vectorSchemaRoot != null) {
                vectorSchemaRoot.close();
            }
        }
    }

    public static void exportArrowStream(ArrowReader arrowReader, long address) {
        try (ArrowArrayStream arrowStream = ArrowArrayStream.wrap(address)) {
            Data.exportArrayStream(allocator, arrowReader, arrowStream);
        }
    }

    public static VectorSchemaRoot importRecordBatch(long arrayAddress, long schemaAddress) {
        try (ArrowArray array = ArrowArray.wrap(arrayAddress);
             ArrowSchema schema = ArrowSchema.wrap(schemaAddress)) {
            return Data.importVectorSchemaRoot(allocator, array, schema, null);
        }
    }

    public static void setLogLevel(String levelName) {
        Level level = null;
        for (Level levelValue : Level.values()) {
            if (0 == levelValue.toString().compareToIgnoreCase(levelName)) {
                level = levelValue;
            }
        }

        if (level == null) {
            logger.warn("can't set log level. No such level: {}", levelName);
            return;
        }

        Logger rootLogger = LoggerFactory.getLogger(ROOT_LOGGER_NAME);
        if (rootLogger instanceof ch.qos.logback.classic.Logger) {
            logger.info("set java log level to: {}", level);
            ch.qos.logback.classic.Logger logbackRootLogger = (ch.qos.logback.classic.Logger) rootLogger;
            logbackRootLogger.setLevel(ch.qos.logback.classic.Level.convertAnSLF4JLevel(level));
        } else {
            logger.warn("can't set log level. root log is not logback: {}", rootLogger.getClass().getName());
        }
    }

    public static void setLogLabel(String label) {
        MDC.put(oceanbaseLabelKey, label);
    }

    public static void clearLogLabel() {
        MDC.remove(oceanbaseLabelKey);
    }
}
