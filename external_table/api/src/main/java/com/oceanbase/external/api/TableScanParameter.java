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

import java.util.Collections;
import java.util.Map;
import java.util.List;

public class TableScanParameter {

    private String tableName;
    private List<String> columns;
    private List<String> sqlFilters;

    /// refer to {@link QuestionMarkSqlFilterExpr} for information about question mark expression
    private List<Object> questionMarkValues;

    TableScanParameter() {}

    public static TableScanParameter of(Map<String, Object> tableScanParamMap, Map<String, String> properties) {
        final String tableNameKey = "table";
        final String columnsKey = "columns";
        final String sqlFilterKey = "filters";
        final String questionMarkKey = "question_mark_values";

        TableScanParameter param = new TableScanParameter();
        param.tableName = properties.getOrDefault(tableNameKey, (String)(tableScanParamMap.getOrDefault(tableNameKey, "")));
        param.columns = (List<String>) tableScanParamMap.getOrDefault(columnsKey, Collections.emptyList());
        param.sqlFilters = (List<String>) tableScanParamMap.getOrDefault(sqlFilterKey, Collections.emptyList());
        param.questionMarkValues = (List<Object>) tableScanParamMap.getOrDefault(questionMarkKey, Collections.emptyList());
        return param;
    }

    public String getTableName() { return this.tableName; }
    public List<String> getColumns() { return this.columns; }
    public List<String> getSqlFilters() { return this.sqlFilters; }
    public List<Object> getQuestionMarkValues() { return this.questionMarkValues; }

    public String toString() {
        return "TableScanParameter{" +
                "tableName='" + tableName + '\'' +
                ", columns=" + columns +
                '}';
    }
}
