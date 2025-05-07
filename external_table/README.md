# Java 外表插件

通常情况下，数据都是存储在数据库自己的存储空间内，而外表功能可以让OceanBase访问存放在外部的数据。
外表插件尝试通过插件的方式扩展外表访问的能力，当前使用Java语言实现了外表插件。

## 如何使用
### 环境准备
此插件支持OceanBase 4.4.0 及以上版本，并依赖 Java 运行时，使用此功能需要确保：
- OceanBase 版本不小于 4.4.0；
- JDK 不小于 11；
- OceanBase 外表插件 Jar 包；
- MySQL 客户端工具，比如 mysql、mariadb、obclient 等，或其它可以连接MySQL的客户端工具。

### 修改 OceanBase 配置项并重启

```sql
-- 启用 Java
alter system set ob_enable_java_env='true';
-- 设置 JDK 的 HOME 目录，通常设置为环境变量的 $JAVA_HOME 即可。
alter system set ob_java_home='`/java/home/path`';

-- 设置一些Java运行时选项，可以是一些调试选项。
-- 可以增加自己的选项，但是最少包含下面几个选项。
-- 可以使用该选项配置 CLASSPATH，比如 -Djava.class.path=/java/class/path1:/java/class/path2
alter system set ob_java_opts='-XX:-CriticalJNINatives -Djdk.lang.processReaperUseDefaultStackSize=true --add-opens=java.base/java.nio=org.apache.arrow.memory.core,ALL-UNNAMED';

-- 设置 OceanBase 外表插件jar包目录。只需要设置目录，不需要写jar包名称，OceanBase 会自动展开目录下的jar包。
alter system set ob_java_connector_path='/jdbc/plugin/jar/package/directory';
```

> NOTE: 
> 1. JDBC 依赖的底层是内置C++插件，不需要安装动态链接库。
> 2. 可以添加一些额外的Java启动选项，比如一些GC调试信息 -XX:+PrintGCDetails -XX:+PrintGCDateStamps -XX:+PrintTenuringDistribution -XX:+PrintHeapAtGC -XX:+PrintReferenceGC -XX:+PrintGCApplicationStoppedTime

上述步骤完成后，重启OceanBase，检查是否安装成功:

```sql
select * from oceanbase.gv$ob_plugins where type='EXTERNAL TABLE' and status='READY';
```

最少可以看到三个插件：

```text
+--------------+----------+-------+--------+----------------+---------+-----------------+------------------+-------------------+-----------------------+---------------+-----------------------------------------------------+
| SVR_IP       | SVR_PORT | NAME  | STATUS | TYPE           | LIBRARY | LIBRARY_VERSION | LIBRARY_REVISION | INTERFACE_VERSION | AUTHOR                | LICENSE       | DESCRIPTION                                         |
+--------------+----------+-------+--------+----------------+---------+-----------------+------------------+-------------------+-----------------------+---------------+-----------------------------------------------------+
| 127.0.0.1    |    55803 | java  | READY  | EXTERNAL TABLE | NULL    | 0.1.0           | NULL             | 0.2.0             | OceanBase Corporation | Mulan PubL v2 | This is the java external table data source plugin. |
| 127.0.0.1    |    55803 | jdbc  | READY  | EXTERNAL TABLE | NULL    | 0.1.0           | NULL             | 0.2.0             | OceanBase Corporation | Mulan PubL v2 | This is a java external data source                 |
| 127.0.0.1    |    55803 | mysql | READY  | EXTERNAL TABLE | NULL    | 0.1.0           | NULL             | 0.2.0             | OceanBase Corporation | Mulan PubL v2 | This is a java external data source                 |
+--------------+----------+-------+--------+----------------+---------+-----------------+------------------+-------------------+-----------------------+---------------+-----------------------------------------------------+
```

其中 java是Java外表插件的通用实现，jdbc是一个Java JDBC数据源，基于 java插件实现，MySQL是一个MySQL JDBC数据源，基于 jdbc插件实现。

## 如何构建
首先确保你的环境上安装了Java开发环境，并且版本大于等于 11。

```bash
git clone https://github.com/oceanbase/oceanbase-plugins.git
cd oceanbase-plugins/external_table
mvn clean package
```

