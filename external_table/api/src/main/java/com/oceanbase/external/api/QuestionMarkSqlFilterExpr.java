package com.oceanbase.external.api;

import org.apache.arrow.vector.types.pojo.FieldType;

/**
 * A question mark expression is a dynamic expression which can be evaluated in running time.
 * We can get the value when creating `Scanner` with the `placeholder` and the `ParamStore` in the table
 * scan parameter.
 * OceanBase use plan cache to enhance SQL query performance. If some SQLs with the same format but just
 * have difference const value, they can share the same physical plan.
 * For example,
 * SQL A: SELECT a, b, c FROM `t` WHERE id=1 and name='ocean';
 * SQL B: SELECT a, b, c FROM `t` WHERE id=2 and name='base';
 * The cached physical plan is:
 * SELECT a, b, c FROM `t` WHERE id={0} and name={1};
 * The constants were parameterized to be {0} and {1} which is the `QuestionMarkSqlFilterExpr`.
 */
public class QuestionMarkSqlFilterExpr implements SqlFilterExpr {
    private final FieldType type;
    private final long placeholderIndex;

    public QuestionMarkSqlFilterExpr(FieldType type, long placeholderIndex) {
        this.type = type;
        this.placeholderIndex = placeholderIndex;
    }

    FieldType getType() { return type; }
    public long getPlaceholderIndex() { return placeholderIndex; }
}
