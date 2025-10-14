# ปลั๊กอินวิเคราะห์ข้อความภาษาไทย OceanBase (Thai Fulltext Parser Plugin)

ปลั๊กอินวิเคราะห์ข้อความภาษาไทยสำหรับ OceanBase ใช้ JNI bridge เพื่อเรียกใช้ไลบรารีการแยกคำภาษาไทย (รวม Apache Lucene ThaiAnalyzer)

## ฟีเจอร์

- ✅ รองรับ OceanBase FTParser interface (`thai_ftparser_main.cpp`)
- ✅ รวม JNI สำหรับการแยกคำภาษาไทยด้วย Java (Lucene ThaiAnalyzer)
- ✅ ประมวลผลข้อความภาษาไทย UTF-8 หลายไบต์
- ✅ กรองคำหยุดอัจฉริยะ
- ✅ ขยายได้: สามารถเปลี่ยนการใช้งานการแยกคำ Java

## การคอมไพล์

### การเตรียมสภาพแวดล้อม

1. ติดตั้งเครื่องมือคอมไพล์พื้นฐาน
```bash
yum install -y git cmake make glibc-devel glibc-headers gcc gcc-c++
```
คำสั่งนี้จะติดตั้งสภาพแวดล้อมการพัฒนา gcc

> หากสภาพแวดล้อมของคุณพร้อมแล้ว สามารถข้ามขั้นตอนนี้ได้

2. ติดตั้ง OceanBase Plugin Development Kit
```bash
yum install -y oceanbase-plugin-dev-kit
```

### การคอมไพล์

```bash
# เลือกไดเรกทอรีทำงานของคุณ
cd `your/workspace`
# ดาวน์โหลดซอร์สโค้ด
git clone https://github.com/oceanbase/oceanbase-plugins
# คอมไพล์
cd oceanbase-plugins/thai_ftparser
mkdir build
cd build
cmake ..
make
```
คุณจะเห็นไฟล์ libthai_ftparser.so ในไดเรกทอรี build นี่คือปลั๊กอินไลบรารีแบบไดนามิก

## เริ่มต้นใช้งานอย่างรวดเร็ว

### การติดตั้งและการใช้งาน

**วิธีที่แนะนำ**: คัดลอกไฟล์ .class และไฟล์ jar ไปยังตำแหน่งที่เกี่ยวข้องแยกกัน

```bash
# 1. คัดลอกไลบรารีแบบไดนามิกของปลั๊กอิน
cp /path/to/yourplugindirpath/libthai_ftparser.so /path/to/observer/plugin_dir/

# 2. สร้างโครงสร้างไดเรกทอรี java (หากไม่มี)
mkdir -p /path/to/observer/java/lib

# 3. คัดลอกไลบรารีการพึ่งพา Lucene
cp java/lib/lucene-core-8.11.2.jar /path/to/observer/java/lib/
cp java/lib/lucene-analyzers-common-8.11.2.jar /path/to/observer/java/lib/

# 4. คัดลอกไฟล์คลาสตัวแยกคำภาษาไทย
cp java/ThaiSegmenter.class /path/to/observer/java/

# 5. ติดตั้งสภาพแวดล้อม Java
yum install java-1.8.0-openjdk-devel -y

# 6. เริ่ม Observer และโหลดปลั๊กอิน
# เชื่อมต่อกับฐานข้อมูล
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # ตัวอย่างข้อมูลการเชื่อมต่อฐานข้อมูล dify

# ตั้งค่าการโหลดปลั๊กอินใน sys tenant
ALTER SYSTEM SET plugins_load='libthai_ftparser.so:on';

# รีสตาร์ท Observer เพื่อให้มีผล
killall observer
cd /path/to/observer
./bin/observer  # เริ่ม observer ในไดเรกทอรีทำงาน observer

# ตรวจสอบการติดตั้ง (ดูด้านล่าง)
```

**คำอธิบายการใช้งานร่วมกันหลายปลั๊กอิน**:
- หากติดตั้งปลั๊กอินตัวแยกคำภาษาอื่นแล้ว เพียงคัดลอกไฟล์ .class ของตัวแยกคำภาษาไทย
- `lucene-core-8.11.2.jar` และ `lucene-analyzers-common-8.11.2.jar` ใช้ร่วมกันโดยปลั๊กอินทั้งหมด
- ตัวแยกคำภาษาไทยใช้ ThaiAnalyzer ไม่ต้องการไฟล์ jar เฉพาะภาษาเพิ่มเติม
- เมื่อไฟล์มีอยู่แล้ว คำสั่ง cp จะถามเพื่อยืนยันการเขียนทับ คุณสามารถเลือกข้ามได้


