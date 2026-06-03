# OceanBase 韩语全文解析插件（Korean Fulltext Parser Plugin）

面向 OceanBase 的韩语全文解析插件。核心使用 JNI 桥接 Java 分词库（已集成 Apache Lucene KoreanAnalyzer/Nori）。

## 功能特性

- ✅ 兼容 OceanBase FTParser 接口（`korean_ftparser_main.cpp`）
- ✅ JNI 集成调用 Java 韩语分词（Lucene KoreanAnalyzer/Nori）
- ✅ UTF-8 多字节韩文处理
- ✅ MIXED 复合词模式：同时保留完整词和分解词
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
cd oceanbase-plugins/korean_ftparser
mkdir build
cd build
cmake ..
make
```
你将会在build目录下看到libkorean_ftparser.so文件，这个就是动态库插件。

## 快速开始

### 部署安装

**推荐方法**：分别复制.class文件和jar文件到对应位置

```bash
# 1. 复制插件动态库
cp /path/to/yourplugindirpath/libkorean_ftparser.so /path/to/observer/plugin_dir/

# 2. 创建java目录结构（如果不存在）
mkdir -p /path/to/observer/java/lib

# 3. 复制Lucene依赖库
cp java/lib/lucene-core-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-common-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-nori-8.11.2.jar /path/to/observer/java/lib/

# 4. 复制韩语分词器类文件
cp java/KoreanSegmenter.class /path/to/observer/java/

# 5. 安装Java环境
yum install java-1.8.0-openjdk-devel -y

# 6. 启动 Observer 并加载插件
# 连接数据库
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # 这里以dify的数据库连接信息为例

# 在sys租户中设置插件加载
ALTER SYSTEM SET plugins_load='libkorean_ftparser.so:on';

# 重启Observer生效
killall observer
cd /path/to/observer
./bin/observer  # 在observer工作目录执行启动observer

# 验证安装（见下文）
```

**多插件共存说明**：
- 如果已安装其他语言分词插件，只需复制韩语专用的jar包和.class文件
- `lucene-core-8.11.2.jar` 和 `lucene-analyzers-common-8.11.2.jar` 被所有插件共享
- `lucene-analyzers-nori-8.11.2.jar` 仅韩语分词器需要
- 文件已存在时，cp命令会询问是否覆盖，可选择跳过


> 📖 **详细插件使用说明**：参考 [OceanBase Plugin Development Kit 用户手册](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/)

### 依赖寻找优先级

插件按以下优先级自动寻找Java依赖：

1. ** 环境变量**（最高优先级）
   ```bash
   export OCEANBASE_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path/lucene-analyzers-nori-8.11.2.jar:/custom/path"
   ```

2. ** Observer 工作目录**（推荐）
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar  
   ${OB_WORKDIR}/java/lib/lucene-analyzers-nori-8.11.2.jar
   ${OB_WORKDIR}/java/KoreanSegmenter.class
   ```

3. ** 插件相对路径**（开发环境）
   ```
   ./java/lib/lucene-*.jar
   ```

** 建议**：使用方式2（复制java目录），无需配置OCEANBASE_PARSER_CLASSPATH，快速体验

### MIXED 模式特性

韩语分词器使用 **MIXED 复合词模式**，具有以下优势：

- **精确搜索**：保留完整复合词，如 `데이터베이스`
- **灵活搜索**：同时提供分解词，如 `데이터`, `베이스`
- **最佳平衡**：既支持精确匹配，又支持部分匹配

### 验证安装

```sql
-- 检查插件是否加载成功
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'korean_ftparser';

-- 创建测试表（注意shell的字符集编码用UTF-8）
CREATE TABLE t_korean (
    c1 INT, 
    c2 VARCHAR(200), 
    c3 TEXT, 
    FULLTEXT INDEX (c2, c3) WITH PARSER korean_ftparser
);

-- 插入韩语测试数据
INSERT INTO t_korean (c1, c2, c3) VALUES
(1, '안녕하세요', '안녕하세요, 저희 웹사이트에 오신 것을 환영합니다'),
(2, '감사합니다', '방문해 주셔서 감사합니다'),
(3, '문의사항', '질문이 있으시면 언제든지 연락해 주세요'),
(4, '고맙습니다', '서비스를 이용해 주셔서 고맙습니다'),
(5, '환영합니다', 'OceanBase에 오신 것을 환영합니다'),
(6, '안녕하세요', '안녕하세요, 다시 만나뵙게 되어 기쁩니다'),
(7, '어떠세요', '요즘 어떻게 지내세요'),
(8, '문제없습니다', '아무 문제가 없습니다'),
(9, '입력양식', '정보를 완전히 입력해 주세요'),
(10, '감사했습니다', '감사했습니다, 앞으로도 만나뵐 수 있기를 바랍니다'),
(11, '데이터베이스', 'OceanBase 데이터베이스 관리 시스템'),
(12, '자연언어처리', '한국어 자연언어처리 기술'),
(13, '컴퓨터과학', '컴퓨터과학과 소프트웨어공학'),
(14, '기계학습', '기계학습과 인공지능의 발전'),
(15, '소프트웨어개발', '소프트웨어개발 방법론');

-- 测试分词功能
SELECT TOKENIZE('데이터베이스 관리 시스템','korean_ftparser', '[{"output": "all"}]');

-- 测试 1：复合词精确匹配（预计返回 c1 = 11）
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('데이터베이스' IN NATURAL LANGUAGE MODE);

-- 测试 2：复合词部分匹配（预计返回 c1 = 11）
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('데이터' IN NATURAL LANGUAGE MODE);

-- 测试 3：多词搜索（预计返回相关结果）
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('안녕하세요 환영합니다' IN NATURAL LANGUAGE MODE);

-- 测试 4：专业术语搜索（预计返回 c1 = 12）
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('자연언어처리' IN NATURAL LANGUAGE MODE);

-- 测试 5：复合专业词汇（预计返回 c1 = 13）
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('컴퓨터과학' IN NATURAL LANGUAGE MODE);

-- 测试 6：技术词汇搜索（预计返回 c1 = 14）
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('기계학습' IN NATURAL LANGUAGE MODE);

-- 测试 7：开发相关词汇（预计返回 c1 = 15）
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('소프트웨어개발' IN NATURAL LANGUAGE MODE);

-- 测试 8：验证MIXED模式优势
-- 搜索"베이스"应该能匹配到"데이터베이스"
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('베이스' IN NATURAL LANGUAGE MODE);
```

## 技术特性

### MIXED 复合词模式

韩语分词器采用 Lucene Nori 的 MIXED 模式：

```
输入: "데이터베이스"
输出: ["데이터베이스", "데이터", "베이스"]

优势:
- 精确搜索: "데이터베이스" → 匹配完整词
- 灵活搜索: "데이터" → 匹配复合词的一部分  
- 语法完整: 保留韩语语法结构
```

### 与其他模式对比

| 模式 | 输出示例 | 搜索特点 |
|------|----------|----------|
| **MIXED** | `[데이터베이스, 데이터, 베이스]` | 精确+灵活 |
| NONE | `[데이터베이스]` | 仅精确 |
| DISCARD | `[데이터, 베이스]` | 仅分解 |

**MIXED 模式最适合数据库全文检索场景**。
