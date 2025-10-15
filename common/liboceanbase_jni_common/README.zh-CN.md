# OceanBase JNI 公共管理库

OceanBase 多语言分词插件的公共 JNI 管理库，提供统一的 JVM 管理和线程安全的 JNI 环境。

## 功能特性

- ✅ 单例 JVM 管理：多个插件共享同一个 JVM 实例
- ✅ 线程安全：自动处理 JNI 线程附加/分离
- ✅ 引用计数：智能管理 JNI 环境的生命周期
- ✅ RAII 模式：自动资源管理，防止内存泄漏

## 核心组件

### GlobalJVMManager
```cpp
// 全局单例 JVM 管理器
GlobalJVMManager& jvm_manager = GlobalJVMManager::getInstance();
JavaVM* jvm = jvm_manager.getJVM();
```

### GlobalThreadManager  
```cpp
// 线程安全的 JNI 环境管理
GlobalThreadManager& thread_manager = GlobalThreadManager::getInstance();
JNIEnv* env = thread_manager.attachCurrentThread();
thread_manager.detachCurrentThread();
```

### ScopedJNIEnvironment
```cpp
// RAII 自动管理 JNI 环境
{
    ScopedJNIEnvironment scoped_env;
    JNIEnv* env = scoped_env.getEnv();
    // 自动在作用域结束时清理
}
```

## 使用场景

### 多插件共存
- **日语分词插件** (`libjapanese_ftparser.so`)
- **韩语分词插件** (`libkorean_ftparser.so`)  
- **泰语分词插件** (`libthai_ftparser.so`)

所有插件共享同一个 JVM 实例，避免冲突。

### 线程安全
自动处理多线程环境下的 JNI 调用：
```cpp
// 线程 A
ScopedJNIEnvironment env_a;  // 自动附加
// 使用 env_a.getEnv()

// 线程 B  
ScopedJNIEnvironment env_b;  // 自动附加
// 使用 env_b.getEnv()
// 两个线程互不干扰
```

## 编译

```bash
cd common/liboceanbase_jni_common
mkdir build && cd build
cmake ..
make
```

生成 `liboceanbase_jni_common.so` 共享库。

## 部署

```bash
# 复制到 Observer 插件目录
cp liboceanbase_jni_common.so /path/to/observer/plugin_dir/

# 确保所有分词插件都能找到此库
export LD_LIBRARY_PATH=/path/to/observer/plugin_dir:$LD_LIBRARY_PATH
```

## 技术优势

### 解决的核心问题
1. **JVM 冲突**：多个插件试图创建 JVM 导致的冲突
2. **内存泄漏**：JNI 环境未正确释放
3. **线程安全**：多线程环境下的 JNI 调用问题
4. **资源管理**：复杂的 JNI 生命周期管理

### 设计模式
- **单例模式**：确保 JVM 全局唯一
- **RAII 模式**：自动资源管理
- **引用计数**：智能指针式的环境管理
- **线程局部存储**：每线程独立的 JNI 环境

## 依赖关系

```
liboceanbase_jni_common.so
├── libjapanese_ftparser.so
├── libkorean_ftparser.so  
└── libthai_ftparser.so
```

**为 OceanBase 多语言全文检索提供稳定可靠的 JNI 基础设施**
