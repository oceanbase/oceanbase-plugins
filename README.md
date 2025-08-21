# oceanbase-plugins
存放OceanBase的插件代码

# 如何构建

```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=`/path/to/oceanbase-plugin-dev-kit` -DCMAKE_INSTALL_PREFIX=./opt
make && make install
```
编译完成后插件库位于 `build/opt/lib/` 下。

# 如何打包
可以执行下面的命令构建RPM包。

```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=`/path/to/oceanbase-plugin-dev-kit` -DPACK_COMPONENTS=`component-name`
cpack -G RPM
```

`component-name` 插件名称，比如 `jieba-ftparser`.
