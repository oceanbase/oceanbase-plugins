# OceanBase 泰语全文解析插件（Thai Fulltext Parser Plugin）

面向 OceanBase 的泰语全文解析插件。核心使用 JNI 桥接 Java 分词库（已集成 Apache Lucene ThaiAnalyzer）。

## 功能特性

- ✅ 兼容 OceanBase FTParser 接口（`thai_ftparser_main.cpp`）
- ✅ JNI 集成调用 Java 泰语分词（Lucene ThaiAnalyzer）
- ✅ UTF-8 多字节泰文处理
- ✅ 可扩展：可替换 Java 分词实现

## 编译

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

## 快速开始

### 部署安装

**推荐方法**：分别复制.class文件和jar文件到对应位置

```bash
# 1. 复制插件动态库
cp /path/to/yourplugindirpath/libthai_ftparser.so /path/to/observer/plugin_dir/

# 2. 创建java目录结构（如果不存在）
mkdir -p /path/to/observer/java/lib

# 3. 复制Lucene依赖库
cp java/lib/lucene-core-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-common-8.11.2.jar /path/to/observer/java/lib/

# 4. 复制泰语分词器类文件
cp java/ThaiSegmenter.class /path/to/observer/java/

# 5. 安装Java环境
yum install java-1.8.0-openjdk-devel -y

# 6. 启动 Observer 并加载插件
# 连接数据库
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # 这里以dify的数据库连接信息为例

# 在sys租户中设置插件加载
ALTER SYSTEM SET plugins_load='libthai_ftparser.so:on';

# 重启Observer生效
killall observer
cd /path/to/observer
./bin/observer  # 在observer工作目录执行启动observer

# 验证安装（见下文）
```

**多插件共存说明**：
- 如果已安装其他语言分词插件，只需复制泰语分词器的.class文件
- `lucene-core-8.11.2.jar` 和 `lucene-analyzers-common-8.11.2.jar` 被所有插件共享
- 泰语分词器使用ThaiAnalyzer，无需额外的语言专用jar包
- 文件已存在时，cp命令会询问是否覆盖，可选择跳过


> 📖 **详细插件使用说明**：参考 [OceanBase Plugin Development Kit 用户手册](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/)

### 依赖寻找优先级

插件按以下优先级自动寻找Java依赖：

1. ** 环境变量**（最高优先级）
   ```bash
   export OCEANBASE_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path"
   ```

2. ** Observer 工作目录**（推荐）
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar  
   ${OB_WORKDIR}/java/ThaiSegmenter.class
   ```

3. ** 插件相对路径**（开发环境）
   ```
   ./java/lib/lucene-*.jar
   ```

** 建议**：使用方式2（复制java目录），无需配置OCEANBASE_PARSER_CLASSPATH，快速体验

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
(1, 'สวัสดีครับ', 'สวัสดีครับ ยินดีต้อนรับสู่เว็บไซต์ของเรา'),
(2, 'ขอบคุณครับ', 'ขอบคุณที่เข้ามาเยี่ยมชม'),
(3, 'สอบถาม', 'หากมีคำถามใดๆ กรุณาติดต่อสอบถาม'),
(4, 'ขอบคุณมากครับ', 'ขอบคุณมากครับที่ใช้บริการของเรา'),
(5, 'ยินดีต้อนรับ', 'ยินดีต้อนรับสู่ OceanBase'),
(6, 'สวัสดีครับ', 'สวัสดีครับ ดีใจที่ได้พบกันอีกครั้ง'),
(7, 'เป็นอย่างไรบ้าง', 'เป็นอย่างไรบ้างช่วงนี้'),
(8, 'ไม่มีปัญหา', 'ไม่มีปัญหาใดๆ เลย'),
(9, 'แบบฟอร์มกรอกข้อมูล', 'กรุณากรอกข้อมูลให้ครบถ้วน'),
(10, 'ขอบคุณมาก', 'ขอบคุณมาก หวังว่าจะได้พบกันอีก'),
(11, 'ฐานข้อมูล', 'ระบบจัดการฐานข้อมูล OceanBase'),
(12, 'การประมวลผลภาษา', 'การประมวลผลภาษาธรรมชาติภาษาไทย'),
(13, 'วิทยาการคอมพิวเตอร์', 'วิทยาการคอมพิวเตอร์และวิศวกรรมซอฟต์แวร์'),
(14, 'การเรียนรู้ของเครื่อง', 'การเรียนรู้ของเครื่องและปัญญาประดิษฐ์'),
(15, 'การพัฒนาซอฟต์แวร์', 'วิธีการพัฒนาซอฟต์แวร์');

-- 测试分词功能
SELECT TOKENIZE('ระบบจัดการฐานข้อมูล','thai_ftparser', '[{"output": "all"}]');

-- 测试 1：基础词汇搜索（预计返回 c1 = 1, 6）
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('สวัสดี' IN NATURAL LANGUAGE MODE);

-- 测试 2：感谢用语搜索（预计返回 c1 = 2, 4, 10）
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ขอบคุณ' IN NATURAL LANGUAGE MODE);

-- 测试 3：技术词汇搜索（预计返回 c1 = 11）
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ฐานข้อมูล' IN NATURAL LANGUAGE MODE);

-- 测试 4：复合技术词汇（预计返回 c1 = 12）
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ประมวลผล' IN NATURAL LANGUAGE MODE);

-- 测试 5：学科名称（预计返回 c1 = 13）
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('วิทยาการคอมพิวเตอร์' IN NATURAL LANGUAGE MODE);

-- 测试 6：人工智能相关（预计返回 c1 = 14）
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ปัญญาประดิษฐ์' IN NATURAL LANGUAGE MODE);

-- 测试 7：软件开发（预计返回 c1 = 15）
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('พัฒนาซอฟต์แวร์' IN NATURAL LANGUAGE MODE);

-- 测试 8：多词组合搜索
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('สวัสดี ยินดีต้อนรับ' IN NATURAL LANGUAGE MODE);

-- 测试 9：问候语搜索
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ยินดีต้อนรับ' IN NATURAL LANGUAGE MODE);

-- 测试 10：系统相关词汇
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ระบบ' IN NATURAL LANGUAGE MODE);
```

## 技术特性

### 泰语分词特点

泰语分词器基于 Apache Lucene ThaiAnalyzer：

```
输入: "การประมวลผลภาษาธรรมชาติ"
输出: ["ประมวล", "ภาษา", "ธรรมชาติ"]

特点:
- 智能分词: 自动识别泰语词汇边界
- 停用词过滤: 过滤常见的功能词
- UTF-8支持: 完整支持泰语字符
```

### 停用词处理

泰语分词器包含智能停用词过滤：

- **过滤对象**: 常见介词、连词、助词
- **保留内容**: 实质性词汇、专业术语
- **优化搜索**: 提高搜索相关性

### 与其他语言对比

| 语言 | 分词特点 | 停用词处理 |
|------|----------|-----------|
| **泰语** | 基础分词 + 停用词 | ✅ 智能过滤 |
| 日语 | BaseForm + 停用词 | ✅ 词干统一 |
| 韩语 | MIXED复合词 | ❌ 保留完整 |

**泰语分词器在简洁性和有效性之间取得最佳平衡**。
