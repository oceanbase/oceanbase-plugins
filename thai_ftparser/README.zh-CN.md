# OceanBase 泰语全文解析插件（Thai Fulltext Parser Plugin）

面向 OceanBase 的泰语全文解析插件。核心使用 JNI 桥接 Java 分词库（已集成 Apache Lucene ThaiAnalyzer）。

## 🎯 功能特性

- ✅ 兼容 OceanBase FTParser 接口（`thai_ftparser_main.cpp`）
- ✅ JNI 集成调用 Java 泰语分词（Lucene ThaiAnalyzer）
- ✅ UTF-8 多字节泰文处理
- ✅ 并发安全：单 JVM 复用 + 线程独立 `JNIEnv*`（`thread_local`）
- ✅ 局部引用安全：`PushLocalFrame/PopLocalFrame` 管理 JNI 局部引用
- ✅ 可扩展：可替换 Java 分词实现

## 🔧 编译

### 环境准备

1. 安装编译基础
```bash
yum install -y git cmake make glibc-devel glibc-headers gcc gcc-c++
```
该命令将会安装gcc开发环境。

> 如果你的环境已经具备可以跳过当前步骤

2. 安装OceanBase 插件开发套件
```bash
yum install -y oceanbase-plugin-dev-kit
```

### 编译

```bash
# 选择一个你自己的工作目录
cd `your/workspace`
# 下载源码
git clone https://github.com/oceanbase/oceanbase-plugins
# 编译
cd oceanbase-plugins/thai_ftparser
mkdir build
cd build
cmake ..
make
```
你将会在build目录下看到libthai_ftparser.so文件，这个就是动态库插件。

## 🚀 快速开始

### 一键部署（推荐方式）

**最简单的方法**：直接将整个 `java/` 文件夹复制到 Observer 工作目录

```bash
# 1. 复制插件动态库
cp /path/to/yourplugindirpath/libthai_ftparser.so /path/to/observer/plugin_dir/

# 2. 复制整个 java 目录（无需其他的目录配置）
cp -r java/ /path/to/observer/java/

# 3. 安装所需依赖
yum install java-1.8.0-openjdk-devel -y
yum install libaio -y

# 4. 启动 Observer 并加载插件
# 连接数据库
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # 这里以dify的数据库连接信息为例

# 在sys租户中设置插件加载
ALTER SYSTEM SET plugins_load='libthai_ftparser.so:on';

# 重启Observer生效
killall observer
cd /path/to/observer
./bin/observer  # 在observer工作目录执行

# 验证安装（见下文）

# 5. 在Dify中的配置
# 如果需要在Docker启动的Dify中使用泰语分词器：
cd docker
bash setup-mysql-env.sh
# 修改.env中的配置：
# OCEANBASE_ENABLE_HYBRID_SEARCH=true 
# OCEANBASE_FULLTEXT_PARSER=thai_ftparser
docker compose up -d
# 进入oceanbase容器：docker exec -it <container_name> bash
# 容器内observer工作目录为 /root/ob/observer
# 使用docker cp /path/to/thai-ftparser <container_name>:/path/to/thai-ftparser
# 按照上述步骤1-4配置插件
```


> 📖 **详细插件使用说明**：参考 [OceanBase Plugin Development Kit 用户手册](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/)

### 依赖寻找优先级

插件按以下优先级自动寻找Java依赖：

1. **🥇 环境变量**（最高优先级）
   ```bash
   export THAI_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path"
   ```

2. **🥈 Observer 工作目录**（推荐）
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar  
   ${OB_WORKDIR}/java/RealThaiSegmenter.class
   ```

3. **🥉 插件相对路径**（开发环境）
   ```
   ../java/lib/lucene-*.jar
   ./java/lib/lucene-*.jar
   ../../java/lib/lucene-*.jar
   ```

**💡 建议**：使用方式2（复制java目录），无需配置THAI_PARSER_CLASSPATH，快速体验

### 验证安装

```sql
-- 检查插件是否加载成功
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'thai_ftparser';

-- 创建测试表（注意shell的字符集编码用UTF-8）
CREATE TABLE t_thai (
    c1 INT, 
    c2 VARCHAR(200), 
    c3 TEXT, 
    FULLTEXT INDEX (c2, c3) WITH PARSER thai_ftparser
);

