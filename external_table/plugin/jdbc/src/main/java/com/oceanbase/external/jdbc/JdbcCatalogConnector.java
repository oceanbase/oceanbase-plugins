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

import com.oceanbase.external.adbc.AdbcCatalogConnector;
import com.oceanbase.external.api.Constants;
import org.apache.arrow.adbc.core.AdbcConnection;
import org.apache.arrow.adbc.core.AdbcDatabase;
import org.apache.arrow.adbc.core.AdbcDriver;
import org.apache.arrow.adbc.core.AdbcException;
import org.apache.arrow.adbc.driver.jdbc.JdbcDriver;
import org.apache.arrow.memory.BufferAllocator;

import java.util.HashMap;
import java.util.Map;

public class JdbcCatalogConnector extends AdbcCatalogConnector {

    public JdbcCatalogConnector(BufferAllocator allocator, Map<String, String> properties) {
        super(allocator, properties);
    }

    @Override
    protected Map<String, Object> getConfig(Map<String, String> properties) {
        String parameters = properties.getOrDefault(Constants.PARAMETERS_KEY, "");
        JdbcConfig jdbcConfig = JdbcConfig.of(parameters);
        Map<String, Object> adbcConfig = new HashMap<>();
        AdbcDriver.PARAM_URI.set(adbcConfig, jdbcConfig.jdbc_url);
        AdbcDriver.PARAM_USERNAME.set(adbcConfig, jdbcConfig.user);
        AdbcDriver.PARAM_PASSWORD.set(adbcConfig, jdbcConfig.password);
        return adbcConfig;
    }

    @Override
    protected AdbcConnection getConnection() throws AdbcException {
        try (AdbcDatabase database = new JdbcDriver(allocator).open(config)) {
            return database.connect();
        }
    }
}
