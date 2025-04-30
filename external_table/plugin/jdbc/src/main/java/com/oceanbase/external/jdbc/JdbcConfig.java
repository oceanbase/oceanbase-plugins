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

import java.util.Map;

public class JdbcConfig {
    public String jdbc_uri;
    public String driver_class;
    public String user;
    public String password;

    static JdbcConfig of(Map<String, String> properties) {
        JdbcConfig config = new JdbcConfig();
        config.jdbc_uri = properties.get("jdbc_uri");
        config.driver_class = properties.get("driver_class");
        config.user = properties.get("user");
        config.password = properties.get("password");

        if (config.jdbc_uri == null || config.user == null) {
            throw new IllegalArgumentException("jdbc uri, user or table is null.");
        }
        return config;
    }
}
