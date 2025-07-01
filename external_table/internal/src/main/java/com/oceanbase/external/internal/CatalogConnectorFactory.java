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

package com.oceanbase.external.internal;

import com.oceanbase.external.api.CatalogConnector;
import com.oceanbase.external.api.Constants;
import com.oceanbase.external.api.DataSource;
import org.apache.arrow.memory.BufferAllocator;

import java.lang.reflect.Constructor;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

@SuppressWarnings("unused")
public class CatalogConnectorFactory {
    private final static Map<String, String> connectorNames = new HashMap<String, String>(){{
        put("jdbc", "com.oceanbase.external.jdbc.JdbcCatalogConnector");
        put("mysql", "com.oceanbase.external.mysql.MysqlCatalogConnector");
    }};

    public static CatalogConnector create(Map<String, String> properties) {
        Objects.requireNonNull(properties, "properties should not be null");

        String pluginClass = properties.get(Constants.PLUGIN_CLASS_KEY);
        if (pluginClass == null) {

            String pluginName = properties.get(Constants.PLUGIN_NAME_KEY);
            if (pluginName != null) {
                pluginClass = connectorNames.get(pluginName);
            }
        }
        if (pluginClass == null) {
            throw new IllegalArgumentException("You should specify the plugin_class or plugin name: " +
                connectorNames.keySet());
        }

        try {
            Class<?> dataSourceClass = Class.forName(pluginClass);
            Constructor<?> constructor = dataSourceClass.getConstructor(BufferAllocator.class, Map.class);
            return (CatalogConnector) constructor.newInstance(JniUtils.getAllocator(pluginClass), properties);
        } catch (ClassNotFoundException ex) {
            throw new RuntimeException("failed to load catalog connector class: " + pluginClass, ex);
        } catch (NoSuchMethodException ex) {
            throw new RuntimeException("catalog connector class doesn't has a property constructor which needs 2 arguments: "
                + " BufferAllocator and Map<String, String>", ex);
        } catch (Exception ex) {
            throw new RuntimeException("failed to create catalog connector instance", ex);
        }
    }
}
