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

import com.oceanbase.external.api.*;
import org.apache.arrow.vector.BigIntVector;
import org.apache.arrow.vector.FieldVector;
import org.apache.arrow.vector.VarCharVector;
import org.apache.arrow.vector.complex.ListVector;
import org.apache.arrow.vector.complex.StructVector;
import org.apache.arrow.vector.types.Types;
import org.apache.arrow.vector.types.pojo.ArrowType;
import org.apache.arrow.vector.types.pojo.FieldType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Some helper function to translate filters from Arrow to Java objects
 */
class SqlFilterUtils {
    private final static Logger logger = LoggerFactory.getLogger(SqlFilterUtils.class);

    /// The constants below defined in observer, so we can't change them.
    private static final String CMP_PREFIX = "cmp.";
    private static final String CONCAT_PREFIX = "concat.";
    private static final Map<String, SqlFilterExpr.Type> SQL_FILTER_TYPE_NAME_MAP = new HashMap<String, SqlFilterExpr.Type>(){{
        put("invalid", SqlFilterExpr.Type.INVALID);
        put("column_ref", SqlFilterExpr.Type.COLUMN_REF);
        put("const_value", SqlFilterExpr.Type.CONST_VALUE);
        put("question_mark", SqlFilterExpr.Type.QUESTION_MARK);
        put("null", SqlFilterExpr.Type.NULL);
        put("filter", SqlFilterExpr.Type.FILTER);
        put(CMP_PREFIX + "eq", SqlFilterExpr.Type.CMP_EQUAL);
        put(CMP_PREFIX + "le", SqlFilterExpr.Type.CMP_LESS_EQUAL);
        put(CMP_PREFIX + "lt", SqlFilterExpr.Type.CMP_LESS);
        put(CMP_PREFIX + "ge", SqlFilterExpr.Type.CMP_GREATER_EQUAL);
        put(CMP_PREFIX + "gt", SqlFilterExpr.Type.CMP_GREATER);
        put(CMP_PREFIX + "ne", SqlFilterExpr.Type.CMP_NOT_EQUAL);
        put(CMP_PREFIX + "is", SqlFilterExpr.Type.CMP_IS);
        put(CMP_PREFIX + "is_not", SqlFilterExpr.Type.CMP_IS_NOT);
        put(CMP_PREFIX + "between", SqlFilterExpr.Type.CMP_BETWEEN);
        put(CMP_PREFIX + "not_between", SqlFilterExpr.Type.CMP_NOT_BETWEEN);
        put(CMP_PREFIX + "like", SqlFilterExpr.Type.CMP_LIKE);
        put(CMP_PREFIX + "not_like", SqlFilterExpr.Type.CMP_NOT_LIKE);
        put(CMP_PREFIX + "in", SqlFilterExpr.Type.CMP_IN);
        put(CMP_PREFIX + "not_in", SqlFilterExpr.Type.CMP_NOT_IN);
        put(CONCAT_PREFIX + "and", SqlFilterExpr.Type.CMP_AND);
        put(CONCAT_PREFIX + "or", SqlFilterExpr.Type.CMP_OR);
        put(CONCAT_PREFIX + "not", SqlFilterExpr.Type.CMP_NOT);
    }};

    static SqlFilterExpr.Type getExprType(String name) {
        return SQL_FILTER_TYPE_NAME_MAP.getOrDefault(name, SqlFilterExpr.Type.INVALID);
    }
    static SqlFilterExpr fromArrow(FieldVector fieldVector, List<SqlFilterExpr> exprs) {
        if (fieldVector.getValueCount() != 1) {
            throw new IllegalArgumentException("invalid value count, expect 1 but got: " +
                    fieldVector.getValueCount());
        }

        String fieldName = fieldVector.getField().getName();
        SqlFilterExpr.Type exprType = SQL_FILTER_TYPE_NAME_MAP.get(fieldName);
        if (null == exprType) {
            throw new IllegalArgumentException("invalid expr type: " + fieldName);
        }

        switch (exprType) {
            case COLUMN_REF:
                return columnRefExprFromArrow(fieldVector);
            case CONST_VALUE:
                return constValueExprFromArrow(fieldVector);
            case QUESTION_MARK:
                return questionMarkSqlFilterExpr(fieldVector);
            case NULL:
                return new ConstValueSqlFilterExpr(null);
            default:
                return predicateExprFromArrow(exprType, fieldVector, exprs);
        }
    }

