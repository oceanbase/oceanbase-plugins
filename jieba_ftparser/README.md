# OceanBase 结巴分词插件
这是一个OceanBase数据库的结巴中文分词动态库插件。

## 如何使用
OceanBase支持动态库插件，可以将这个代码在你的目标平台上编译出动态链接库，然后安装到你的OceanBase集群中。

### 编译环境准备
> 建议找一个与你的OceanBase集群运行环境一致的机器。

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
cd oceanbase-plugins/jieba_ftparser
mkdir build
cd build
cmake ..
make
```
你将会在build目录下看到libjieba_ftparser.so文件，这个就是动态库插件。

### 安装
1. 复制动态库到`plugin_dir`目录
你需要将动态库复制到OceanBase集群中每个observer节点运行目录下的plugin_dir目录。
2. 修改集群参数
以sys租户root用户登录OceanBase数据库，修改配置项。
```sql
alter system set plugins_load='libjieba_ftparser.so';
```
3. 重启集群
OceanBase不支持动态加载动态链接库库插件，必须重启集群才会重新加载。

### 测试
使用任意用户登录OceanBase数据库。
```sql
MySQL [test]> SELECT TOKENIZE('OceanBase是一款自主研发的原生分布式数据库。','jieba');
+----------------------------------------------------------------------------------+
| TOKENIZE('OceanBase是一款自主研发的原生分布式数据库。','jieba')                  |
+----------------------------------------------------------------------------------+
| ["研发", "原生", "oceanbase", "分布式", "自主", "数据库", "一款"]                |
+----------------------------------------------------------------------------------+
1 row in set (0.00 sec)
```

### 说明
1. 当前仅支持UTF8编码；
2. 当前插件系统与分词插件接口属于实验室特性，未来可能会做变更，也欢迎对插件系统和分词提出需求；
3. 分词器的词典直接编译进了动态链接库，修改词典需要重新编译动态库，词典目录 cppjieba/dict；
4. 结巴分词仍然不完善，欢迎一起优化；
5. 动态库使用具备一些安全风险，使用前需要充分测试；
6. 插件代码中并没有捕获异常，如果出现异常，比如内存分配失败，将会导致整个进程异常退出。

### 致谢
当前代码使用了[yanyiwu/cppjieba结巴分词库](https://github.com/yanyiwu/cppjieba)，根据OceanBase插件的风格做了一些调整，比如日志打印、不抛异常等。
