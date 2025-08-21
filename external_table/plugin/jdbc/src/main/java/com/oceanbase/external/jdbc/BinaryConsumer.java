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

import java.io.IOException;
import java.io.InputStream;
import java.sql.ResultSet;
import java.sql.SQLException;

import org.apache.arrow.adapter.jdbc.consumer.BaseConsumer;
import org.apache.arrow.memory.ArrowBuf;
import org.apache.arrow.vector.VarBinaryVector;
import org.apache.arrow.vector.BitVectorHelper;

/**
 * Consume binary data from JDBC
 * The {@link org.apache.arrow.adapter.jdbc.consumer.BinaryConsumer} has a bug when
 * consuming null, so I rewrite it.
 */
public class BinaryConsumer extends BaseConsumer<VarBinaryVector> {

    private final byte[] reuseBytes = new byte[1024];

    /** Instantiate a BinaryConsumer. */
    public BinaryConsumer(VarBinaryVector vector, int index) {
        super(vector, index);
        if (vector != null) {
            vector.allocateNewSafe();
        }
    }

    /** consume a InputStream. */
    public void consume(InputStream is) throws IOException {
        while (currentIndex >= vector.getValueCapacity()) {
            vector.reallocValidityAndOffsetBuffers();
        }

        final int startOffset = vector.getStartOffset(currentIndex);
        final ArrowBuf offsetBuffer = vector.getOffsetBuffer();
        int dataLength = 0;

        if (is != null) {
            int read;
            while ((read = is.read(reuseBytes)) != -1) {
                while (vector.getDataBuffer().capacity() < (startOffset + dataLength + read)) {
                    vector.reallocDataBuffer();
                }
                vector.getDataBuffer().setBytes(startOffset + dataLength, reuseBytes, 0, read);
                dataLength += read;
            }

            BitVectorHelper.setBit(vector.getValidityBuffer(), currentIndex);
        }
        offsetBuffer.setInt(
                (currentIndex + 1) * ((long) VarBinaryVector.OFFSET_WIDTH), startOffset + dataLength);
        vector.setLastSet(currentIndex);
    }

    @Override
    public void consume(ResultSet resultSet) throws SQLException, IOException {
        InputStream is = resultSet.getBinaryStream(columnIndexInResultSet);
        consume(is);
        moveWriterPosition();
    }

    public void moveWriterPosition() {
        currentIndex++;
    }

    @Override
    public void resetValueVector(VarBinaryVector vector) {
        this.vector = vector;
        this.vector.allocateNewSafe();
        this.currentIndex = 0;
    }
}
