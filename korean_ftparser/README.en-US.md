# OceanBase Korean Fulltext Parser Plugin

A Korean fulltext parser plugin for OceanBase. It uses JNI bridge to call Java tokenization libraries (integrated with Apache Lucene KoreanAnalyzer/Nori).

## Features

- ✅ Compatible with OceanBase FTParser interface (`korean_ftparser_main.cpp`)
- ✅ JNI integration for Java Korean tokenization (Lucene KoreanAnalyzer/Nori)
- ✅ UTF-8 multi-byte Korean text processing
- ✅ MIXED compound word mode: preserves both complete words and decomposed words
- ✅ Extensible: Java tokenization implementation can be replaced

## Build

### Environment Setup

1. Install basic build tools
```bash
yum install -y git cmake make glibc-devel glibc-headers gcc gcc-c++
```
This command installs the gcc development environment.

> Skip this step if your environment already has these tools

2. Install OceanBase Plugin Development Kit
```bash
yum install -y oceanbase-plugin-dev-kit
```

### Compilation

```bash
# Choose your workspace directory
cd `your/workspace`
# Download source code
git clone https://github.com/oceanbase/oceanbase-plugins
# Build
cd oceanbase-plugins/korean_ftparser
mkdir build
cd build
cmake ..
make
```
You will see the libkorean_ftparser.so file in the build directory. This is the dynamic library plugin.

## Quick Start

### Deployment and Installation

**Recommended method**: Copy .class files and jar files to corresponding locations separately

```bash
# 1. Copy plugin dynamic library
cp /path/to/yourplugindirpath/libkorean_ftparser.so /path/to/observer/plugin_dir/

# 2. Create java directory structure (if not exists)
mkdir -p /path/to/observer/java/lib

# 3. Copy Lucene dependency libraries
cp java/lib/lucene-core-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-common-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-nori-8.11.2.jar /path/to/observer/java/lib/

# 4. Copy Korean segmenter class file
cp java/KoreanSegmenter.class /path/to/observer/java/

# 5. Install Java environment
yum install java-1.8.0-openjdk-devel -y

# 6. Start Observer and load plugin
# Connect to database
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # Example with dify database connection info

# Set plugin loading in sys tenant
ALTER SYSTEM SET plugins_load='libkorean_ftparser.so:on';

# Restart Observer to take effect
killall observer
cd /path/to/observer
./bin/observer  # Start observer in the observer working directory

# Verify installation (see below)
```

**Multi-plugin Coexistence Notes**:
- If other language parser plugins are already installed, only copy Korean-specific jar files and .class files
- `lucene-core-8.11.2.jar` and `lucene-analyzers-common-8.11.2.jar` are shared by all plugins
- `lucene-analyzers-nori-8.11.2.jar` is only needed by the Korean parser
- When files already exist, cp command will ask for overwrite confirmation, you can choose to skip


> 📖 **Detailed Plugin Usage**: Refer to [OceanBase Plugin Development Kit User Manual](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/)

### Dependency Search Priority

The plugin automatically searches for Java dependencies in the following priority order:

1. **Environment Variable** (Highest Priority)
   ```bash
   export OCEANBASE_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path/lucene-analyzers-nori-8.11.2.jar:/custom/path"
   ```

2. **Observer Working Directory** (Recommended)
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar  
   ${OB_WORKDIR}/java/lib/lucene-analyzers-nori-8.11.2.jar
   ${OB_WORKDIR}/java/KoreanSegmenter.class
   ```

3. **Plugin Relative Path** (Development Environment)
   ```
   ./java/lib/lucene-*.jar
   ```

**Recommendation**: Use method 2 (copy java directory), no need to configure OCEANBASE_PARSER_CLASSPATH for quick experience

### MIXED Mode Features

The Korean tokenizer uses **MIXED compound word mode** with the following advantages:

- **Precise Search**: Preserves complete compound words, e.g., `데이터베이스`
- **Flexible Search**: Also provides decomposed words, e.g., `데이터`, `베이스`
- **Optimal Balance**: Supports both exact matching and partial matching

### Installation Verification

```sql
-- Check if plugin is loaded successfully
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'korean_ftparser';

-- Create test table (ensure shell character encoding is UTF-8)
CREATE TABLE t_korean (
    c1 INT, 
    c2 VARCHAR(200), 
    c3 TEXT, 
    FULLTEXT INDEX (c2, c3) WITH PARSER korean_ftparser
);

-- Insert Korean test data
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

-- Test tokenization functionality
SELECT TOKENIZE('데이터베이스 관리 시스템','korean_ftparser', '[{"output": "all"}]');

-- Test 1: Compound word exact matching (expected to return c1 = 11)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('데이터베이스' IN NATURAL LANGUAGE MODE);

-- Test 2: Compound word partial matching (expected to return c1 = 11)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('데이터' IN NATURAL LANGUAGE MODE);

-- Test 3: Multi-word search (expected to return related results)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('안녕하세요 환영합니다' IN NATURAL LANGUAGE MODE);

-- Test 4: Technical term search (expected to return c1 = 12)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('자연언어처리' IN NATURAL LANGUAGE MODE);

-- Test 5: Compound technical vocabulary (expected to return c1 = 13)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('컴퓨터과학' IN NATURAL LANGUAGE MODE);

-- Test 6: Technology vocabulary search (expected to return c1 = 14)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('기계학습' IN NATURAL LANGUAGE MODE);

-- Test 7: Development-related vocabulary (expected to return c1 = 15)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('소프트웨어개발' IN NATURAL LANGUAGE MODE);

-- Test 8: Verify MIXED mode advantages
-- Searching "베이스" should match "데이터베이스"
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('베이스' IN NATURAL LANGUAGE MODE);
```

## Technical Features

### MIXED Compound Word Mode

The Korean tokenizer adopts Lucene Nori's MIXED mode:

```
Input: "데이터베이스"
Output: ["데이터베이스", "데이터", "베이스"]

Advantages:
- Precise search: "데이터베이스" → matches complete word
- Flexible search: "데이터" → matches part of compound word  
- Grammar integrity: preserves Korean grammatical structure
```

### Comparison with Other Modes

| Mode | Output Example | Search Characteristics |
|------|---------------|----------------------|
| **MIXED** | `[데이터베이스, 데이터, 베이스]` | Precise + Flexible |
| NONE | `[데이터베이스]` | Precise only |
| DISCARD | `[데이터, 베이스]` | Decomposed only |

**MIXED mode is most suitable for database fulltext search scenarios**.