执行完成后，可以找到 jar 包，这个包是我们需要用到的插件包：
```txt
./internal/target/oceanbase-external-plugin-0.1.0-SNAPSHOT-jar-with-dependencies.jar
```
其中 `0.1.0` 是版本号。

如果你的环境中安装了多个版本的JDK，可以使用下面的命令指定使用的JDK版本来打包：
```bash
JAVA_HOME=/usr/lib/jvm/jdk-19-oracle-x64/ mvn clean package
```

### 测试使用
我们将介绍如何通过外表访问MySQL数据库，因此需要有一个MySQL数据库（OceanBase 也有MySQL模式，因此也适用于OceanBase）。

假设你的测试环境信息是这样的：
- MySQL 数据库（测试数据源），运行机器 192.168.0.10，端口 3306。测试账号 `root`，密码为空；
- OceanBase 数据库，运行机器 192.168.0.20，端口 55800，测试账号 `root`，密码为空。

那么连接 MySQL 和 OceanBase 的命令是：
```shell
# 连接MySQL的 test 数据库
mysql -h 192.168.0.10 -P 3306 -uroot -Dtest -c -A

# 连接 OceanBase 的 test 数据库
mysql -h 192.168.0.20 -P 55800 -uroot -Dtest -c -A
```

在MySQL数据库中创建表 `lineitem`：
```sql
CREATE TABLE `lineitem` (
  `l_orderkey` bigint(20) NOT NULL,
  `l_partkey` bigint(20) NOT NULL,
  `l_suppkey` bigint(20) NOT NULL,
  `l_linenumber` bigint(20) NOT NULL,
  `l_quantity` bigint(20) NOT NULL,
  `l_extendedprice` decimal(10,2) NOT NULL,
  `l_discount` decimal(10,2) NOT NULL,
  `l_tax` decimal(10,2) NOT NULL,
  `l_returnflag` char(1) DEFAULT NULL,
  `l_linestatus` char(1) DEFAULT NULL,
  `l_shipdate` date DEFAULT NULL,
  `l_commitdate` date DEFAULT NULL,
  `l_receiptdate` date DEFAULT NULL,
  `l_shipinstruct` char(25) DEFAULT NULL,
  `l_shipmode` char(10) DEFAULT NULL,
  `l_comment` varchar(44) DEFAULT NULL,
  PRIMARY KEY (`l_orderkey`,`l_linenumber`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

插入一些测试数据：

```sql
INSERT INTO `lineitem` (
  `l_orderkey`, `l_partkey`, `l_suppkey`, `l_linenumber`, `l_quantity`, `l_extendedprice`, 
  `l_discount`, `l_tax`, `l_returnflag`, `l_linestatus`, `l_shipdate`, `l_commitdate`, 
  `l_receiptdate`, `l_shipinstruct`, `l_shipmode`, `l_comment`
) VALUES
(1, 155190, 7706, 1, 17, 21168.23, 0.04, 0.02, 'N', 'O', '1996-03-13', '1996-02-12', '1996-03-22', 'DELIVER IN PERSON', 'TRUCK', 'egular courts above the'),
(1, 67310, 7311, 2, 36, 45983.16, 0.09, 0.06, 'N', 'O', '1996-04-12', '1996-02-28', '1996-04-20', 'TAKE BACK RETURN', 'MAIL', 'ly final dependencies: slyly bold '),
(1, 63700, 3701, 3, 8, 13309.60, 0.10, 0.02, 'N', 'O', '1996-01-29', '1996-03-05', '1996-01-31', 'TAKE BACK RETURN', 'REG AIR', 'riously. regular, express dep'),
(1, 2132, 4633, 4, 28, 28955.64, 0.09, 0.06, 'N', 'O', '1996-04-21', '1996-03-30', '1996-05-16', 'NONE', 'AIR', 'lites. fluffily even de'),
(1, 24027, 1534, 5, 24, 22824.48, 0.10, 0.04, 'N', 'O', '1996-03-30', '1996-03-14', '1996-04-01', 'NONE', 'FOB', ' pending foxes. slyly re'),
(1, 15635, 638, 6, 32, 49620.16, 0.07, 0.02, 'N', 'O', '1996-01-30', '1996-02-07', '1996-02-03', 'DELIVER IN PERSON', 'MAIL', 'arefully slyly ex'),
(2, 106170, 1191, 1, 38, 44694.46, 0.00, 0.05, 'N', 'O', '1997-01-28', '1997-01-14', '1997-02-02', 'TAKE BACK RETURN', 'RAIL', 'ven requests. deposits breach a'),
(3, 4297, 1798, 1, 45, 54058.05, 0.06, 0.00, 'R', 'F', '1994-02-02', '1994-01-04', '1994-02-23', 'NONE', 'AIR', 'ongside of the furiously brave acco'),
(3, 19036, 6540, 2, 49, 46796.47, 0.10, 0.00, 'R', 'F', '1993-11-09', '1993-12-20', '1993-11-24', 'TAKE BACK RETURN', 'RAIL', ' unusual accounts. eve'),
(3, 128449, 3474, 3, 27, 39890.88, 0.06, 0.07, 'A', 'F', '1994-01-16', '1993-11-22', '1994-01-23', 'DELIVER IN PERSON', 'SHIP', 'nal foxes wake. ');
```

在OceanBase中创建外表：
```sql
create external table lineitem(
  l_orderkey           bigint,
  l_partkey            bigint,
  l_suppkey            bigint,
  l_linenumber         bigint,
  l_quantity           bigint,
  l_extendedprice      decimal(10,2),
  l_discount           decimal(10,2),
  l_tax                decimal(10,2),
  l_returnflag         char(1),
  l_linestatus         char(1),
  l_shipdate           date,
  l_commitdate         date,
  l_receiptdate        date,
  l_shipinstruct       char(25),
  l_shipmode           char(10),
  l_comment            varchar(44)
) PROPERTIES (
  TYPE='plugin',
  NAME='mysql',
  PARAMETERS=(
    jdbc_uri='jdbc:mysql://192.168.0.10:3306/test?useSSL=false',
    `user`='root',
    `password`='',
    `table`='lineitem')
);
```

> 访问 5.7 或更低版本的MySQL，需要使用 useSSL=false 参数（如果有必要）。

这时就可以直接在 OceanBase 中直接查询MySQL中的数据了：
```sql
MySQL [test]> select * from lineitem;
+------------+-----------+-----------+--------------+------------+-----------------+------------+-------+--------------+--------------+------------+--------------+---------------+-------------------+------------+-------------------------------------+
| l_orderkey | l_partkey | l_suppkey | l_linenumber | l_quantity | l_extendedprice | l_discount | l_tax | l_returnflag | l_linestatus | l_shipdate | l_commitdate | l_receiptdate | l_shipinstruct    | l_shipmode | l_comment                           |
+------------+-----------+-----------+--------------+------------+-----------------+------------+-------+--------------+--------------+------------+--------------+---------------+-------------------+------------+-------------------------------------+
|          1 |    155190 |      7706 |            1 |         17 |        21168.23 |       0.04 |  0.02 | N            | O            | 1996-03-13 | 1996-02-12   | 1996-03-22    | DELIVER IN PERSON | TRUCK      | egular courts above the             |
|          1 |     67310 |      7311 |            2 |         36 |        45983.16 |       0.09 |  0.06 | N            | O            | 1996-04-12 | 1996-02-28   | 1996-04-20    | TAKE BACK RETURN  | MAIL       | ly final dependencies: slyly bold   |
|          1 |     63700 |      3701 |            3 |          8 |        13309.60 |       0.10 |  0.02 | N            | O            | 1996-01-29 | 1996-03-05   | 1996-01-31    | TAKE BACK RETURN  | REG AIR    | riously. regular, express dep       |
|          1 |      2132 |      4633 |            4 |         28 |        28955.64 |       0.09 |  0.06 | N            | O            | 1996-04-21 | 1996-03-30   | 1996-05-16    | NONE              | AIR        | lites. fluffily even de             |
|          1 |     24027 |      1534 |            5 |         24 |        22824.48 |       0.10 |  0.04 | N            | O            | 1996-03-30 | 1996-03-14   | 1996-04-01    | NONE              | FOB        |  pending foxes. slyly re            |
|          1 |     15635 |       638 |            6 |         32 |        49620.16 |       0.07 |  0.02 | N            | O            | 1996-01-30 | 1996-02-07   | 1996-02-03    | DELIVER IN PERSON | MAIL       | arefully slyly ex                   |
|          2 |    106170 |      1191 |            1 |         38 |        44694.46 |       0.00 |  0.05 | N            | O            | 1997-01-28 | 1997-01-14   | 1997-02-02    | TAKE BACK RETURN  | RAIL       | ven requests. deposits breach a     |
|          3 |      4297 |      1798 |            1 |         45 |        54058.05 |       0.06 |  0.00 | R            | F            | 1994-02-02 | 1994-01-04   | 1994-02-23    | NONE              | AIR        | ongside of the furiously brave acco |
|          3 |     19036 |      6540 |            2 |         49 |        46796.47 |       0.10 |  0.00 | R            | F            | 1993-11-09 | 1993-12-20   | 1993-11-24    | TAKE BACK RETURN  | RAIL       |  unusual accounts. eve              |
|          3 |    128449 |      3474 |            3 |         27 |        39890.88 |       0.06 |  0.07 | A            | F            | 1994-01-16 | 1993-11-22   | 1994-01-23    | DELIVER IN PERSON | SHIP       | nal foxes wake.                     |
+------------+-----------+-----------+--------------+------------+-----------------+------------+-------+--------------+--------------+------------+--------------+---------------+-------------------+------------+-------------------------------------+
10 rows in set (21.93 sec)
```

### 功能限制
- OceanBase Oracle 模式暂未实现此功能；
- JSON/Array/GIS 等类型暂未支持；
- 不支持并发查询；
- 同一数据源多表 join 不会作为一条 SQL 下推到数据源；
- 聚合函数、limit等不会下推；
- Jar包不支持动态加载，在进程启动前就需要把Jar包放在指定的目录。一个目录下可以放多个jar包，都会加载；
- 无法通过插件设置查询超时时间；
- 表 PARAMETERS 属性不支持修改，必须重新建表。

### 常用工具下载
- [Oracle JDK 11 下载页面](https://www.oracle.com/cn/java/technologies/javase/jdk11-archive-downloads.html)
- [OpenJDK 下载页面](https://www.openlogic.com/openjdk-downloads?field_java_parent_version_target_id=406&field_operating_system_target_id=426&field_architecture_target_id=391&field_java_package_target_id=396)

### FAQ
#### show create table 中看到的 external\$tablecol1\$a 是什么意思？
外表中的字段都是生成的，用户不需要关心这个字符串。
external\$tablecol1\$a 中 external\$tablecol 是固定前缀，接着是列序号，最后是列名称，插件使用此名称访问外表信息。

#### unexpected error for jni
Java 程序以抛异常的形式返回异常，但是当前OceanBase并没有将异常信息转换为用户友好的信息。因此当Java插件抛出异常后，用户收到的错误信息并不直观，需要通过搜索日志来排查错误。
最常见的Java 插件错误信息：
ERROR 11053 (HY000): unexpected error for jni

## 当前有哪些插件
当前实现了三个插件，分别叫做 `java`、`jdbc`，`mysql`。
其中 `java` 是所有 Java 插件的基础，具体插件只要实现 `com.oceanbase.external.api.DataSource` 即可。
`jdbc` 插件支持使用JDBC访问的数据源，`com.oceanbase.external.jdbc.JdbcDataSource` 是它的 `DataSource` 实现。用户使用时指定 JDBC URL 即可访问远程数据库。
而 `mysql` 插件是一个特殊的 `jdbc` 插件，对 `jdbc` 的一些功能属性做了定制调整，比如列名称使用 '\`' 扩起来，其 `DataSource` 实现是 `com.oceanbase.external.mysql.MysqlJdbcDataSource`。

支持的插件(数据源)并没有支持动态或运行时注册的功能，未来有变更，可以通过 `com.oceanbase.external.internal.DataSourceFactory#dataSources` 查看支持哪些插件。

