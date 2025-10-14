# OceanBase Japanese Fulltext Parser Plugin

A Japanese fulltext parser plugin for OceanBase. It uses JNI bridge to call Java tokenization libraries (integrated with Apache Lucene JapaneseAnalyzer/Kuromoji).

## Features

- ✅ Compatible with OceanBase FTParser interface (`japanese_ftparser_main.cpp`)
- ✅ JNI integration for Java Japanese tokenization (Lucene JapaneseAnalyzer/Kuromoji)
- ✅ UTF-8 multi-byte Japanese text processing
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
cd oceanbase-plugins/japanese_ftparser
mkdir build
cd build
cmake ..
make
```
You will see the libjapanese_ftparser.so file in the build directory. This is the dynamic library plugin.

## Quick Start

### Deployment and Installation

**Recommended method**: Copy .class files and jar files to corresponding locations separately

```bash
# 1. Copy plugin dynamic library
cp /path/to/yourplugindirpath/libjapanese_ftparser.so /path/to/observer/plugin_dir/

# 2. Create java directory structure (if not exists)
mkdir -p /path/to/observer/java/lib

# 3. Copy Lucene dependency libraries
cp java/lib/lucene-core-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-common-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-kuromoji-8.11.2.jar /path/to/observer/java/lib/

# 4. Copy Japanese segmenter class file
cp java/JapaneseSegmenter.class /path/to/observer/java/

# 5. Install Java environment
yum install java-1.8.0-openjdk-devel -y

# 6. Start Observer and load plugin
# Connect to database
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # Example with dify database connection info

# Set plugin loading in sys tenant
ALTER SYSTEM SET plugins_load='libjapanese_ftparser.so:on';

# Restart Observer to take effect
killall observer
cd /path/to/observer
./bin/observer  # Start observer in the observer working directory

# Verify installation (see below)
```

**Multi-plugin Coexistence Notes**:
- If other language parser plugins are already installed, only copy Japanese-specific jar files and .class files
- `lucene-core-8.11.2.jar` and `lucene-analyzers-common-8.11.2.jar` are shared by all plugins
- `lucene-analyzers-kuromoji-8.11.2.jar` is only needed by the Japanese parser
- When files already exist, cp command will ask for overwrite confirmation, you can choose to skip


> 📖 **Detailed Plugin Usage**: Refer to [OceanBase Plugin Development Kit User Manual](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/)

### Dependency Search Priority

The plugin automatically searches for Java dependencies in the following priority order:

1. **Environment Variable** (Highest Priority)
   ```bash
   export OCEANBASE_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path/lucene-analyzers-kuromoji-8.11.2.jar:/custom/path"
   ```

2. **Observer Working Directory** (Recommended)
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar  
   ${OB_WORKDIR}/java/lib/lucene-analyzers-kuromoji-8.11.2.jar
   ${OB_WORKDIR}/java/JapaneseSegmenter.class
   ```

3. **Plugin Relative Path** (Development Environment)
   ```
   ./java/lib/lucene-*.jar
   ```

**Recommendation**: Use method 2 (copy java directory), no need to configure OCEANBASE_PARSER_CLASSPATH for quick experience

### Installation Verification

```sql
-- Check if plugin is loaded successfully
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'japanese_ftparser';

-- Create test table (ensure shell character encoding is UTF-8)
CREATE TABLE t_japanese (
    c1 INT, 
    c2 VARCHAR(200), 
    c3 TEXT, 
    FULLTEXT INDEX (c2, c3) WITH PARSER japanese_ftparser
);

-- Insert Japanese test data
INSERT INTO t_japanese (c1, c2, c3) VALUES
(1, 'こんにちは', 'こんにちは、私たちのウェブサイトへようこそ'),
(2, 'ありがとう', 'ご訪問いただきありがとうございます'),
(3, 'お問い合わせ', 'ご質問がございましたら、営業日にお気軽にお問い合わせください'),
(4, 'ありがとうございます', 'サービスをご利用いただきありがとうございます'),
(5, 'ようこそ', 'OceanBaseへようこそ'),
(6, 'こんにちは', 'こんにちは、またお会いできることを楽しみにしています'),
(7, 'いかがですか', '最近いかがお過ごしでしょうか'),
(8, '問題ありません', '何も問題ありません'),
(9, 'フォーム入力', '情報を完全に入力してください'),
(10, 'ありがとうございました', 'ありがとうございました、また将来お会いできることを願っています'),
(11, 'こんにちは', 'こんにちは、こんにちは'),
(12, '繰り返しテスト', 'パーサーが重複する単語を適切に処理するかテストします'),
(13, '何でもいい', 'あなたが必要とするものは何でも'),
(14, '誰も理解しない', '何が起こっているのか誰も理解していません'),
(15, '通常通り', 'すべて通常通りです'),
(16, '2025年は良い年', '2025年は開発にとって良い年です'),
(17, '2025年ありがとう', '2025年ありがとうございます'),
(18, 'OceanBaseデータベース', 'OceanBaseデータベースを選ぶ理由');

-- Test fulltext search functionality
SELECT TOKENIZE('ご質問がございましたら、営業日にお気軽にお問い合わせください','japanese_ftparser', '[{"output": "all"}]');

-- Test 1: Single word matching (expected to return c1 = 3)
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('気軽' IN NATURAL LANGUAGE MODE);

-- Test 2: Multiple word matching (expected to return c1 = 1, 5, 6, 11)
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('こんにちは ようこそ' IN NATURAL LANGUAGE MODE);

-- Test 3: Stopword test (should return no results if "の" is a stopword)
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('の' IN NATURAL LANGUAGE MODE);

-- Test 4: Number + Japanese mixed (expected to return c1 = 16, 17)
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('2025' IN NATURAL LANGUAGE MODE);

-- Test 5: Repeated words (expected to return c1 = 12)
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('テスト' IN NATURAL LANGUAGE MODE);

-- Test 6: Fuzzy matching (expected to return c1 = 2, 4, 10, 17)
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('ありがとう' IN NATURAL LANGUAGE MODE);

-- Test 7: Verify key Japanese sentence search (expected to return c1 = 18)
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('理由' IN NATURAL LANGUAGE MODE);
```