# OceanBase 日本語全文解析プラグイン（Japanese Fulltext Parser Plugin）

OceanBase 向けの日本語全文解析プラグイン。コアは JNI ブリッジを使用して Java 分詞ライブラリ（Apache Lucene JapaneseAnalyzer/Kuromoji を統合済み）を呼び出します。

## 🎯 機能特性

- ✅ OceanBase FTParser インターフェース互換（`ftparser_main.cpp`）
- ✅ JNI 統合で Java 日本語分詞呼び出し（Lucene JapaneseAnalyzer/Kuromoji）
- ✅ UTF-8 マルチバイト日本語処理
- ✅ 並行安全性：単一 JVM 再利用 + スレッド独立 `JNIEnv*`（`thread_local`）
- ✅ ローカル参照安全性：`PushLocalFrame/PopLocalFrame` で JNI ローカル参照管理
- ✅ 拡張性：Java 分詞実装を置き換え可能

## 📁 プロジェクト構造

```
japanese_ftparser
├── README.ja.md
├── build/
│   └── libjapanese_ftparser.so      # プラグイン .so
├── java/
│   ├── JapaneseSegmenter.java       # Kuromoji 駆動の Java 日本語分詞器
│   ├── JapaneseSegmenter.class
│   └── lib/
│       ├── lucene-core-8.11.2.jar
│       ├── lucene-analyzers-common-8.11.2.jar
│       └── lucene-analyzers-kuromoji-8.11.2.jar
```

## 🚀 クイックスタート

### ワンクリックデプロイ（推奨方法）

**最も簡単な方法**：`java/` フォルダ全体を Observer 作業ディレクトリにコピーする

```bash
# 1. プラグインダイナミックライブラリをコピー
cp /path/to/yourplugindirpath/libjapanese_ftparser.so /path/to/observer/plugin_dir/

# 2. java ディレクトリ全体をコピー（他のディレクトリ設定不要）
cp -r java/ /path/to/observer/java/

# 3. 必要な依存関係をインストール
yum install java-1.8.0-openjdk-devel -y

# 4. Observer を起動してプラグインをロード
# データベースに接続
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # ここではdifyのデータベース接続情報を例として使用

# sys テナントでプラグイン読み込みを設定
ALTER SYSTEM SET plugins_load='libjapanese_ftparser.so:on';

# Observer を再起動して有効化
killall observer
cd /path/to/observer
./bin/observer  # observer 作業ディレクトリで observer を起動

# インストール検証（下記参照）
```


> 📖 **詳細なプラグイン使用説明**：[OceanBase Plugin Development Kit ユーザーマニュアル](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/)

### 依存関係検索優先順位

プラグインは以下の優先順位で Java 依存関係を自動検索します：

1. **🥇 環境変数**（最高優先度）
   ```bash
   export JAPANESE_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path/lucene-analyzers-kuromoji-8.11.2.jar:/custom/path"
   ```

2. **🥈 Observer 作業ディレクトリ**（推奨）
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-kuromoji-8.11.2.jar
   ${OB_WORKDIR}/java/JapaneseSegmenter.class
   ```

3. **🥉 プラグイン相対パス**（開発環境）
   ```
   ../java/lib/lucene-*.jar
   ./java/lib/lucene-*.jar
   ../../java/lib/lucene-*.jar
   ```

**💡 推奨**：方法2（java ディレクトリコピー）を使用し、JAPANESE_PARSER_CLASSPATH 設定不要でクイック体験

### インストール検証

```sql
-- プラグインが正常に読み込まれたか確認
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'japanese_ftparser';

-- テストテーブル作成（shell の文字セットエンコーディングは UTF-8 を使用）
CREATE TABLE t_japanese (
    c1 INT,
    c2 VARCHAR(200),
    c3 TEXT,
    FULLTEXT INDEX (c2, c3) WITH PARSER japanese_ftparser
);

-- 日本語テストデータを挿入
INSERT INTO t_japanese (c1, c2, c3) VALUES
(1, 'こんにちは', 'こんにちは、私たちのウェブサイトへようこそ'),
(2, 'ありがとう', 'ご訪問いただきありがとうございます'),
(3, 'お問い合わせ', 'ご質問がございましたら、営業日にお気軽にお問い合わせください'),
(4, 'ありがとうございます', 'サービスをご利用いただきありがとうございます'),
(5, 'ようこそ', 'OceanBaseへようこそ'),
(6, 'こんにちは', 'こんにちは、またお会いできることを楽しみにしています'),
(7, 'いかがですか', '最近いかがお過ごしでしょうか'),
(8, 'お問い合わせ', 'ご質問がございましたら、営業日にお気軽にお問い合わせください'),
(9, '問題ありません', '何も問題ありません'),
(10, 'フォーム入力', '情報を完全に入力してください'),
(11, 'ありがとうございました', 'ありがとうございました、また将来お会いできることを願っています'),
(12, 'こんにちは', 'こんにちは、こんにちは'),
(13, 'テスト繰り返し', 'パーサーが重複する単語を適切に処理するかテストします'),
(14, '何でもいい', 'あなたが必要とするものは何でも'),
(15, '誰も理解しない', '何が起こっているのか誰も理解していません'),
(16, '通常通り', 'すべて通常通りです'),
(17, '2025年は良い年', '2025年は開発にとって良い年です'),
(18, '2025年ありがとう', '2025年ありがとうございます'),
(19, 'OceanBaseデータベース', 'OceanBaseデータベースを選ぶ理由');

-- 全文検索機能テスト
-- テスト 1：単一単語マッチング（c1 = 3, 8 を予想）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('気軽' IN NATURAL LANGUAGE MODE);

-- テスト 2：複数単語マッチング（c1 = 1, 5, 6, 12 を予想）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('こんにちは ようこそ' IN NATURAL LANGUAGE MODE);

-- テスト 3：ストップワードテスト（"の" がストップワードの場合、結果なし）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('の' IN NATURAL LANGUAGE MODE);

-- テスト 4：数字 + 日本語混合（c1 = 17, 18 を予想）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('2025' IN NATURAL LANGUAGE MODE);

-- テスト 5：重複単語（c1 = 13 を予想）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('テスト' IN NATURAL LANGUAGE MODE);

-- テスト 6：あいまいマッチング（c1 = 2, 4, 11, 18 を予想）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('ありがとう' IN NATURAL LANGUAGE MODE);

-- テスト 7：重要な日本語文検索検証（c1 = 19 を予想）
SELECT * FROM t_japanese
WHERE MATCH(c2, c3) AGAINST('理由' IN NATURAL LANGUAGE MODE);
```

## 🏗️ アーキテクチャ

### 4層設計

1. プラグインインターフェース層
   - OceanBase FTParser コールバック実装
   - プラグインライフサイクルと登録
   - C インターフェース

2. 解析コア層
   - 解析セッションとトークンキャッシュ
   - UTF-8 日本語処理
   - JNI ブリッジ統合

3. JNI ブリッジ層
   - 単一 JVM 再利用（集中管理）
   - スレッドごとの `JNIEnv*` 自動取得・分離
   - `PushLocalFrame/PopLocalFrame` でローカル参照管理
   - 例外キャッチとエラーコード返却

4. Java 分詞層
   - 具体的な日本語分詞実装（Kuromoji）