-- 插入泰语测试数据
INSERT INTO t_thai (c1, c2, c3) VALUES
(1, 'สวัสดีค่ะ', 'สวัสดีค่ะ ยินดีต้อนรับสู่เว็บไซต์ของเรา'),
(2, 'ขอบคุณครับ', 'ขอบคุณที่เข้ามาเยี่ยมชม'),
(3, 'สอบถามเพิ่มเติม', 'หากมีคำถามสามารถติดต่อเราได้ทุกวันทำการ'),
(4, 'ขอบคุณมาก', 'ขอบคุณที่ใช้บริการของเรา'),
(5, 'ยินดีต้อนรับ', 'ยินดีต้อนรับสู่ OceanBase'),
(6, 'สวัสดีครับ', 'สวัสดีครับ หวังว่าจะได้พบกันอีกเร็วๆ นี้'),
(7, 'เป็นอย่างไรบ้าง', 'เป็นอย่างไรบ้างในช่วงเวลาที่ผ่านมา'),
(8, 'สอบถามเพิ่มเติม', 'หากมีคำถามสามารถติดต่อเราได้ทุกวันทำการ'),
(9, 'ไม่มีปัญหา', 'ไม่มีปัญหาใดๆ เลย'),
(10, 'ได้โปรดกรอกแบบฟอร์ม', 'กรุณากรอกข้อมูลให้ครบถ้วน'),
(11, 'ขอบคุณและหวังว่าจะกลับมาใหม่', 'ขอบคุณและหวังว่าจะได้พบกันอีกในอนาคต'),
(12, 'สวัสดีค่ะสวัสดีครับ', 'สวัสดีค่ะ สวัสดีครับ'),
(13, 'ทดสอบซ้ำๆ', 'ทดสอบซ้ำๆ เพื่อดูว่า parser จัดการคำซ้ำได้ดีไหม'),
(14, 'เป็นอะไรก็ได้', 'เป็นอะไรก็ได้ที่คุณต้องการ'),
(15, 'ไม่มีใครเข้าใจ', 'ไม่มีใครเข้าใจสิ่งที่เกิดขึ้น'),
(16, 'เป็นไปตามปกติ', 'เป็นไปตามปกติทุกประการ'),
(17, '2025 ปีนี้เป็นปีที่ดี', 'ปี 2025 เป็นปีที่ดีสำหรับการพัฒนา'),
(18, 'ขอบคุณ 2025', 'ขอบคุณในปี 2025');
-- 测试全文搜索功能
-- 测试 1：匹配单个词（预计返回 c1 = 1, 6, 12）
SELECT * FROM t_thai 
WHERE MATCH(c2, c3) AGAINST('สวัสดี' IN NATURAL LANGUAGE MODE);

-- 测试 2：匹配多个词（预计返回 c1 = 1, 5, 6, 12）
SELECT * FROM t_thai 
WHERE MATCH(c2, c3) AGAINST('สวัสดี ยินดี' IN NATURAL LANGUAGE MODE);

-- 测试 3：停用词测试（如果 "เป็น" 是停用词，应该无结果）
SELECT * FROM t_thai 
WHERE MATCH(c2, c3) AGAINST('เป็น' IN NATURAL LANGUAGE MODE);

-- 测试 4：数字 + 泰语混合（预计返回 c1 = 17, 18）
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('2025' IN NATURAL LANGUAGE MODE);

-- 测试 5：重复词语（预计返回 c1 = 13）
SELECT * FROM t_thai 
WHERE MATCH(c2, c3) AGAINST('ทดสอบซ้ำๆ' IN NATURAL LANGUAGE MODE);

-- 测试 6：模糊匹配（预计返回 c1 = 2, 4, 11, 18）
SELECT * FROM t_thai 
WHERE MATCH(c2, c3) AGAINST('ขอบคุณ' IN NATURAL LANGUAGE MODE);
```

## 🏗️ 架构

### 四层设计

1. 插件接口层
   - 实现 OceanBase FTParser 回调
   - 插件生命周期与注册
   - C 接口

2. 解析核心层
   - 解析会话与 token 缓存
   - UTF-8 泰文处理
   - 集成 JNI 桥

3. JNI 桥接层
   - 单 JVM 复用（集中管理）
   - 每线程 `JNIEnv*` 自动获取与分离
   - `PushLocalFrame/PopLocalFrame` 管理局部引用
   - 异常捕获与错误码回传

4. Java 分词层
   - 具体泰语分词实现

