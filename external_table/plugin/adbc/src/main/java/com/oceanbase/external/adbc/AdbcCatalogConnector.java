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

package com.oceanbase.external.adbc;

import com.oceanbase.external.api.CatalogConnector;
import com.oceanbase.external.api.Constants;
import com.oceanbase.external.api.DataSource;
import org.apache.arrow.adbc.core.AdbcConnection;
import org.apache.arrow.adbc.core.AdbcException;
import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.vector.FieldVector;
import org.apache.arrow.vector.VectorSchemaRoot;
import org.apache.arrow.vector.complex.ListVector;
import org.apache.arrow.vector.complex.StructVector;
import org.apache.arrow.vector.ipc.ArrowReader;
import org.apache.arrow.vector.types.Types;
import org.apache.arrow.vector.types.pojo.ArrowType;
import org.apache.arrow.vector.types.pojo.Field;
import org.apache.arrow.vector.types.pojo.FieldType;

import java.io.IOException;
import java.util.*;
import java.util.function.Function;

public abstract class AdbcCatalogConnector extends CatalogConnector {
    protected final Map<String, Object> config;

    private AdbcConnection connection = null;

    public AdbcCatalogConnector(BufferAllocator allocator, Map<String, String> properties) {
        super(allocator, properties);
        config = getConfig(properties);
    }

    @Override
    public void open() throws IOException {
        try {
            connection = getConnection();
        } catch (AdbcException e) {
            throw new IOException(e);
        }
    }

    @Override
    public ArrowReader getDatabases() throws IOException {
        try {
            ArrowReader arrowReader = connection.getObjects(
                AdbcConnection.GetObjectsDepth.DB_SCHEMAS, null/*catalog*/, null/*database*/, null/*table name*/, null, null);
            Function<VectorSchemaRoot, Iterator<VectorSchemaRoot>> transformer = vectorSchemaRoot -> {
                ListVector vector = (ListVector) vectorSchemaRoot.getVector(1);
                StructVector databasesVector = (StructVector) vector.getDataVector();
                FieldVector valueVector = databasesVector.getChild("db_schema_name");
                VectorSchemaRoot schemaRoot = VectorSchemaRoot.of(valueVector);
                return Collections.singletonList(schemaRoot).iterator();
            };

            return new TransformArrowReader(allocator, arrowReader, transformer);
        } catch (AdbcException e) {
            throw new IOException(e);
        }
    }

    @Override
    public ArrowReader getTables(String database) throws IOException {
        try {
            ArrowReader arrowReader =  connection.getObjects(
                AdbcConnection.GetObjectsDepth.TABLES, null/*catalog*/, database, null/*table name*/, new String[]{"TABLE"}, null);
            Function<VectorSchemaRoot, Iterator<VectorSchemaRoot>> transformer = vectorSchemaRoot -> {
                ListVector vector = (ListVector) vectorSchemaRoot.getVector(1);
                StructVector databasesVector = (StructVector) vector.getDataVector();
                ListVector tableListVector = databasesVector.getChild("db_schema_tables", ListVector.class);
                StructVector tableVector = (StructVector) tableListVector.getDataVector();
                FieldVector tableNameVector = tableVector.getChild("table_name");
                VectorSchemaRoot schemaRoot = VectorSchemaRoot.of(tableNameVector);
                return Collections.singletonList(schemaRoot).iterator();
            };
            return new TransformArrowReader(allocator, arrowReader, transformer);
        } catch (AdbcException e) {
            throw new IOException(e);
        }
    }

    @Override
    public ArrowReader getTableSchema(String database, String table) throws IOException {
        try {
            ArrowReader arrowReader = connection.getObjects(
                AdbcConnection.GetObjectsDepth.ALL, null/*catalog*/, database, table, null, null);
            Function<VectorSchemaRoot, Iterator<VectorSchemaRoot>> transformer = vectorSchemaRoot -> {
                ListVector vector = (ListVector) vectorSchemaRoot.getVector(1);
                StructVector databasesVector = (StructVector) vector.getDataVector();
                ListVector tableListVector = (ListVector) databasesVector.getChild("db_schema_tables");
                StructVector tableVector = (StructVector) tableListVector.getDataVector();
                ListVector tableColumnsVector = tableVector.getChild("table_columns", ListVector.class);
                StructVector columnsStructVector = (StructVector) tableColumnsVector.getDataVector();

                // ADBC 传来的数据比较多，而且字段名称不符合协议要求，这里做一下转换
                // 每个字段的含义可以参考 java.sql.DatabaseMetaData.getColumns 的注释。
                List<Field> dstFields = Arrays.asList(
                    new Field(
                        Constants.CATALOG_COLUMN_NAME_KEY,
                        FieldType.notNullable(ArrowType.Utf8.INSTANCE),
                        Collections.emptyList()),
                    new Field(
                        Constants.CATALOG_ORDINAL_POSITION_KEY,
                        FieldType.nullable(Types.MinorType.INT.getType()),
                        Collections.emptyList()),
                    new Field(
                        Constants.CATALOG_REMARKS_KEY,
                        FieldType.nullable(ArrowType.Utf8.INSTANCE),
                        Collections.emptyList()),
                    new Field(
                        Constants.CATALOG_TYPE_NAME_KEY,
                        FieldType.nullable(ArrowType.Utf8.INSTANCE),
                        Collections.emptyList()),
                    new Field(
                        Constants.CATALOG_COLUMN_SIZE_KEY,
                        FieldType.nullable(Types.MinorType.INT.getType()),
                        Collections.emptyList()),
                    new Field(
                        Constants.CATALOG_DECIMAL_DIGITS_KEY,
                        FieldType.nullable(Types.MinorType.SMALLINT.getType()),
                        Collections.emptyList())
                );
                // 下面的是ADBC原生的名字，可以参考 org.apache.arrow.adbc.core.StandardSchemas.COLUMN_SCHEMA
                // 或 org.apache.arrow.adbc.core.AdbcConnection.getObjects
                List<FieldVector> dstColumnVectors = Arrays.asList(
                    columnsStructVector.getChild("column_name"),
                    columnsStructVector.getChild("ordinal_position"),
                    columnsStructVector.getChild("remarks"),
                    columnsStructVector.getChild("xdbc_type_name"),
                    columnsStructVector.getChild("xdbc_column_size"),
                    columnsStructVector.getChild("xdbc_decimal_digits")
                );
                VectorSchemaRoot schemaRoot = new VectorSchemaRoot(dstFields, dstColumnVectors);
                return Collections.singletonList(schemaRoot).iterator();
            };
            return new TransformArrowReader(allocator, arrowReader, transformer);
        } catch (AdbcException e) {
            throw new IOException(e);
        }
    }

    @Override
    public DataSource createDataSource(String database, String table, Map<String, String> properties) throws IOException {
        return null;
    }

    @Override
    public void close() throws Exception {
        if (connection != null) {
            connection.close();
            connection = null;
        }
    }

    protected Map<String, Object> getConfig(Map<String, String> properties) {
        return new HashMap<>(properties);
    }

    protected AdbcConnection getConnection() throws AdbcException {
        throw new RuntimeException("no implementation");
    }
}