> 如果你对运行时注册插件感兴趣，欢迎提交PR。

## 如何新增插件
新增插件的方法非常简单，只需要实现接口 `com.oceanbase.external.api.DataSource` 即可。

`DataSource` 接口中最少只需要实现两个函数。

```java
import org.apache.arrow.memory.BufferAllocator;

/**
 * 构造函数
 * @param allocator Arrow 中的allocator，供Arrow分配内存使用。
 * @param properties 创建数据源时指定的参数，通常是用户通过 `create external table` 时传入的参数。
 */
DataSource(BufferAllocator allocator, Map<String, String> properties);

/**
 * 创建遍历数据的Scanner
 * 该函数返回一个 ArrowReader，它每次可以读取一批数据。
 * @param scanParameters 在扫描器时的额外参数。可以结合构造函数中的 `properties`，创建一个 
 *                       `com.oceanbase.external.api.TableScanParameter` 对象。
 */
ArrowReader createScanner(Map<String, Object> scanParameters) throws IOException;

```

如果还要实现更多的功能，可以参考接口 `com.oceanbase.external.api.DataSource` 的定义。

如果你想新增数据源实现，或者优化此 Java 代码，欢迎提 PR。

## 实现原理
### OceanBase 外表相关的功能模块
OceanBase 外表涉及以下几个模块。

