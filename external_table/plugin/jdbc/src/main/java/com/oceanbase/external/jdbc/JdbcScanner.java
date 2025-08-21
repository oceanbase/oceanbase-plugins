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

import org.apache.arrow.adapter.jdbc.ArrowVectorIterator;
import org.apache.arrow.adapter.jdbc.JdbcToArrow;
import org.apache.arrow.adapter.jdbc.JdbcToArrowConfig;
import org.apache.arrow.adapter.jdbc.JdbcToArrowUtils;
import org.apache.arrow.memory.ArrowBuf;
import org.apache.arrow.vector.BaseFixedWidthVector;
import org.apache.arrow.vector.FieldVector;
import org.apache.arrow.vector.VectorSchemaRoot;
import org.apache.arrow.vector.VectorUnloader;
import org.apache.arrow.vector.ipc.ArrowReader;
import org.apache.arrow.vector.ipc.message.ArrowRecordBatch;
import org.apache.arrow.vector.types.pojo.Schema;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

/// copy from org.apache.arrow.adbc.driver.jdbc.JdbcArrowReader
/// Because JdbcArrowReader doesn't support type conversion config.
public class JdbcScanner extends ArrowReader {
    private final static Logger logger = LoggerFactory.getLogger(JdbcScanner.class);

    private final Connection connection;
    private final Statement statement;
    private final ResultSet resultSet;
    private final ArrowVectorIterator delegate;
    private final Schema schema;
    private long bytesRead;

    public JdbcScanner(Connection connection,
                       Statement statement,
                       ResultSet resultSet,
                       JdbcToArrowConfig config)
            throws SQLException, IOException {
        super(config.getAllocator());
        this.connection = connection;
        this.statement = statement;
        this.resultSet = resultSet;
        this.delegate = JdbcToArrow.sqlToArrowVectorIterator(resultSet, config);
        this.schema = JdbcToArrowUtils.jdbcToArrowSchema(resultSet.getMetaData(), config);
    }

    @Override
    public boolean loadNextBatch() throws IOException {
        if (!delegate.hasNext()) return false;

        // root will be reused, so we can't close it
        final VectorSchemaRoot root = delegate.next();
        final VectorUnloader unloader = new VectorUnloader(root);
        try (final ArrowRecordBatch recordBatch = unloader.getRecordBatch()) {
            long thisBytesRead = recordBatch.computeBodyLength();
            bytesRead += thisBytesRead;
            if (thisBytesRead >= (2L * 1024 * 1024 * 1024)) {
                logger.info("read more than 2G for one batch: {}", thisBytesRead);
            }
            loadRecordBatch(recordBatch);
        }
        return true;
    }

    @Override
    public long bytesRead() {
        return bytesRead;
    }

    @Override
    protected void closeReadSource() throws IOException {
        try {
            delegate.close();

            if (!resultSet.isClosed()) {
                resultSet.close();
            }

            if (!statement.isClosed()) {
                statement.close();
            }

            if (!connection.isClosed()) {
                connection.close();
            }
        } catch (SQLException e) {
            throw new IOException(e);
        }
    }

    @Override
    protected Schema readSchema() throws IOException {
        return schema;
    }
}
