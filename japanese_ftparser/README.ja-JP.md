# OceanBase 日本語全文解析プラグイン（Japanese Fulltext Parser Plugin）

OceanBase向けの日本語全文解析プラグインです。JNIブリッジを使用してJava分かち書きライブラリ（Apache Lucene JapaneseAnalyzer/Kuromoji統合済み）を呼び出します。

## 機能特性

- ✅ OceanBase FTParser インターフェース対応（`japanese_ftparser_main.cpp`）
- ✅ JNI統合によるJava日本語分かち書き（Lucene JapaneseAnalyzer/Kuromoji）
- ✅ UTF-8マルチバイト日本語処理
- ✅ 拡張可能：Java分かち書き実装の置き換え可能

## コンパイル

### 環境準備

1. 基本コンパイル環境のインストール
```bash
yum install -y git cmake make glibc-devel glibc-headers gcc gcc-c++
```
このコマンドでgcc開発環境がインストールされます。

> 環境が既に整っている場合はこの手順をスキップできます

2. OceanBase プラグイン開発キットのインストール
```bash
yum install -y oceanbase-plugin-dev-kit
```

### コンパイル

```bash
# 作業ディレクトリを選択
cd `your/workspace`
# ソースコードをダウンロード
git clone https://github.com/oceanbase/oceanbase-plugins
# コンパイル
cd oceanbase-plugins/japanese_ftparser
mkdir build
cd build
cmake ..
make
```
buildディレクトリに`libjapanese_ftparser.so`ファイルが作成されます。これが動的ライブラリプラグインです。

## クイックスタート

### デプロイとインストール

**推奨方法**：.classファイルとjarファイルをそれぞれ適切な場所にコピー

```bash
# 1. プラグイン動的ライブラリをコピー
cp /path/to/yourplugindirpath/libjapanese_ftparser.so /path/to/observer/plugin_dir/

# 2. javaディレクトリ構造を作成（存在しない場合）
mkdir -p /path/to/observer/java/lib

# 3. Lucene依存ライブラリをコピー
cp java/lib/lucene-core-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-common-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-kuromoji-8.11.2.jar /path/to/observer/java/lib/

# 4. 日本語分かち書きクラスファイルをコピー
cp java/JapaneseSegmenter.class /path/to/observer/java/

# 5. Java環境をインストール
yum install java-1.8.0-openjdk-devel -y

# 6. Observer を起動してプラグインを読み込み
# データベースに接続
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # difyのデータベース接続情報の例

# sys テナントでプラグイン読み込みを設定
ALTER SYSTEM SET plugins_load='libjapanese_ftparser.so:on';

# Observer を再起動して有効化
killall observer
cd /path/to/observer
./bin/observer  # observerワーキングディレクトリでobserverを起動

# インストール確認（下記参照）
```

**マルチプラグイン共存説明**：
- 他の言語分かち書きプラグインが既にインストールされている場合は、日本語専用のjarファイルと.classファイルのみをコピー
- `lucene-core-8.11.2.jar` と `lucene-analyzers-common-8.11.2.jar` は全プラグインで共有
- `lucene-analyzers-kuromoji-8.11.2.jar` は日本語分かち書きのみで必要
- ファイルが既に存在する場合、cpコマンドが上書きを確認するので、スキップを選択可能


> 📖 **詳細なプラグイン使用説明**：[OceanBase Plugin Development Kit ユーザーマニュアル](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/) を参照

### 依存関係の検索優先順位

プラグインは以下の優先順位でJava依存関係を自動検索します：

1. **環境変数**（最高優先度）
   ```bash
   export OCEANBASE_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path/lucene-analyzers-kuromoji-8.11.2.jar:/custom/path"
   ```

2. **Observer ワーキングディレクトリ**（推奨）
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar  
   ${OB_WORKDIR}/java/lib/lucene-analyzers-kuromoji-8.11.2.jar
   ${OB_WORKDIR}/java/JapaneseSegmenter.class
   ```

3. **プラグイン相対パス**（開発環境）
   ```
   ./java/lib/lucene-*.jar
   ```

**推奨**: 方式2（javaディレクトリのコピー）を使用し、`OCEANBASE_PARSER_CLASSPATH`の設定不要でクイック体験が可能

### インストール確認

```sql
-- プラグインが正常に読み込まれているかチェック
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'japanese_ftparser';

-- テストテーブル作成（シェルの文字セットエンコーディングはUTF-8を使用）
CREATE TABLE t_japanese (
    c1 INT, 
    c2 VARCHAR(200), 
    c3 TEXT, 
    FULLTEXT INDEX (c2, c3) WITH PARSER japanese_ftparser
);

-- 日本語テストデータの挿入
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

-- 全文検索機能のテスト
SELECT TOKENIZE('ご質問がございましたら、営業日にお気軽にお問い合わせください','japanese_ftparser', '[{"output": "all"}]');

-- テスト 1：単語マッチング（c1 = 3 が返される予定）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('気軽' IN NATURAL LANGUAGE MODE);

-- テスト 2：複数語マッチング（c1 = 1, 5, 6, 11 が返される予定）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('こんにちは ようこそ' IN NATURAL LANGUAGE MODE);

-- テスト 3：ストップワードテスト（「の」がストップワードの場合は結果なし）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('の' IN NATURAL LANGUAGE MODE);

-- テスト 4：数字 + 日本語混合（c1 = 16, 17 が返される予定）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('2025' IN NATURAL LANGUAGE MODE);

-- テスト 5：重複語句（c1 = 12 が返される予定）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('テスト' IN NATURAL LANGUAGE MODE);

-- テスト 6：あいまいマッチング（c1 = 2, 4, 10, 17 が返される予定）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('ありがとう' IN NATURAL LANGUAGE MODE);

-- テスト 7：重要な日本語文検索確認（c1 = 18 が返される予定）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('理由' IN NATURAL LANGUAGE MODE);
```

## 技術仕様

### ES完全ソリューション

日本語分かち書きはElasticsearch完全ソリューションを採用：

```
設定: CustomAnalyzer.builder()
  .withTokenizer("japanese")                    // kuromoji_tokenizer
  .addTokenFilter("japaneseBaseForm")           // kuromoji_baseform  
  .addTokenFilter("japanesePartOfSpeechStop")   // kuromoji_part_of_speech
  .addTokenFilter("cjkWidth")                   // cjk_width
  .addTokenFilter("lowercase")                  // lowercase
  .addTokenFilter("stop")                       // ja_stop

特徴:
- BaseForm語幹抽出: 動詞・形容詞の活用形を原形に統一
- ストップワード除去: 助詞・代名詞等の機能語を除去
- 文字幅正規化: 全角・半角の統一
- 小文字変換: アルファベットの統一
```

### Dify設定との整合

このプラグインはDifyのElasticsearch日本語設定と完全に整合：

```json
{
  "analysis": {
    "analyzer": {
      "ja_analyzer": {
        "type": "custom",
        "tokenizer": "kuromoji_tokenizer",
        "filter": [
          "kuromoji_baseform",      // ✅ 実装済み
          "kuromoji_part_of_speech", // ✅ 実装済み  
          "ja_stop",                // ✅ 実装済み
          "kuromoji_number",        // 将来拡張予定
          "kuromoji_stemmer"        // 将来拡張予定
        ]
      }
    }
  }
}
```

**データベース全文検索に最適化された日本語分かち書きソリューション**です。
