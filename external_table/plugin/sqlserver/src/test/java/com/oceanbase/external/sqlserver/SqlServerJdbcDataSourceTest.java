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

package com.oceanbase.external.sqlserver;

import com.oceanbase.external.api.Constants;
import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.memory.RootAllocator;
import org.apache.arrow.vector.VectorSchemaRoot;
import org.apache.arrow.vector.ipc.ArrowReader;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Test class for SqlServerJdbcDataSource
 * 
 * To run integration tests, set the following environment variables:
 * - SQLSERVER_JDBC_URL: jdbc:sqlserver://localhost:1433;databaseName=testdb
 * - SQLSERVER_USER: your_username
 * - SQLSERVER_PASSWORD: your_password
 * - SQLSERVER_TABLE: test_table
 */
public class SqlServerJdbcDataSourceTest {

    private static final String ENV_JDBC_URL = System.getenv("SQLSERVER_JDBC_URL");
    private static final String ENV_USER = System.getenv("SQLSERVER_USER");
    private static final String ENV_PASSWORD = System.getenv("SQLSERVER_PASSWORD");
    private static final String ENV_TABLE = System.getenv("SQLSERVER_TABLE");

    private BufferAllocator allocator;

    @BeforeEach
    public void setUp() {
        allocator = new RootAllocator(Long.MAX_VALUE);
    }

    @AfterEach
    public void tearDown() {
        if (allocator != null) {
            allocator.close();
        }
    }

    @Test
    @DisplayName("Integration test: Connect to SQL Server and query data")
    public void testRealConnection() throws Exception {
        // Skip if environment variables are not set
        Assumptions.assumeTrue(ENV_JDBC_URL != null && ENV_USER != null && ENV_TABLE != null,
            "SQL Server connection info not provided. Set SQLSERVER_JDBC_URL, SQLSERVER_USER, SQLSERVER_PASSWORD, SQLSERVER_TABLE");

        // Validate JDBC URL format
        if (!ENV_JDBC_URL.startsWith("jdbc:sqlserver://")) {
            throw new IllegalArgumentException(
                "Invalid JDBC URL format. Expected: jdbc:sqlserver://host:port;databaseName=dbname\n" +
                "But got: " + ENV_JDBC_URL + "\n" +
                "Example: jdbc:sqlserver://localhost:1433;databaseName=mydb"
            );
        }

        Map<String, String> properties = new HashMap<>();
        String params = String.format(
            "{\"jdbc_url\":\"%s\",\"user\":\"%s\",\"password\":\"%s\",\"table\":\"%s\"}",
            ENV_JDBC_URL, ENV_USER, ENV_PASSWORD != null ? ENV_PASSWORD : "", ENV_TABLE
        );
        properties.put(Constants.PARAMETERS_KEY, params);

        SqlServerJdbcDataSource dataSource = new SqlServerJdbcDataSource(allocator, properties);
        
        System.out.println("DataSource created: " + dataSource.toDisplayString());

        // Create a simple scan parameter according to TableScanParameter.of() method
        Map<String, Object> scanParams = new HashMap<>();
        scanParams.put("columns", Arrays.asList("year_field"));  // 使用columns而不是required_fields
        scanParams.put("filters", Collections.emptyList());  // 使用filters而不是predicates
        scanParams.put("question_mark_values", Collections.emptyList());  // 添加question_mark_values参数

        System.out.println("\nStarting to scan table...");
        
        try (ArrowReader reader = dataSource.createScanner(scanParams)) {
            assertNotNull(reader, "Scanner should not be null");
            System.out.println("Scanner created successfully!");

            // Make sure the reader is initialized.
            // ensureInitialized() is in method of getVectorSchemaRoot().
            VectorSchemaRoot tempRoot = reader.getVectorSchemaRoot();
            assertNotNull(tempRoot, "VectorSchemaRoot should not be null");
            System.out.println("Schema: " + tempRoot.getSchema());
            
            int rowCount = 0;
            int batchCount = 0;
            
            // 使用标准的ArrowReader接口读取数据
            while (reader.loadNextBatch()) {
                batchCount++;
                VectorSchemaRoot root = reader.getVectorSchemaRoot();
                assertNotNull(root, "VectorSchemaRoot should not be null");
                
                int batchRows = root.getRowCount();
                rowCount += batchRows;
                
                System.out.println("\n=== Batch " + batchCount + ": " + batchRows + " rows ===");
                
                // 第一个批次显示schema信息
                if (batchCount == 1) {
                    System.out.println("Schema: " + root.getSchema());
                    System.out.println();
                }
                
                // 输出表内容
                printTableData(root, Math.min(batchRows, 100)); // 每批次最多显示10行
                
                // 限制输出用于测试
                if (batchCount >= 3) {
                    System.out.println("Stopping after 3 batches for testing");
                    break;
                }
            }
            
            System.out.println("Total rows read: " + rowCount);
            System.out.println("Total batches: " + batchCount);
            
            assertTrue(rowCount >= 0, "Should read at least 0 rows");
            assertTrue(batchCount > 0, "Should read at least 1 batch");
        }
    }

    /**
     * 打印表数据内容
     */
    private void printTableData(VectorSchemaRoot root, int maxRows) {
        if (root.getRowCount() == 0) {
            System.out.println("No data in this batch");
            return;
        }

        // 打印列标题
        System.out.print("Row\t");
        for (int col = 0; col < root.getFieldVectors().size(); col++) {
            String fieldName = root.getVector(col).getField().getName();
            System.out.print(fieldName + "\t");
        }
        System.out.println();

        // 打印分隔线
        System.out.print("---\t");
        for (int col = 0; col < root.getFieldVectors().size(); col++) {
            System.out.print("--------\t");
        }
        System.out.println();

        // 打印数据行
        for (int row = 0; row < Math.min(maxRows, root.getRowCount()); row++) {
            System.out.print((row + 1) + "\t");
            for (int col = 0; col < root.getFieldVectors().size(); col++) {
                Object value = root.getVector(col).getObject(row);
                String displayValue = (value == null) ? "NULL" : value.toString();
                
                // 根据字段类型设置不同的显示长度限制
                String fieldName = root.getVector(col).getField().getName().toLowerCase();
                int maxLength = 20; // 默认长度
                
                // 日期时间类型需要更多空间来完整显示
                if (fieldName.contains("date") || fieldName.contains("time")) {
                    maxLength = 50; // 足够显示完整的日期时间字符串
                }
                // 二进制类型可以适当更长
                else if (fieldName.contains("binary")) {
                    maxLength = 30;
                }
                
                // 限制显示长度，避免输出过长
                if (displayValue.length() > maxLength) {
                    displayValue = displayValue.substring(0, maxLength - 3) + "...";
                }
                System.out.print(displayValue + "\t");
            }
            System.out.println();
        }

        // 如果有更多行，显示省略信息
        if (root.getRowCount() > maxRows) {
            System.out.println("... (" + (root.getRowCount() - maxRows) + " more rows)");
        }
    }
}

