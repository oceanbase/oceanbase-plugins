# OceanBase Thai Fulltext Parser Plugin

A Thai fulltext parser plugin for OceanBase. It uses JNI bridge to call Java tokenization libraries (integrated with Apache Lucene ThaiAnalyzer).

## Features

- ✅ Compatible with OceanBase FTParser interface (`thai_ftparser_main.cpp`)
- ✅ JNI integration for Java Thai tokenization (Lucene ThaiAnalyzer)
- ✅ UTF-8 multi-byte Thai text processing
- ✅ Intelligent stopword filtering
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
cd oceanbase-plugins/thai_ftparser
mkdir build
cd build
cmake ..
make
```
You will see the libthai_ftparser.so file in the build directory. This is the dynamic library plugin.

## Quick Start

### Deployment and Installation

**Recommended method**: Copy .class files and jar files to corresponding locations separately

```bash
# 1. Copy plugin dynamic library
cp /path/to/yourplugindirpath/libthai_ftparser.so /path/to/observer/plugin_dir/

# 2. Create java directory structure (if not exists)
mkdir -p /path/to/observer/java/lib

# 3. Copy Lucene dependency libraries
cp java/lib/lucene-core-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-common-8.11.2.jar /path/to/observer/java/lib/

# 4. Copy Thai segmenter class file
cp java/ThaiSegmenter.class /path/to/observer/java/

# 5. Install Java environment
yum install java-1.8.0-openjdk-devel -y

# 6. Start Observer and load plugin
# Connect to database
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # Example with dify database connection info

# Set plugin loading in sys tenant
ALTER SYSTEM SET plugins_load='libthai_ftparser.so:on';

# Restart Observer to take effect
killall observer
cd /path/to/observer
./bin/observer  # Start observer in the observer working directory

# Verify installation (see below)
```

**Multi-plugin Coexistence Notes**:
- If other language parser plugins are already installed, only copy the Thai segmenter .class file
- `lucene-core-8.11.2.jar` and `lucene-analyzers-common-8.11.2.jar` are shared by all plugins
- Thai segmenter uses ThaiAnalyzer, no additional language-specific jar packages needed
- When files already exist, cp command will ask for overwrite confirmation, you can choose to skip


> 📖 **Detailed Plugin Usage**: Refer to [OceanBase Plugin Development Kit User Manual](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/)

### Dependency Search Priority

The plugin automatically searches for Java dependencies in the following priority order:

1. **Environment Variable** (Highest Priority)
   ```bash
   export OCEANBASE_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path"
   ```

2. **Observer Working Directory** (Recommended)
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar  
   ${OB_WORKDIR}/java/ThaiSegmenter.class
   ```

3. **Plugin Relative Path** (Development Environment)
   ```
   ./java/lib/lucene-*.jar
   ```

**Recommendation**: Use method 2 (copy java directory), no need to configure OCEANBASE_PARSER_CLASSPATH for quick experience

### Installation Verification

```sql
-- Check if plugin is loaded successfully
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'thai_ftparser';

-- Create test table (ensure shell character encoding is UTF-8)
CREATE TABLE t_thai (
    c1 INT, 
    c2 VARCHAR(200), 
    c3 TEXT, 
    FULLTEXT INDEX (c2, c3) WITH PARSER thai_ftparser
);

-- Insert Thai test data
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

-- Test tokenization functionality
SELECT TOKENIZE('ระบบจัดการฐานข้อมูล','thai_ftparser', '[{"output": "all"}]');

-- Test 1: Basic vocabulary search (expected to return c1 = 1, 6)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('สวัสดี' IN NATURAL LANGUAGE MODE);

-- Test 2: Thank you expressions search (expected to return c1 = 2, 4, 10)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ขอบคุณ' IN NATURAL LANGUAGE MODE);

-- Test 3: Technical vocabulary search (expected to return c1 = 11)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ฐานข้อมูล' IN NATURAL LANGUAGE MODE);

-- Test 4: Compound technical terms (expected to return c1 = 12)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ประมวลผล' IN NATURAL LANGUAGE MODE);

-- Test 5: Academic discipline name (expected to return c1 = 13)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('วิทยาการคอมพิวเตอร์' IN NATURAL LANGUAGE MODE);

-- Test 6: Artificial intelligence related (expected to return c1 = 14)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ปัญญาประดิษฐ์' IN NATURAL LANGUAGE MODE);

-- Test 7: Software development (expected to return c1 = 15)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('พัฒนาซอฟต์แวร์' IN NATURAL LANGUAGE MODE);

-- Test 8: Multi-word combination search
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('สวัสดี ยินดีต้อนรับ' IN NATURAL LANGUAGE MODE);

-- Test 9: Greeting expressions search
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ยินดีต้อนรับ' IN NATURAL LANGUAGE MODE);

-- Test 10: System-related vocabulary
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ระบบ' IN NATURAL LANGUAGE MODE);
```

## Technical Features

### Thai Tokenization Characteristics

The Thai tokenizer is based on Apache Lucene ThaiAnalyzer:

```
Input: "การประมวลผลภาษาธรรมชาติ"
Output: ["ประมวล", "ภาษา", "ธรรมชาติ"]

Features:
- Intelligent segmentation: Automatically identifies Thai word boundaries
- Stopword filtering: Filters common functional words
- UTF-8 support: Complete support for Thai characters
```

### Stopword Processing

The Thai tokenizer includes intelligent stopword filtering:

- **Filtering targets**: Common prepositions, conjunctions, particles
- **Preserved content**: Substantive vocabulary, technical terms
- **Optimized search**: Improves search relevance
