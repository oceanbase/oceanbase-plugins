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

import java.io.IOException;
import java.util.Collections;
import java.util.List;
import java.util.Map;

import org.apache.arrow.vector.ipc.ArrowReader;

/**
 * A data source provides data.
 * To relational databases, it means a table with specific database settings.
 * A DataSource must provide constructor that accept two parameters:
 * - allocate: org.apache.arrow.memory.BufferAllocator, which used by Arrow;
 * - properties: java.util.Map<String, String>, which provides information to create the DataSource.
 */
public interface DataSource {
    /**
     * Describe which keys are sensitive.
     * Sensitive keys will not be printed in the log and console.
     * For example: password.
     */
    @SuppressWarnings("unused")
    default List<String> sensitiveKeys() { return Collections.emptyList(); }

    /**
     * Test which filter can be push down to external table data source
     * @param filters The filters to be tested
     * @return Each element is the filter serialized string.
     * Return empty string if you don't support the filter to be pushdown.
     * Why the filter should be serialized?
     * The filter is generated in `logical plan` and pushdown to `execute plan` and maybe transfer over `observer` nodes.
     */
    @SuppressWarnings("unused")
    default List<String> pushdownFilters(List<SqlFilter> filters) { return Collections.emptyList(); }

    /**
     * Create a scanner.
     * Scanner generate a stream of data.
     * @param scanParameters Extra parameters about this scanning. Refer to {@link TableScanParameter#of(Map, Map)}
     *                        for more details.
     */
    ArrowReader createScanner(Map<String, Object> scanParameters) throws IOException;
}
