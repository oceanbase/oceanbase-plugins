# OceanBase 한국어 전문 분석 플러그인 (Korean Fulltext Parser Plugin)

OceanBase를 위한 한국어 전문 분석 플러그인입니다. JNI 브리지를 사용하여 Java 형태소 분석 라이브러리 (Apache Lucene KoreanAnalyzer/Nori 통합)를 호출합니다.

## 기능

- ✅ OceanBase FTParser 인터페이스 호환 (`korean_ftparser_main.cpp`)
- ✅ JNI 통합을 통한 Java 한국어 형태소 분석 (Lucene KoreanAnalyzer/Nori)
- ✅ UTF-8 멀티바이트 한글 처리
- ✅ MIXED 복합어 모드: 완전한 단어와 분해된 단어를 동시에 보존
- ✅ 확장 가능: Java 형태소 분석 구현 교체 가능

## 컴파일

### 환경 준비

1. 기본 컴파일 도구 설치
```bash
yum install -y git cmake make glibc-devel glibc-headers gcc gcc-c++
```
이 명령어는 gcc 개발 환경을 설치합니다.

> 환경이 이미 준비되어 있다면 이 단계를 건너뛸 수 있습니다

2. OceanBase 플러그인 개발 키트 설치
```bash
yum install -y oceanbase-plugin-dev-kit
```

### 컴파일

```bash
# 작업 디렉토리 선택
cd `your/workspace`
# 소스 코드 다운로드
git clone https://github.com/oceanbase/oceanbase-plugins
# 컴파일
cd oceanbase-plugins/korean_ftparser
mkdir build
cd build
cmake ..
make
```
build 디렉토리에 libkorean_ftparser.so 파일이 생성됩니다. 이것이 동적 라이브러리 플러그인입니다.

## 빠른 시작

### 배포 및 설치

**권장 방법**: .class 파일과 jar 파일을 해당 위치에 개별적으로 복사

```bash
# 1. 플러그인 동적 라이브러리 복사
cp /path/to/yourplugindirpath/libkorean_ftparser.so /path/to/observer/plugin_dir/

# 2. java 디렉토리 구조 생성 (존재하지 않는 경우)
mkdir -p /path/to/observer/java/lib

# 3. Lucene 의존성 라이브러리 복사
cp java/lib/lucene-core-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-common-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-nori-8.11.2.jar /path/to/observer/java/lib/

# 4. 한국어 형태소 분석기 클래스 파일 복사
cp java/KoreanSegmenter.class /path/to/observer/java/

# 5. Java 환경 설치
yum install java-1.8.0-openjdk-devel -y

# 6. Observer 시작 및 플러그인 로드
# 데이터베이스 연결
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # dify 데이터베이스 연결 정보 예시

# sys 테넌트에서 플러그인 로딩 설정
ALTER SYSTEM SET plugins_load='libkorean_ftparser.so:on';

# Observer 재시작하여 적용
killall observer
cd /path/to/observer
./bin/observer  # observer 작업 디렉토리에서 observer 시작

# 설치 확인 (아래 참조)
```

**멀티 플러그인 공존 설명**:
- 다른 언어 형태소 분석 플러그인이 이미 설치되어 있다면, 한국어 전용 jar 파일과 .class 파일만 복사
- `lucene-core-8.11.2.jar`와 `lucene-analyzers-common-8.11.2.jar`는 모든 플러그인에서 공유
- `lucene-analyzers-nori-8.11.2.jar`는 한국어 형태소 분석기에서만 필요
- 파일이 이미 존재하는 경우 cp 명령어가 덮어쓰기를 확인하므로, 건너뛰기를 선택할 수 있습니다


> 📖 **자세한 플러그인 사용 설명**: [OceanBase Plugin Development Kit 사용자 매뉴얼](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/) 참조

### 의존성 검색 우선순위

플러그인은 다음 우선순위로 Java 의존성을 자동 검색합니다:

1. **환경 변수** (최고 우선순위)
   ```bash
   export OCEANBASE_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path/lucene-analyzers-nori-8.11.2.jar:/custom/path"
   ```

2. **Observer 작업 디렉토리** (권장)
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar  
   ${OB_WORKDIR}/java/lib/lucene-analyzers-nori-8.11.2.jar
   ${OB_WORKDIR}/java/KoreanSegmenter.class
   ```

3. **플러그인 상대 경로** (개발 환경)
   ```
   ./java/lib/lucene-*.jar
   ```

**권장사항**: 방식 2 (java 디렉토리 복사) 사용, OCEANBASE_PARSER_CLASSPATH 설정 불필요로 빠른 체험

### MIXED 모드 특성

한국어 형태소 분석기는 **MIXED 복합어 모드**를 사용하여 다음과 같은 장점을 제공합니다:

- **정확한 검색**: 완전한 복합어 보존, 예: `데이터베이스`
- **유연한 검색**: 분해된 단어도 동시 제공, 예: `데이터`, `베이스`
- **최적의 균형**: 정확한 매칭과 부분 매칭을 모두 지원

### 설치 확인

```sql
-- 플러그인이 성공적으로 로드되었는지 확인
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'korean_ftparser';

-- 테스트 테이블 생성 (셸의 문자셋 인코딩은 UTF-8 사용)
CREATE TABLE t_korean (
    c1 INT, 
    c2 VARCHAR(200), 
    c3 TEXT, 
    FULLTEXT INDEX (c2, c3) WITH PARSER korean_ftparser
);

-- 한국어 테스트 데이터 삽입
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

-- 형태소 분석 기능 테스트
SELECT TOKENIZE('데이터베이스 관리 시스템','korean_ftparser', '[{"output": "all"}]');

-- 테스트 1: 복합어 정확 매칭 (c1 = 11 반환 예상)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('데이터베이스' IN NATURAL LANGUAGE MODE);

-- 테스트 2: 복합어 부분 매칭 (c1 = 11 반환 예상)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('데이터' IN NATURAL LANGUAGE MODE);

-- 테스트 3: 다중 단어 검색 (관련 결과 반환 예상)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('안녕하세요 환영합니다' IN NATURAL LANGUAGE MODE);

-- 테스트 4: 전문 용어 검색 (c1 = 12 반환 예상)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('자연언어처리' IN NATURAL LANGUAGE MODE);

-- 테스트 5: 복합 전문 어휘 (c1 = 13 반환 예상)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('컴퓨터과학' IN NATURAL LANGUAGE MODE);

-- 테스트 6: 기술 어휘 검색 (c1 = 14 반환 예상)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('기계학습' IN NATURAL LANGUAGE MODE);

-- 테스트 7: 개발 관련 어휘 (c1 = 15 반환 예상)
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('소프트웨어개발' IN NATURAL LANGUAGE MODE);

-- 테스트 8: MIXED 모드 장점 검증
-- "베이스" 검색이 "데이터베이스"와 매칭되어야 함
SELECT * FROM t_korean
WHERE MATCH(c2, c3) AGAINST('베이스' IN NATURAL LANGUAGE MODE);
```

## 기술 특성

### MIXED 복합어 모드

한국어 형태소 분석기는 Lucene Nori의 MIXED 모드를 채택합니다:

```
입력: "데이터베이스"
출력: ["데이터베이스", "데이터", "베이스"]

장점:
- 정확한 검색: "데이터베이스" → 완전한 단어 매칭
- 유연한 검색: "데이터" → 복합어의 일부 매칭  
- 문법 완전성: 한국어 문법 구조 보존
```

### 다른 모드와의 비교

| 모드 | 출력 예시 | 검색 특성 |
|------|-----------|----------|
| **MIXED** | `[데이터베이스, 데이터, 베이스]` | 정확+유연 |
| NONE | `[데이터베이스]` | 정확만 |
| DISCARD | `[데이터, 베이스]` | 분해만 |

**MIXED 모드는 데이터베이스 전문 검색 시나리오에 가장 적합합니다**.
