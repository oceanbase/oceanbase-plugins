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

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;

public class JdbcConfig {
    public String jdbc_url;
    public String user;
    public String password;
    public String table;

    static JdbcConfig of(String parameters) {
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false);
        try {
            JdbcConfig config = objectMapper.readValue(parameters, JdbcConfig.class);
            if (config.jdbc_url == null || config.user == null || config.table == null) {
                throw new IllegalArgumentException("jdbc url, user or table is null.");
            }
            return config;
        } catch (JsonProcessingException e) {
            throw new RuntimeException(String.format("failed to parse json: %s", parameters), e);
        }
    }

    public String toString() {
        ObjectMapper objectMapper = new ObjectMapper();
        try {
            return objectMapper.writeValueAsString(this);
        } catch (JsonProcessingException e) {
            throw new RuntimeException("failed to json", e);
        }
    }
    public String toDisplayString() {
        JdbcConfig other = new JdbcConfig();
        other.jdbc_url = this.jdbc_url;
        other.user = this.user;
        other.password = "****";
        other.table = this.table;
        ObjectMapper objectMapper = new ObjectMapper();
        try {
            return objectMapper.writeValueAsString(other);
        } catch (JsonProcessingException e) {
            throw new RuntimeException("failed to json string", e);
        }
    }
}