    private static ColumnRefSqlFilterExpr columnRefExprFromArrow(FieldVector fieldVector) {
        if (fieldVector.getField().getType().getTypeID() != ArrowType.ArrowTypeID.Utf8) {
            throw new IllegalArgumentException("invalid value type, should be varchar but got: "
                    + fieldVector.getField().getType().getTypeID().toString() + ", field vector is " + fieldVector);
        }

        String columnName = ((VarCharVector)fieldVector).getObject(0).toString();
        return new ColumnRefSqlFilterExpr(columnName);
    }

    private static ConstValueSqlFilterExpr constValueExprFromArrow(FieldVector fieldVector) {
        Object value = fieldVector.getObject(0);
        return new ConstValueSqlFilterExpr(value);
    }

    /**
     * build question mark filter expression
     * A question mark expression contains 2 fields:
     * - value: The value of the expression. We should pay attention to the DataType but not the real value.
     * - placeholder: The index of the value in the `ParamStore`.
     */
    private static QuestionMarkSqlFilterExpr questionMarkSqlFilterExpr(FieldVector fieldVector) {
        if (fieldVector.getMinorType() != Types.MinorType.STRUCT) {
            throw new IllegalArgumentException("invalid value type, expect STRUCT but got: " +
                    fieldVector.getMinorType() + ". Schema=" + fieldVector.getField());
        }

        StructVector structVector = (StructVector) fieldVector;
        FieldVector valueVector = structVector.getChild("value");
        FieldVector placeholderVector = structVector.getChild("placeholder");
        if (null == valueVector || null == placeholderVector) {
            throw new IllegalArgumentException("no value or placeholder child in the question mark expression. value=" +
                    valueVector + ", placeholder=" + placeholderVector + ". struct vector=" + fieldVector.getField());
        }

        if (valueVector.getValueCount() != 1 || placeholderVector.getValueCount() != 1) {
            throw new IllegalArgumentException("invalid value count. value's count=" + valueVector.getValueCount() +
                    ", placeholder's count=" + placeholderVector.getValueCount() +
                    ". struct vector=" + fieldVector.getField() + " with value=" + fieldVector);
        }

        if (placeholderVector.getMinorType() != Types.MinorType.BIGINT) {
            throw new IllegalArgumentException("invalid placeholder type, expect BIGINT but got: " +
                    placeholderVector.getMinorType() + ". struct vector=" + fieldVector.getField());
        }

        FieldType valueType = valueVector.getField().getFieldType();
        long placeholder = ((BigIntVector)placeholderVector).get(0);
        return new QuestionMarkSqlFilterExpr(valueType, placeholder);
    }

