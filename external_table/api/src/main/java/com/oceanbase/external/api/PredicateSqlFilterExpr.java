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

import java.util.List;

public class PredicateSqlFilterExpr implements SqlFilterExpr {
    private final Type type;
    private final List<SqlFilterExpr> children;

    public enum Type {
        INVALID,
        EQUAL,
        LESS_EQUAL,
        LESS_THAN,
        GREATER_EQUAL,
        GREATER_THAN,
        NOT_EQUAL,
        IS,
        IS_NOT,
        BETWEEN,
        NOT_BETWEEN,
        LIKE,
        NOT_LIKE,
        IN,
        NOT_IN,
        AND,
        OR,
        NOT
    }

    public PredicateSqlFilterExpr(Type type, List<SqlFilterExpr> children) {
        this.type = type;
        this.children = children;
    }

    public Type getType() { return type; }
    public List<SqlFilterExpr> getChildren() { return children; }
}
