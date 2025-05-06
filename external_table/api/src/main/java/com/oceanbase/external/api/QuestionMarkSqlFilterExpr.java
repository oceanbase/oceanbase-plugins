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
