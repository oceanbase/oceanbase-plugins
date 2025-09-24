# OceanBase ปลั๊กอินการวิเคราะห์ข้อความแบบเต็มของภาษาไทย (Thai Fulltext Parser Plugin)

ปลั๊กอินการวิเคราะห์ข้อความแบบเต็มของภาษาไทยสำหรับ OceanBase โดยใช้ JNI Bridge เชื่อมต่อกับไลบรารีการตัดคำภาษา Java (ได้รวม Apache Lucene ThaiAnalyzer แล้ว)

## 🎯 คุณสมบัติหลัก

- ✅ เข้ากันได้กับ OceanBase FTParser Interface (`thai_ftparser_main.cpp`)
- ✅ ผสาน JNI เรียกใช้การตัดคำภาษาไทยของ Java (Lucene ThaiAnalyzer)
- ✅ จัดการข้อความไทยแบบ UTF-8 หลายไบต์
- ✅ ความปลอดภัยในการทำงานพร้อมกัน: JVM เดียวใช้ซ้ำ + `JNIEnv*` แยกตามเธรด (`thread_local`)
- ✅ ความปลอดภัยของการอ้างอิงในเครื่อง: `PushLocalFrame/PopLocalFrame` จัดการ JNI local references
- ✅ ขยายได้: สามารถแทนที่การใช้งานการตัดคำ Java

## 🔧 คอมไพล์

### เตรียมสภาพแวดล้อม

1. ติดตั้งพื้นฐานการคอมไพล์
```bash
yum install -y git cmake make glibc-devel glibc-headers gcc gcc-c++
```
คำสั่งนี้จะติดตั้งสภาพแวดล้อมการพัฒนา gcc

> หากสภาพแวดล้อมของคุณพร้อมแล้ว สามารถข้ามขั้นตอนนี้ได้

2. ติดตั้งชุดพัฒนาปลั๊กอิน OceanBase
```bash
yum install -y oceanbase-plugin-dev-kit
```

### คอมไพล์

```bash
# เลือกไดเรกทอรีทำงานของคุณเอง
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
คุณจะเห็นไฟล์ libthai_ftparser.so ในไดเรกทอรี build นี่คือปลั๊กอินไลบรารีไดนามิก

## 🚀 เริ่มใช้งานอย่างรวดเร็ว

### การปรับใช้แบบคลิกเดียว (แนะนำ)

**วิธีที่ง่ายที่สุด**: คัดลอกโฟลเดอร์ `java/` ทั้งหมดไปยังไดเรกทอรีทำงาน Observer

```bash
# 1. คัดลอกไลบรารีไดนามิกของปลั๊กอิน
cp /path/to/yourplugindirpath/libthai_ftparser.so /path/to/observer/plugin_dir/

# 2. คัดลอกไดเรกทอรี java ทั้งหมด (ไม่ต้องกำหนดค่าไดเรกทอรีอื่น)
cp -r java/ /path/to/observer/java/

# 3. ติดตั้ง dependency ที่จำเป็น
yum install java-1.8.0-openjdk-devel -y
yum install libaio -y

# 4. เริ่ม Observer และโหลดปลั๊กอิน
# เชื่อมต่อฐานข้อมูล
obclient -h127.0.0.1 -P2881 -uroot@sys -pdifyai123456 # ใช้ข้อมูลการเชื่อมต่อฐานข้อมูล dify เป็นตัวอย่าง

# ตั้งค่าการโหลดปลั๊กอินใน tenant sys
ALTER SYSTEM SET plugins_load='libthai_ftparser.so:on';

# รีสตาร์ท Observer เพื่อให้มีผล
killall observer
cd /path/to/observer
./bin/observer  # เรียกใช้ในไดเรกทอรีทำงาน observer

# ตรวจสอบการติดตั้ง (ดูด้านล่าง)

