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

import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.vector.ipc.ArrowReader;

/**
 * A data source provides data.
 * To relational databases, it means a table with specific database settings.
 * A DataSource must provide constructor that accept two parameters:
 * - allocate: org.apache.arrow.memory.BufferAllocator, which used by Arrow;
 * - properties: Map<String, String>, which provides information to create the DataSource.
 *   The `properties` contains the information of the `PROPERTIES` field in `create table` SQL.
 */
public abstract class DataSource {
    protected final BufferAllocator allocator;
    protected final Map<String, String> properties;

    public DataSource(BufferAllocator allocator, Map<String, String> properties) {
        this.allocator = allocator;
        this.properties = properties;
    }

    /**
     * Get a string that display the parameters
     * You can mask content if sensitive.
     * <p>
     * For example:
     * parameters = '{"table"="external","user"="root","password"="sensitive","jdbc_url"="jdbc://xxx"}'
     * return:
     * parameters = '{"table"="external","user"="root","password"="****","jdbc_url"="jdbc://xxx"}'
     * </p>
     * <p>
     * Please don't use '\'' in the string.
     * </p>
     */
    @SuppressWarnings("unused")
    public String toDisplayString() { return properties.getOrDefault(Constants.PARAMETERS_KEY, ""); }

    /**
     * Test which filter can be push down to external table data source
     * @param filters The filters to be tested
     * @return Each element is the filter serialized string.
     * Return empty string if you don't support the filter to be pushdown.
     * Why the filter should be serialized?
     * The filter is generated in `logical plan` and pushdown to `execute plan` and maybe transfer over `observer` nodes.
     */
    @SuppressWarnings("unused")
    public List<String> pushdownFilters(List<SqlFilter> filters) { return Collections.emptyList(); }

    /**
     * Create a scanner.
     * Scanner generate a stream of data.
     * @param scanParameters Extra parameters about this scanning. Refer to {@link TableScanParameter#of(Map)}
     *                        for more details.
     */
    public abstract ArrowReader createScanner(Map<String, Object> scanParameters) throws IOException;
}