> 📖 **คำอธิบายการใช้ปลั๊กอินโดยละเอียด**: อ้างอิง [OceanBase Plugin Development Kit คู่มือผู้ใช้](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/)

### ลำดับความสำคัญการค้นหาการพึ่งพา

ปลั๊กอินจะค้นหาการพึ่งพา Java โดยอัตโนมัติตามลำดับความสำคัญต่อไปนี้:

1. **ตัวแปรสภาพแวดล้อม** (ความสำคัญสูงสุด)
   ```bash
   export OCEANBASE_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path"
   ```

2. **ไดเรกทอรีทำงาน Observer** (แนะนำ)
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar  
   ${OB_WORKDIR}/java/ThaiSegmenter.class
   ```

3. **เส้นทางสัมพัทธ์ของปลั๊กอิน** (สภาพแวดล้อมการพัฒนา)
   ```
   ./java/lib/lucene-*.jar
   ```

**คำแนะนำ**: ใช้วิธีที่ 2 (คัดลอกไดเรกทอรี java) ไม่ต้องกำหนดค่า OCEANBASE_PARSER_CLASSPATH เพื่อประสบการณ์ที่รวดเร็ว

### การตรวจสอบการติดตั้ง

```sql
-- ตรวจสอบว่าปลั๊กอินโหลดสำเร็จหรือไม่
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'thai_ftparser';

-- สร้างตารางทดสอบ (ตรวจสอบให้แน่ใจว่าการเข้ารหัสอักขระของ shell เป็น UTF-8)
CREATE TABLE t_thai (
    c1 INT, 
    c2 VARCHAR(200), 
    c3 TEXT, 
    FULLTEXT INDEX (c2, c3) WITH PARSER thai_ftparser
);

-- แทรกข้อมูลทดสอบภาษาไทย
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

-- ทดสอบฟังก์ชันการแยกคำ
SELECT TOKENIZE('ระบบจัดการฐานข้อมูล','thai_ftparser', '[{"output": "all"}]');

-- ทดสอบ 1: การค้นหาคำศัพท์พื้นฐาน (คาดว่าจะส่งคืน c1 = 1, 6)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('สวัสดี' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 2: การค้นหาคำขอบคุณ (คาดว่าจะส่งคืน c1 = 2, 4, 10)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ขอบคุณ' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 3: การค้นหาคำศัพท์เทคนิค (คาดว่าจะส่งคืน c1 = 11)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ฐานข้อมูล' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 4: คำศัพท์เทคนิคผสม (คาดว่าจะส่งคืน c1 = 12)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ประมวลผล' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 5: ชื่อสาขาวิชา (คาดว่าจะส่งคืน c1 = 13)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('วิทยาการคอมพิวเตอร์' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 6: เกี่ยวกับปัญญาประดิษฐ์ (คาดว่าจะส่งคืน c1 = 14)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ปัญญาประดิษฐ์' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 7: การพัฒนาซอฟต์แวร์ (คาดว่าจะส่งคืน c1 = 15)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('พัฒนาซอฟต์แวร์' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 8: การค้นหาแบบผสมหลายคำ
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('สวัสดี ยินดีต้อนรับ' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 9: การค้นหาคำทักทาย
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ยินดีต้อนรับ' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 10: คำศัพท์เกี่ยวกับระบบ
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ระบบ' IN NATURAL LANGUAGE MODE);
```

## คุณสมบัติทางเทคนิค

### คุณสมบัติการแยกคำภาษาไทย

ตัวแยกคำภาษาไทยอิงตาม Apache Lucene ThaiAnalyzer:

```
อินพุต: "การประมวลผลภาษาธรรมชาติ"
เอาต์พุต: ["ประมวล", "ภาษา", "ธรรมชาติ"]

คุณสมบัติ:
- การแยกคำอัจฉริยะ: ระบุขอบเขตคำศัพท์ภาษาไทยโดยอัตโนมัติ
- กรองคำหยุด: กรองคำที่ใช้บ่อย
- รองรับ UTF-8: รองรับอักขระภาษาไทยอย่างสมบูรณ์
```

### การประมวลผลคำหยุด

ตัวแยกคำภาษาไทยรวมการกรองคำหยุดอัจฉริยะ:

- **วัตถุประสงค์การกรอง**: คำบุพบท คำสันธาน คำช่วยที่พบบ่อย
- **เนื้อหาที่เก็บไว้**: คำศัพท์สำคัญ คำศัพท์เฉพาะทาง
- **การค้นหาที่เหมาะสม**: เพิ่มความเกี่ยวข้องของการค้นหา
