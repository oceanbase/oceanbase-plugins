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

import com.oceanbase.external.api.DataSource;

import org.apache.arrow.memory.BufferAllocator;

import java.lang.reflect.Constructor;
import java.lang.IllegalArgumentException;
import java.util.*;

@SuppressWarnings("unused")
public class DataSourceFactory {

    private final static String PLUGIN_CLASS_KEY = "plugin_class";

    /// 'ob.' means this property is passed through oceanbase plugin adaptor not by user.
    private final static String PLUGIN_NAME_KEY  = "ob.plugin_name";

    private final static Map<String, String> dataSources = new HashMap<String, String>(){{
            put("jdbc", "com.oceanbase.external.jdbc.JdbcDataSource");
            put("mysql", "com.oceanbase.external.mysql.MysqlJdbcDataSource");
        }};

    public static DataSource create(Map<String, String> properties) {
        if (properties == null) {
            throw new IllegalArgumentException("parameter properties is null");
        }

        String pluginClass = properties.get(PLUGIN_CLASS_KEY);
        if (pluginClass == null) {

            String pluginName = properties.get(PLUGIN_NAME_KEY);
            if (pluginName != null) {
                pluginClass = dataSources.get(pluginName);
            }
        }
        if (pluginClass == null) {
            throw new IllegalArgumentException("You should specify the plugin_class or plugin name: " +
                    dataSourceNames());
        }

        // plugin class can be a key or real class name.
        pluginClass = dataSources.getOrDefault(pluginClass, pluginClass);
        try {
            Class<?> dataSourceClass = Class.forName(pluginClass);
            Constructor<?> constructor = dataSourceClass.getConstructor(BufferAllocator.class, Map.class);
            return (DataSource) constructor.newInstance(JniUtils.getAllocator(), properties);
        } catch (ClassNotFoundException ex) {
            throw new RuntimeException("failed to load data source class: " + pluginClass, ex);
        } catch (NoSuchMethodException ex) {
            throw new RuntimeException("data source class doesn't has a property constructor which needs 2 arguments: "
                    + " BufferAllocator and Map", ex);
        } catch (Exception ex) {
            throw new RuntimeException("failed to create data source instance", ex);
        }
    }

    public static List<String> dataSourceNames() {
        return new ArrayList<>(dataSources.keySet());
    }
}
