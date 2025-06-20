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

import com.oceanbase.external.api.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.text.MessageFormat;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

public class QueryBuilder {
    private final static Logger logger = LoggerFactory.getLogger(QueryBuilder.class);
    private final String identifierQuote;

    public QueryBuilder() {
        this("");
    }
    public QueryBuilder(String identifierQuote) {
        this.identifierQuote = identifierQuote;
    }

    /**
     * Build the JDBC query SQL string.
     * @param tableScanParameter contains the information about the query.
     * @return The Query SQL String
     */
    public String buildSelectQuery(TableScanParameter tableScanParameter, JdbcConfig jdbcConfig) {
        StringBuilder sb = new StringBuilder("SELECT ");
        if (tableScanParameter.getColumns().isEmpty()) {
            sb.append(1);
        } else {
            sb.append(tableScanParameter.getColumns().stream()
                    .map(columnName -> quoteString(columnName, identifierQuote))
                    .collect(Collectors.joining(",")));
        }
        sb.append(" FROM ").append(quoteString(jdbcConfig.table, identifierQuote));

        /// The filters were created by {@link JdbcDataSource.pushdownFilters}
        List<String> filters = tableScanParameter.getSqlFilters();
        if (!filters.isEmpty()) {
            logger.debug("filters is : {}", filters);
            // NOTE we must convert the elements to string as the MessageFormat would like to format
            // the elements friendly to human. For example, the number 1000000 would be formatted to
            // 1,000,000 which is not a valid sql.
            sb.append(" WHERE ");
            String filterSql;
            if (!tableScanParameter.getQuestionMarkValues().isEmpty()) {
                Object[] questionMarkStringValues = tableScanParameter.getQuestionMarkValues().stream()
                    .map(QueryBuilder::toSqlString)
                    .toArray();

                // `'` is a special character in MessageFormat.
                // we should replace `'` to `''`.
                filterSql = filters.stream()
                    .map(filter -> MessageFormat.format(
                        filter.replace("'", "''"), questionMarkStringValues))
                    .collect(Collectors.joining(" AND "));
            } else {
                filterSql = String.join(" AND ", filters);
            }
            sb.append(filterSql);
        }
        return sb.toString();
    }

    /**
     * Convert filter expressions into SQL query elements in the `WHERE` conditions
     * @param sqlFilter The filter expression
     * @return The SQL query element. For example, a=1, (b>100) or (c in ('x', 'y')).
     */
    public String buildQueryFilter(SqlFilter sqlFilter) {
        PredicateSqlFilterExpr sqlFilterExpr = sqlFilter.getSqlFilterExpr();
        return predicateExprToQueryString(sqlFilterExpr);
    }

    private String sqlFilterExprToQueryString(SqlFilterExpr sqlFilterExpr) {
        if (sqlFilterExpr instanceof ColumnRefSqlFilterExpr) {
            String columnName = ((ColumnRefSqlFilterExpr) sqlFilterExpr).getColumnName();
            return quoteString(columnName, this.identifierQuote);
        } else if (sqlFilterExpr instanceof ConstValueSqlFilterExpr) {
            Object object = ((ConstValueSqlFilterExpr) sqlFilterExpr).getValue();
            logger.debug("const value expr: value: {}, string value: {}", object, toSqlString(object));
            return toSqlString(object);

        } else if (sqlFilterExpr instanceof QuestionMarkSqlFilterExpr) {
            QuestionMarkSqlFilterExpr questionMarkSqlFilterExpr = (QuestionMarkSqlFilterExpr) sqlFilterExpr;
            return "{" + questionMarkSqlFilterExpr.getPlaceholderIndex() + "}";
        } else if (sqlFilterExpr instanceof PredicateSqlFilterExpr) {
            return predicateExprToQueryString((PredicateSqlFilterExpr) sqlFilterExpr);
        } else {
            throw new IllegalArgumentException("invalid expr type: " + sqlFilterExpr.getClass());
        }
    }