# 5. การกำหนดค่าใน Dify
# หากต้องการใช้ตัวตัดคำภาษาไทยใน Dify ที่เริ่มด้วย Docker:
cd docker
bash setup-mysql-env.sh
# แก้ไข .env:
# OCEANBASE_ENABLE_HYBRID_SEARCH=true
# OCEANBASE_FULLTEXT_PARSER=thai_ftparser
docker compose up -d
# เข้าสู่คอนเทนเนอร์ oceanbase: docker exec -it <container_name> bash
# ไดเรกทอรีทำงาน observer ในคอนเทนเนอร์คือ /root/ob/observer
# ใช้ docker cp /path/to/thai-ftparser <container_name>:/path/to/thai-ftparser
# ปฏิบัติตามขั้นตอนที่ 1-4 เพื่อกำหนดค่าปลั๊กอิน
```


> 📖 **คำแนะนำการใช้งานปลั๊กอินโดยละเอียด**: อ้างอิง [คู่มือผู้ใช้ OceanBase Plugin Development Kit](https://oceanbase.github.io/oceanbase-plugin-dev-kit/user-guide/)

### ลำดับความสำคัญในการค้นหา dependency

ปลั๊กอินจะค้นหา Java dependency โดยอัตโนมัติตามลำดับความสำคัญต่อไปนี้:

1. **🥇 ตัวแปรสภาพแวดล้อม** (ความสำคัญสูงสุด)
   ```bash
   export THAI_PARSER_CLASSPATH="/custom/path/lucene-core-8.11.2.jar:/custom/path/lucene-analyzers-common-8.11.2.jar:/custom/path"
   ```

2. **🥈 ไดเรกทอรีทำงาน Observer** (แนะนำ)
   ```
   ${OB_WORKDIR}/java/lib/lucene-core-8.11.2.jar
   ${OB_WORKDIR}/java/lib/lucene-analyzers-common-8.11.2.jar
   ${OB_WORKDIR}/java/RealThaiSegmenter.class
   ```

3. **🥉 เส้นทางสัมพัทธ์ของปลั๊กอิน** (สภาพแวดล้อมการพัฒนา)
   ```
   ../java/lib/lucene-*.jar
   ./java/lib/lucene-*.jar
   ../../java/lib/lucene-*.jar
   ```

**💡 แนะนำ**: ใช้วิธีที่ 2 (คัดลอกไดเรกทอรี java) ไม่ต้องกำหนดค่า THAI_PARSER_CLASSPATH เพื่อทดลองใช้งานได้อย่างรวดเร็ว

### ตรวจสอบการติดตั้ง

```sql
-- ตรวจสอบว่าปลั๊กอินโหลดสำเร็จหรือไม่
SELECT * FROM oceanbase.GV$OB_PLUGINS WHERE NAME = 'thai_ftparser';

-- สร้างตารางทดสอบ (ให้ความสนใจกับการเข้ารหัสอักขระของ shell ใช้ UTF-8)
CREATE TABLE t_thai (
    c1 INT,
    c2 VARCHAR(200),
    c3 TEXT,
    FULLTEXT INDEX (c2, c3) WITH PARSER thai_ftparser
);

