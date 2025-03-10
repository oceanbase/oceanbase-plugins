# oceanbase-plugins
存放OceanBase的插件代码

# how to build

```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=`/path/to/oceanbase-plugin-dev-kit` -DCMAKE_INSTALL_PREFIX=./opt
make && make install
```
Then you can get plugin libraries from `build/opt/lib/`.

# how to pack
You can execute the commands below if you want to build RPM package.

```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=`/path/to/oceanbase-plugin-dev-kit` -DPACK_COMPONENTS=`component-name`
cpack -G RPM
```

`component-name` is the plugin name, such as `jieba-ftparser`.
