# OceanBase 日语全文解析插件（Japanese Fulltext Parser Plugin）

面向 OceanBase 的日语全文解析插件。核心使用 JNI 桥接 Java 分词库（已集成 Apache Lucene JapaneseAnalyzer/Kuromoji）。

## 🎯 功能特性

- ✅ 兼容 OceanBase FTParser 接口（`ftparser_main.cpp`）
- ✅ JNI 集成调用 Java 日语分词（Lucene JapaneseAnalyzer/Kuromoji）
- ✅ UTF-8 多字节日文处理
- ✅ 并发安全：单 JVM 复用 + 线程独立 `JNIEnv*`（`thread_local`）
- ✅ 局部引用安全：`PushLocalFrame/PopLocalFrame` 管理 JNI 局部引用
- ✅ 可扩展：可替换 Java 分词实现

## 📁 项目结构

```
japanese_ftparser
├── README.zh-CN.md
├── build/                          
│   └── libjapanese_ftparser.so      # 插件 .so
├── java/
│   ├── JapaneseSegmenter.java       # Kuromoji 驱动的 Java 日语分词器
│   ├── JapaneseSegmenter.class
│   └── lib/
│       ├── lucene-core-8.11.2.jar
│       ├── lucene-analyzers-common-8.11.2.jar
│       └── lucene-analyzers-kuromoji-8.11.2.jar
```

## 🚀 快速开始

### 一键部署（推荐方式）

**最简单的方法**：直接将整个 `java/` 文件夹复制到 Observer 工作目录

```bash
# 1. 复制插件动态库
cp /path/to/yourplugindirpath/libjapanese_ftparser.so /path/to/observer/plugin_dir/

# 2. 复制整个 java 目录（无需其他的目录配置）
cp -r java/ /path/to/observer/java/

# 3. 安装所需依赖
yum install java-1.8.0-openjdk-devel -y

# 4. 启动 Observer 并加载插件
# 连接数据库
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # 这里以dify的数据库连接信息为例

# 在sys租户中设置插件加载
ALTER SYSTEM SET plugins_load='libjapanese_ftparser.so:on';

# 重启Observer生效
killall observer
cd /path/to/observer
./bin/observer  # 在observer工作目录执行启动observer

# 验证安装（见下文）
```


> 📖 **详细插件使用说明**：参考 [OceanBase Plugin Development Kit 用户手册](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/)

### 依赖寻找优先级

插件按以下优先级自动寻找Java依赖：

1. **🥇 环境变量**（最高优先级）
   ```bash
   export JAPANESE_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path/lucene-analyzers-kuromoji-8.11.2.jar:/custom/path"
   ```

2. **🥈 Observer 工作目录**（推荐）
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar  
   ${OB_WORKDIR}/java/lib/lucene-analyzers-kuromoji-8.11.2.jar
   ${OB_WORKDIR}/java/JapaneseSegmenter.class
   ```

3. **🥉 插件相对路径**（开发环境）
   ```
   ../java/lib/lucene-*.jar
   ./java/lib/lucene-*.jar
   ../../java/lib/lucene-*.jar
   ```

**💡 建议**：使用方式2（复制java目录），无需配置JAPANESE_PARSER_CLASSPATH，快速体验

### 验证安装

```sql
-- 检查插件是否加载成功
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'japanese_ftparser';

-- 创建测试表（注意shell的字符集编码用UTF-8）
CREATE TABLE t_japanese (
    c1 INT, 
    c2 VARCHAR(200), 
    c3 TEXT, 
    FULLTEXT INDEX (c2, c3) WITH PARSER japanese_ftparser
);

-- 插入日语测试数据
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
(12, 'テスト繰り返し', 'パーサーが重複する単語を適切に処理するかテストします'),
(13, '何でもいい', 'あなたが必要とするものは何でも'),
(14, '誰も理解しない', '何が起こっているのか誰も理解していません'),
(15, '通常通り', 'すべて通常通りです'),
(16, '2025年は良い年', '2025年は開発にとって良い年です'),
(17, '2025年ありがとう', '2025年ありがとうございます'),
(18, 'OceanBaseデータベース', 'OceanBaseデータベースを選ぶ理由');
-- 测试全文搜索功能
SELECT TOKENIZE('ご質問がございましたら、営業日にお気軽にお問い合わせください','japanese_ftparser', '[{"output": "all"}]');
-- 测试 1：匹配单个词（预计返回 c1 = 3）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('気軽' IN NATURAL LANGUAGE MODE);

-- 测试 2：匹配多个词（预计返回 c1 = 1, 5, 6, 11）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('こんにちは ようこそ' IN NATURAL LANGUAGE MODE);

-- 测试 3：停用词测试（如果 "の" 是停用词，应该无结果）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('の' IN NATURAL LANGUAGE MODE);

-- 测试 4：数字 + 日语混合（预计返回 c1 = 16, 17）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('2025' IN NATURAL LANGUAGE MODE);

-- 测试 5：重复词语（预计返回 c1 = 12）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('テスト' IN NATURAL LANGUAGE MODE);

-- 测试 6：模糊匹配（预计返回 c1 = 2, 4, 10, 17）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('ありがとう' IN NATURAL LANGUAGE MODE);

-- 测试 7：验证关键日语句子搜索（预计返回 c1 = 18）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('理由' IN NATURAL LANGUAGE MODE);
```

## 🏗️ 架构

### 四层设计

1. 插件接口层
   - 实现 OceanBase FTParser 回调
   - 插件生命周期与注册
   - C 接口

2. 解析核心层
   - 解析会话与 token 缓存
   - UTF-8 日文处理
   - 集成 JNI 桥

3. JNI 桥接层
   - 单 JVM 复用（集中管理）
   - 每线程 `JNIEnv*` 自动获取与分离
   - `PushLocalFrame/PopLocalFrame` 管理局部引用
   - 异常捕获与错误码回传

4. Java 分词层
   - 具体日语分词实现（Kuromoji）

