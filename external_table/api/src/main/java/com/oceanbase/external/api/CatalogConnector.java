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

import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.vector.ipc.ArrowReader;

import java.io.IOException;
import java.util.Map;

public abstract class CatalogConnector implements AutoCloseable {
    protected final BufferAllocator allocator;
    protected final Map<String, String> properties;

    public CatalogConnector(BufferAllocator allocator, Map<String, String> properties) {
        this.allocator = allocator;
        this.properties = properties;
    }

    public abstract void open() throws IOException;

    public abstract ArrowReader getDatabases() throws IOException;
    public abstract ArrowReader getTables(String database) throws IOException;
    public abstract ArrowReader getTableSchema(String database,String table) throws IOException;

    public abstract DataSource  createDataSource(String database, String table, Map<String, String> properties) throws IOException;
}