| 模块 | 外表插件                          | 说明 |
| --- |-------------------------------| ---- |
| SQL Parser | 不相关                           | 解析SQL语句 |
| Resolver | 接口 `DataSource#sensitiveKeys`   | Resolver 阶段对SQL语句进行校验，创建 'generated column'，标记敏感字段(比如 password)。那 sensitiveKeys 接口就用来注明哪些字段是敏感字段 |
| 并行查询 | 当前未涉及                         | OceanBase 并行查询框架支持并行查询，但由于JDBC与相关数据库均无法支持多个连接查询时保持数据一致性的功能，因此当前插件不支持并行查询 |
| 数据传输 | 输出 apache arrow 格式数据          | Apache Arrow 描述了高效的内存数据存储格式，并且支持了多种语言的实现、C++ 与 Java 之间零拷贝传输数据 |
| 字符编码 | 数据交互使用 utf8 编码                | 插件与OceanBase数据交互使用 apache arrow 库，该库也采用 utf8 编码，同时当前已知主流数据库均支持 utf8 编码 |
| 谓词下推 | 接口 `DataSource#pushdownFilters` | 谓词 (filter) ，对应 SQL 中的 WHERE 条件，可以部分下推到插件，插件自己决定支持哪些类型的谓词 |

### C++ 与 Java 的交互
OceanBase Kernel(observer)是使用C++编写的，而此外表插件功能是使用Java编写的，因此OceanBase与此插件之间使用JNI来交互。
为了方便 C++ 使用，插件基础库(internal 模块)提供了一些基础能力，比如：
- `com.oceanbase.external.internal.DataSourceFactory#create`: 根据用户指定的属性信息创建一个 `DataSource`；
- `com.oceanbase.external.internal.DataSourceFactory#dataSourceNames`: 当前都支持哪些Java插件(数据源)。这些将会在 OCEANBASE.GV$OB_PLUGINS 表中展示出来；
- `com.oceanbase.external.internal.JniUtils#getExceptionMessage`: 输出异常信息，方便排查问题；
- `com.oceanbase.external.internal.JniUtils#parseSqlFilterFromArrow`: observer 将 filter（谓词）信息使用 arrow 格式表示，这里将 Arrow 描述的Filter表达式转换为Java的；
- `com.oceanbase.external.internal.JniUtils#exportArrowStream`: 从 Java 中导出一批行数据到 C++ 中。

### C++ 与 Java 的数据传输
observer与插件传输数据时，使用[Apache Arrow](https://arrow.apache.org/)。
Arrow 描述了一种高效的内存列格式，对向量运算友好，并且有多种语言的实现，包括C++、Java、Python等。并且在C++与Java之间支持[零拷贝](https://arrow.apache.org/docs/java/cdata.html)。
当前OceanBase也支持向量运算，并且在内存中使用列存储保存常见的数据类型，比如 int64、double 等，因此将Arrow数据转换成OceanBase内部数据，效率也比较高。
需要指出的是，除了表数据的传输，Filter表达式也使用Arrow传输数据（不是必须的，方案也可以选择JSON或其它）。