    private String predicateExprToQueryString(PredicateSqlFilterExpr sqlFilterExpr) {
        List<SqlFilterExpr> children = sqlFilterExpr.getChildren();
        String queryFormat = "";
        int expectedChildrenSize = 0;
        switch (sqlFilterExpr.getType()) {
            case EQUAL: {
                queryFormat = "{0}={1}";
                expectedChildrenSize = 2;
            } break;
            case NOT_EQUAL: {
                queryFormat = "{0}<>{1}";
                expectedChildrenSize = 2;
            } break;
            case LESS_EQUAL: {
                queryFormat = "{0}<={1}";
                expectedChildrenSize = 2;
            } break;
            case LESS_THAN: {
                queryFormat = "{0}<{1}";
                expectedChildrenSize = 2;
            } break;
            case GREATER_EQUAL: {
                queryFormat = "{0}>={1}";
                expectedChildrenSize = 2;
            } break;
            case GREATER_THAN: {
                queryFormat = "{0}>{1}";
                expectedChildrenSize = 2;
            } break;
            case IS: {
                queryFormat = "{0} IS {1}";
                expectedChildrenSize = 2;
            } break;
            case IS_NOT: {
                queryFormat = "{0} IS NOT {1}";
                expectedChildrenSize = 2;
            } break;
            case BETWEEN: {
                queryFormat = "{0} BETWEEN {1} AND {2}";
                expectedChildrenSize = 3;
            } break;
            case NOT_BETWEEN: {
                queryFormat = "NOT {0} BETWEEN {1} AND {2}";
                expectedChildrenSize = 3;
            } break;
            case LIKE: {
                queryFormat = "{0} LIKE {1}";
                expectedChildrenSize = 2;
            } break;
            case NOT_LIKE: {
                queryFormat = "{0} NOT LIKE {1}";
                expectedChildrenSize = 2;
            } break;
            case IN:
            case NOT_IN: {
                if (children.size() < 2) {
                    throw new IllegalArgumentException("at least 2 children for [NOT] IN filter, but got " + children.size());
                }
                String inParams = children.stream()
                        .skip(1) // the first element should be column
                        .map(this::sqlFilterExprToQueryString)
                        .collect(Collectors.joining(", "));
                String sqlKeyWord = sqlFilterExpr.getType() == PredicateSqlFilterExpr.Type.IN ?
                        " IN " : " NOT IN ";
                return sqlFilterExprToQueryString(children.get(0)) + sqlKeyWord + '(' + inParams + ')';
            }
            case AND: {
                return children.stream()
                        .map(filterExpr -> "(" + sqlFilterExprToQueryString(filterExpr) + ")")
                        .collect(Collectors.joining(" AND "));
            }
            case OR: {
                return children.stream()
                        .map(filterExpr -> "(" + sqlFilterExprToQueryString(filterExpr) + ")")
                        .collect(Collectors.joining(" OR "));
            }
            case NOT: {
                return " NOT (" + sqlFilterExprToQueryString(children.get(0)) + ")";
            }
            default: {
                queryFormat = "";
                expectedChildrenSize = -1;
            } break;
        }

        if (expectedChildrenSize > 0 && children.size() < expectedChildrenSize) {
            // some expressions have more than children than expected, such as LIKE
            throw new IllegalArgumentException("invalid children number. expect " + expectedChildrenSize
                    + " but got: " + children.size());
        }

        if (!queryFormat.isEmpty()) {
            Object[] childrenSqlStrings = children.stream()
                    .map(this::sqlFilterExprToQueryString)
                    .toArray();
            for (Object object : childrenSqlStrings) {
                logger.info("query to string value: {}", object);
            }
            logger.info("query to string. format={}", queryFormat);
            try {
                return MessageFormat.format(queryFormat, childrenSqlStrings);
            } catch (Exception e) {
                throw new RuntimeException("catch exception while formatting query filter. " +
                        "query format is '" + queryFormat + "', arguments are " + Arrays.toString(childrenSqlStrings) +
                        ", exceptions " + e.getMessage() + ", " + Arrays.toString(e.getStackTrace()), e);
            }
        }
        return "";
    }

    /**
     * Convert value into string which used in SQL where condition.
     * For example, WHERE A='abc' when the object is a string 'abc'
     */
    private static String toSqlString(Object object) {
        if (object == null) {
            return "null";
        }
        if (object instanceof String || object instanceof org.apache.arrow.vector.util.Text) {
            String val = Objects.toString(object);
            // replace `'` with `''` in SQL
            return "'" + val.replace("'", "''") +  "'";
        }
        // TODO Some types should be converted to string and quoted, such as date, datetime
        return Objects.toString(object);
    }

    private static String quoteString(Object str, String quote) {
        return quote + str + quote;
    }
}
