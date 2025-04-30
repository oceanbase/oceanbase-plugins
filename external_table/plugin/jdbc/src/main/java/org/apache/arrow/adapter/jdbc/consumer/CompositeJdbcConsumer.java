/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file CompositeJdbcConsumer
 * This is a copy of arrow-jdbc CompositeJdbcConsumer to fix
 * consumer.vector.getMinorType().getType() suppressed
 * exceptions.
 *
 * @note This file is not required to put under directory
 * org/apache/arrow/adapter/jdbc/consumer. Maven package command
 * will do the right thing and the CompositeJdbcConsumer.class
 * will be moved into target/org/apache/arrow/adapter/jdbc/consumer.
 * I move this file to the correspond directory to make IDEA happy.
 */

package org.apache.arrow.adapter.jdbc.consumer;

import org.apache.arrow.adapter.jdbc.JdbcFieldInfo;
import org.apache.arrow.adapter.jdbc.consumer.exceptions.JdbcConsumerException;
import org.apache.arrow.util.AutoCloseables;
import org.apache.arrow.vector.ValueVector;
import org.apache.arrow.vector.VectorSchemaRoot;
import org.apache.arrow.vector.types.pojo.ArrowType;

import java.io.IOException;
import java.sql.ResultSet;
import java.sql.SQLException;

/** Composite consumer which hold all consumers. It manages the consume and cleanup process. */
@SuppressWarnings("unused")
public class CompositeJdbcConsumer implements JdbcConsumer {

  private final JdbcConsumer[] consumers;

  /** Construct an instance. */
  public CompositeJdbcConsumer(JdbcConsumer[] consumers) {
    this.consumers = consumers;
  }

  @Override
  public void consume(ResultSet rs) throws SQLException, IOException {
    for (int i = 0; i < consumers.length; i++) {
      try {
        consumers[i].consume(rs);
      } catch (Exception e) {
        if (consumers[i] instanceof BaseConsumer) {
          BaseConsumer consumer = (BaseConsumer) consumers[i];
          JdbcFieldInfo fieldInfo =
              new JdbcFieldInfo(rs.getMetaData(), consumer.columnIndexInResultSet);
          ArrowType arrowType = null;
          try {
            arrowType = consumer.vector.getMinorType().getType();
          } catch (Exception ex) {
            // ignore
          }
          String typeName = consumer.vector.getMinorType().name();
          throw new JdbcConsumerException(
              String.format("Exception while consuming JDBC value. minorType is: %s", typeName), e, fieldInfo, arrowType);
        } else {
          throw e;
        }
      }
    }
  }

  @Override
  public void close() {

    try {
      // clean up
      AutoCloseables.close(consumers);
    } catch (Exception e) {
      throw new RuntimeException("Error occurred while releasing resources.", e);
    }
  }

  @Override
  public void resetValueVector(ValueVector vector) {}

  /** Reset inner consumers through vectors in the vector schema root. */
  public void resetVectorSchemaRoot(VectorSchemaRoot root) {
    assert root.getFieldVectors().size() == consumers.length;
    for (int i = 0; i < consumers.length; i++) {
      consumers[i].resetValueVector(root.getVector(i));
    }
  }
}