    private static PredicateSqlFilterExpr predicateExprFromArrow(SqlFilterExpr.Type exprType, FieldVector fieldVector, List<SqlFilterExpr> exprs) {
        if (fieldVector.getField().getType().getTypeID() != ArrowType.ArrowTypeID.List) {
            logger.warn("invalid value type, expect LIST but got: {}, {}",
                    fieldVector.getField().getType().getTypeID(), fieldVector);
            return null;
        }

        ListVector listVector = (ListVector) fieldVector;
        if (listVector.getDataVector().getField().getType().getTypeID() != ArrowType.ArrowTypeID.Int) {
            throw new IllegalArgumentException("invalid data type in data vector, expect INT but got: " +
                    listVector.getDataVector().getField().getType().getTypeID() + ", list vector is " + listVector);
        }

        ArrayList<Integer> exprIndexes = (ArrayList<Integer>) fieldVector.getObject(0);

        PredicateSqlFilterExpr.Type type = PredicateSqlFilterExpr.Type.INVALID;
        ArrayList<SqlFilterExpr> children = new ArrayList<>();

        int expectedChildrenSize = 0;
        switch (exprType) {
            case CMP_EQUAL: {
                expectedChildrenSize = 2;
                type = PredicateSqlFilterExpr.Type.EQUAL;
            } break;
            case CMP_LESS_EQUAL: {
                expectedChildrenSize = 2;
                type = PredicateSqlFilterExpr.Type.LESS_EQUAL;
            } break;
            case CMP_LESS: {
                expectedChildrenSize = 2;
                type = PredicateSqlFilterExpr.Type.LESS_THAN;
            } break;
            case CMP_GREATER_EQUAL: {
                expectedChildrenSize = 2;
                type = PredicateSqlFilterExpr.Type.GREATER_EQUAL;
            } break;
            case CMP_GREATER: {
                expectedChildrenSize = 2;
                type = PredicateSqlFilterExpr.Type.GREATER_THAN;
            } break;
            case CMP_NOT_EQUAL: {
                expectedChildrenSize = 2;
                type = PredicateSqlFilterExpr.Type.NOT_EQUAL;
            } break;
            case CMP_IS: {
                expectedChildrenSize = 2;
                type = PredicateSqlFilterExpr.Type.IS;
            } break;
            case CMP_IS_NOT: {
                expectedChildrenSize = 2;
                type = PredicateSqlFilterExpr.Type.IS_NOT;
            } break;
            case CMP_BETWEEN: {
                expectedChildrenSize = 3;
                type = PredicateSqlFilterExpr.Type.BETWEEN;
            } break;
            case CMP_NOT_BETWEEN: {
                expectedChildrenSize = 3;
                type = PredicateSqlFilterExpr.Type.NOT_BETWEEN;
            } break;
            case CMP_LIKE: {
                expectedChildrenSize = 2;
                type = PredicateSqlFilterExpr.Type.LIKE;
            } break;
            case CMP_NOT_LIKE: {
                expectedChildrenSize = 2;
                type = PredicateSqlFilterExpr.Type.NOT_LIKE;
            } break;
            case CMP_IN: {
                expectedChildrenSize = -1;
                type = PredicateSqlFilterExpr.Type.IN;
            } break;
            case CMP_NOT_IN: {
                expectedChildrenSize = -1;
                type = PredicateSqlFilterExpr.Type.NOT_IN;
            } break;
            case CMP_AND: {
                expectedChildrenSize = -1;
                type = PredicateSqlFilterExpr.Type.AND;
            } break;
            case CMP_OR: {
                expectedChildrenSize = -1; // may have many children
                type = PredicateSqlFilterExpr.Type.OR;
            } break;
            case CMP_NOT: {
                expectedChildrenSize = 1;
                type = PredicateSqlFilterExpr.Type.NOT;
            } break;
            default: {
                logger.info("not supported expr: {}, parsed expr type: {}", fieldVector.getField().getName(), exprType);
                return null;
            }
        }

        if (expectedChildrenSize > 0 && exprIndexes.size() < expectedChildrenSize) {
            throw new IllegalArgumentException("invalid children size. expected " + expectedChildrenSize
                    + " but got " + exprIndexes.size());
        }

        for (int childIndex : exprIndexes) {
            if (childIndex < 0 || childIndex >= exprs.size()) {
                throw new ArrayIndexOutOfBoundsException("invalid expr index in building comparison expr. fieldVector:" +
                        fieldVector + ", child index: " + childIndex + ", exprs size: " + exprs.size());
            }

            children.add(exprs.get(childIndex));
        }
        return new PredicateSqlFilterExpr(type, children);
    }
}
