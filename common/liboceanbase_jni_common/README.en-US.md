# OceanBase JNI Common Management Library

A common JNI management library for OceanBase multi-language tokenization plugins, providing unified JVM management and thread-safe JNI environment.

## Features

- ✅ Singleton JVM Management: Multiple plugins share the same JVM instance
- ✅ Thread Safety: Automatic JNI thread attach/detach handling
- ✅ Reference Counting: Smart JNI environment lifecycle management
- ✅ RAII Pattern: Automatic resource management, prevents memory leaks

## Core Components

### GlobalJVMManager
```cpp
// Global singleton JVM manager
GlobalJVMManager& jvm_manager = GlobalJVMManager::getInstance();
JavaVM* jvm = jvm_manager.getJVM();
```

### GlobalThreadManager  
```cpp
// Thread-safe JNI environment management
GlobalThreadManager& thread_manager = GlobalThreadManager::getInstance();
JNIEnv* env = thread_manager.attachCurrentThread();
thread_manager.detachCurrentThread();
```

### ScopedJNIEnvironment
```cpp
// RAII automatic JNI environment management
{
    ScopedJNIEnvironment scoped_env;
    JNIEnv* env = scoped_env.getEnv();
    // Automatically cleaned up when scope ends
}
```

## Use Cases

### Multi-Plugin Coexistence
- **Japanese Parser Plugin** (`libjapanese_ftparser.so`)
- **Korean Parser Plugin** (`libkorean_ftparser.so`)  
- **Thai Parser Plugin** (`libthai_ftparser.so`)

All plugins share the same JVM instance, avoiding conflicts.

### Thread Safety
Automatically handles JNI calls in multi-threaded environments:
```cpp
// Thread A
ScopedJNIEnvironment env_a;  // Auto attach
// Use env_a.getEnv()

// Thread B  
ScopedJNIEnvironment env_b;  // Auto attach
// Use env_b.getEnv()
// Threads don't interfere with each other
```

## Build

```bash
cd common/liboceanbase_jni_common
mkdir build && cd build
cmake ..
make
```

Generates `liboceanbase_jni_common.so` shared library.

## Deployment

```bash
# Copy to Observer plugin directory
cp liboceanbase_jni_common.so /path/to/observer/plugin_dir/

# Ensure all tokenization plugins can find this library
export LD_LIBRARY_PATH=/path/to/observer/plugin_dir:$LD_LIBRARY_PATH
```

## Technical Advantages

### Core Problems Solved
1. **JVM Conflicts**: Conflicts caused by multiple plugins trying to create JVM
2. **Memory Leaks**: Improper JNI environment release
3. **Thread Safety**: JNI call issues in multi-threaded environments
4. **Resource Management**: Complex JNI lifecycle management

### Design Patterns
- **Singleton Pattern**: Ensures globally unique JVM
- **RAII Pattern**: Automatic resource management
- **Reference Counting**: Smart pointer-style environment management
- **Thread Local Storage**: Per-thread independent JNI environment

## Dependencies

```
liboceanbase_jni_common.so
├── libjapanese_ftparser.so
├── libkorean_ftparser.so  
└── libthai_ftparser.so
```

**Providing stable and reliable JNI infrastructure for OceanBase multi-language fulltext search**
