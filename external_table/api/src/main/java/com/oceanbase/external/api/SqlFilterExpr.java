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

public interface SqlFilterExpr {

    enum Type {
        INVALID,
        COLUMN_REF,
        CONST_VALUE,
        QUESTION_MARK,
        NULL,
        FILTER,
        CMP_EQUAL,
        CMP_LESS_EQUAL,
        CMP_LESS,
        CMP_GREATER_EQUAL,
        CMP_GREATER,
        CMP_NOT_EQUAL,
        CMP_IS,
        CMP_IS_NOT,
        CMP_BETWEEN,
        CMP_NOT_BETWEEN,
        CMP_LIKE,
        CMP_NOT_LIKE,
        CMP_IN,
        CMP_NOT_IN,
        CMP_AND,
        CMP_OR,
        CMP_NOT,
    }
}