-- แทรกข้อมูลทดสอบภาษาไทย
INSERT INTO t_thai (c1, c2, c3) VALUES
(1, 'สวัสดีค่ะ', 'สวัสดีค่ะ ยินดีต้อนรับสู่เว็บไซต์ของเรา'),
(2, 'ขอบคุณครับ', 'ขอบคุณที่เข้ามาเยี่ยมชม'),
(3, 'สอบถามเพิ่มเติม', 'หากมีคำถามสามารถติดต่อเราได้ทุกวันทำการ'),
(4, 'ขอบคุณมาก', 'ขอบคุณที่ใช้บริการของเรา'),
(5, 'ยินดีต้อนรับ', 'ยินดีต้อนรับสู่ OceanBase'),
(6, 'สวัสดีครับ', 'สวัสดีครับ หวังว่าจะได้พบกันอีกเร็วๆ นี้'),
(7, 'เป็นอย่างไรบ้าง', 'เป็นอย่างไรบ้างในช่วงเวลาที่ผ่านมา'),
(8, 'สอบถามเพิ่มเติม', 'หากมีคำถามสามารถติดต่อเราได้ทุกวันทำการ'),
(9, 'ไม่มีปัญหา', 'ไม่มีปัญหาใดๆ เลย'),
(10, 'ได้โปรดกรอกแบบฟอร์ม', 'กรุณากรอกข้อมูลให้ครบถ้วน'),
(11, 'ขอบคุณและหวังว่าจะกลับมาใหม่', 'ขอบคุณและหวังว่าจะได้พบกันอีกในอนาคต'),
(12, 'สวัสดีค่ะสวัสดีครับ', 'สวัสดีค่ะ สวัสดีครับ'),
(13, 'ทดสอบซ้ำๆ', 'ทดสอบซ้ำๆ เพื่อดูว่า parser จัดการคำซ้ำได้ดีไหม'),
(14, 'เป็นอะไรก็ได้', 'เป็นอะไรก็ได้ที่คุณต้องการ'),
(15, 'ไม่มีใครเข้าใจ', 'ไม่มีใครเข้าใจสิ่งที่เกิดขึ้น'),
(16, 'เป็นไปตามปกติ', 'เป็นไปตามปกติทุกประการ'),
(17, '2025 ปีนี้เป็นปีที่ดี', 'ปี 2025 เป็นปีที่ดีสำหรับการพัฒนา'),
(18, 'ขอบคุณ 2025', 'ขอบคุณในปี 2025');

-- ทดสอบฟังก์ชันการค้นหาข้อความแบบเต็ม
-- ทดสอบ 1: จับคู่คำเดียว (คาดว่าจะคืนค่า c1 = 1, 6, 12)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('สวัสดี' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 2: จับคู่หลายคำ (คาดว่าจะคืนค่า c1 = 1, 5, 6, 12)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('สวัสดี ยินดี' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 3: ทดสอบคำหยุด (หาก "เป็น" เป็นคำหยุด ไม่ควรมีผลลัพธ์)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('เป็น' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 4: ตัวเลข + ภาษาไทยผสม (คาดว่าจะคืนค่า c1 = 17, 18)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('2025' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 5: คำซ้ำ (คาดว่าจะคืนค่า c1 = 13)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ทดสอบซ้ำๆ' IN NATURAL LANGUAGE MODE);

-- ทดสอบ 6: จับคู่แบบ fuzzy (คาดว่าจะคืนค่า c1 = 2, 4, 11, 18)
SELECT * FROM t_thai
WHERE MATCH(c2, c3) AGAINST('ขอบคุณ' IN NATURAL LANGUAGE MODE);
```

## 🏗️ สถาปัตยกรรม

### การออกแบบ 4 ชั้น

1. ชั้นอินเทอร์เฟซปลั๊กอิน
   - ใช้งาน OceanBase FTParser callbacks
   - วงจรชีวิตและการลงทะเบียนปลั๊กอิน
   - C Interface

2. ชั้นแกนการวิเคราะห์
   - เซสชันการวิเคราะห์และแคช token
   - การจัดการข้อความไทย UTF-8
   - ผสาน JNI Bridge

3. ชั้น JNI Bridge
   - JVM เดียวใช้ซ้ำ (จัดการแบบรวมศูนย์)
   - การเข้าถึงและแยก `JNIEnv*` อัตโนมัติต่อเธรด
   - จัดการ local references ด้วย `PushLocalFrame/PopLocalFrame`
   - จับข้อยกเว้นและส่งคืน error codes

4. ชั้นการตัดคำ Java
   - การใช้งานการตัดคำภาษาไทยเฉพาะ (ThaiAnalyzer)
