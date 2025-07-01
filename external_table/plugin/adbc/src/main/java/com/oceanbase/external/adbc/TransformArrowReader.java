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

import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.vector.VectorSchemaRoot;
import org.apache.arrow.vector.VectorUnloader;
import org.apache.arrow.vector.ipc.ArrowReader;
import org.apache.arrow.vector.types.pojo.Schema;

import java.io.IOException;
import java.util.Collections;
import java.util.Iterator;
import java.util.function.Function;

public class TransformArrowReader extends ArrowReader {
    private final ArrowReader parent;
    private final Function<VectorSchemaRoot, Iterator<VectorSchemaRoot>> transformer;

    private Iterator<VectorSchemaRoot> transformedVectorSchemaRoot = Collections.emptyIterator();

    public TransformArrowReader(BufferAllocator allocator,
                                ArrowReader parent,
                                Function<VectorSchemaRoot, Iterator<VectorSchemaRoot>> transformer) {
        super(allocator);
        this.parent = parent;
        this.transformer = transformer;
    }

    @Override
    public boolean loadNextBatch() throws IOException {
        if (!transformedVectorSchemaRoot.hasNext()) {
            if (!parent.loadNextBatch()) {
                return false;
            }

            transformedVectorSchemaRoot = transformer.apply(parent.getVectorSchemaRoot());
        }

        if (!transformedVectorSchemaRoot.hasNext()) {
            return false;
        }

        try (VectorSchemaRoot root = transformedVectorSchemaRoot.next()) {
            loadRecordBatch(new VectorUnloader(root).getRecordBatch());
            return true;
        }
    }

    @Override
    public long bytesRead() {
        return 0;
    }

    @Override
    protected void closeReadSource() throws IOException {
        transformedVectorSchemaRoot.forEachRemaining(VectorSchemaRoot::close);
        parent.close();
    }

    @Override
    protected Schema readSchema() throws IOException {
        VectorSchemaRoot root = parent.getVectorSchemaRoot();
        Iterator<VectorSchemaRoot> transformed = transformer.apply(root);
        if (!transformed.hasNext()) {
            throw new IOException("failed to fetch children schema");
        }

        Schema schema;
        try (VectorSchemaRoot transformedRoot = transformed.next()) {
            schema = transformedRoot.getSchema();
            transformed.forEachRemaining(VectorSchemaRoot::close);
            return schema;
        }
    }
}
