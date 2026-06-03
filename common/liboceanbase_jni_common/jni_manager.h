/**
 * Copyright (c) 2023 OceanBase
 * OceanBase JNI Common Library - Global JVM and Thread Management
 */

#pragma once

#include <jni.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <thread>

namespace oceanbase {
namespace jni {

/**
 * JNI Configuration Utilities
 * @brief Provides unified configuration management for all plugins
 */
class JNIConfigUtils {
public:
    /**
     * Get unified Java classpath
     * @return Classpath string, checks OCEANBASE_JNI_CLASSPATH env var first
     */
    static std::string get_unified_classpath();
    
    /**
     * Get unified JVM maximum heap size
     * @return Max heap size in MB, checks OCEANBASE_JNI_MAX_HEAP env var first
     */
    static size_t get_unified_max_heap_mb();
    
    /**
     * Get unified JVM initial heap size
     * @return Initial heap size in MB, checks OCEANBASE_JNI_INIT_HEAP env var first
     */
    static size_t get_unified_init_heap_mb();

private:
    /**
     * Build dynamic classpath by scanning directory
     * @param base_dir Base directory (e.g., "./java")
     * @return Dynamically built classpath string
     */
    static std::string build_dynamic_classpath(const std::string& base_dir);
};

/**
 * Global JVM Manager
 * @brief Centralized JVM lifecycle management for all plugins with JNI environment.
 * @details This class manages the global JVM instance and ensures
 * proper creation, sharing, and cleanup across multiple plugin instances.
 */
class GlobalJVMManager {
public:
    /**
     * Get or create the global JVM instance
     * @param classpath Java classpath for JVM initialization
     * @param max_heap_mb Maximum heap size in MB
     * @param init_heap_mb Initial heap size in MB
     * @return Pointer to the global JVM instance, or nullptr on failure
     */
    static JavaVM* get_or_create_jvm(const std::string& classpath, 
                                    size_t max_heap_mb = 512, 
                                    size_t init_heap_mb = 128);
    
    /**
     * Register a plugin using the JVM
     * @param plugin_name Name of the plugin
     */
    static void register_plugin(const std::string& plugin_name);
    
    /**
     * Unregister a plugin and cleanup if it's the last one
     * @param plugin_name Name of the plugin
     */
    static void unregister_plugin(const std::string& plugin_name);
    
    /**
     * Get the current plugin count
     */
    static int get_plugin_count();
    
    /**
     * Force shutdown JVM (only for testing or emergency)
     */
    static void force_shutdown_jvm();
    
    /**
     * Get the global JVM instance (if exists)
     */
    static JavaVM* get_jvm();

private:
    static std::mutex global_mutex_;
    static JavaVM* shared_jvm_;
    static std::atomic<int> plugin_count_;
    static bool jvm_created_by_us_;
    static std::unordered_set<std::string> registered_plugins_;
    
    // Configuration consistency tracking
    static std::string first_instance_classpath_;
    static size_t first_instance_max_heap_mb_;
    static size_t first_instance_init_heap_mb_;
    static bool config_recorded_;
    
    // Helper method for configuration validation
    static bool validate_config_consistency(const std::string& classpath, 
                                           size_t max_heap_mb, 
                                           size_t init_heap_mb);
    
    // Disable construction
    GlobalJVMManager() = delete;
    ~GlobalJVMManager() = delete;
};

/**
 * Global Thread Manager  
 * @brief Manages JNI environment pointers for each thread globally
 * @details Ensures proper thread attachment/detachment coordination
 * across multiple plugins using reference counting.
 */
class GlobalThreadManager {
public:
    /**
     * Acquire JNI environment for current thread for a plugin
     * @param jvm The JVM instance to attach to
     * @param plugin_name Name of the plugin requesting the environment
     * @return JNI environment pointer for current thread, or nullptr on failure
     */
    static JNIEnv* acquire_jni_env_for_plugin(JavaVM* jvm, const std::string& plugin_name);
    
    /**
     * Release JNI environment for current thread for a plugin
     * @param jvm The JVM instance to detach from
     * @param plugin_name Name of the plugin releasing the environment
     */
    static void release_jni_env_for_plugin(JavaVM* jvm, const std::string& plugin_name);
    
    /**
     * Get the reference count for a specific thread
     * @param tid Thread ID to query
     * @return Reference count for the thread
     */
    static int get_thread_ref_count(std::thread::id tid);
    
    /**
     * Get the number of threads currently attached
     */
    static int get_attached_thread_count();

private:
    static std::mutex thread_mutex_;
    static std::unordered_map<std::thread::id, int> global_thread_ref_count_;
    static std::unordered_set<std::thread::id> attached_threads_;
    // TODO: thread_plugin_map_ is currently not utilized, kept for future debugging/monitoring needs
    // static std::unordered_map<std::thread::id, std::unordered_set<std::string>> thread_plugin_map_;
    
    // Disable construction
    GlobalThreadManager() = delete;
    ~GlobalThreadManager() = delete;
};

/**
 * RAII-style JNI Environment Management
 * @brief Automatic JNI environment acquisition and release
 * @details This class provides RAII-style management of JNI environments
 * with automatic cleanup and exception safety.
 */
class ScopedJNIEnvironment {
private:
    JNIEnv* env_;
    std::string plugin_name_;
    bool is_valid_;
    
public:
    /**
     * Constructor - automatically acquires JNI environment
     * @param plugin_name Name of the plugin
     * @param classpath Java classpath (optional, uses existing JVM if empty)
     * @param max_heap_mb Maximum heap size in MB
     * @param init_heap_mb Initial heap size in MB
     */
    ScopedJNIEnvironment(const std::string& plugin_name, 
                        const std::string& classpath = "",
                        size_t max_heap_mb = 512,
                        size_t init_heap_mb = 128);
    
    /**
     * Destructor - automatically releases JNI environment
     */
    ~ScopedJNIEnvironment();
    
    /**
     * Get the JNI environment pointer
     */
    JNIEnv* get() const { return env_; }
    
    /**
     * Check if the environment is valid
     */
    operator bool() const { return is_valid_; }
    
    /**
     * Check if the environment is valid
     */
    bool is_valid() const { return is_valid_; }
    
    // Disable copy and move
    ScopedJNIEnvironment(const ScopedJNIEnvironment&) = delete;
    ScopedJNIEnvironment& operator=(const ScopedJNIEnvironment&) = delete;
    ScopedJNIEnvironment(ScopedJNIEnvironment&&) = delete;
    ScopedJNIEnvironment& operator=(ScopedJNIEnvironment&&) = delete;
};

/**
 * JNI Utility Functions
 * @brief Common JNI utility functions
 */
class JNIUtils {
public:
    /**
     * Convert C++ string to Java string
     */
    static jstring cpp_string_to_jstring(JNIEnv* env, const std::string& str);
    
    /**
     * Convert Java string to C++ string
     */
    static std::string jstring_to_cpp_string(JNIEnv* env, jstring jstr);
    
    /**
     * Convert Java string array to C++ vector
     */
    static int jstring_array_to_cpp_vector(JNIEnv* env, jobjectArray jarray, 
                                          std::vector<std::string>& result);
    
    /**
     * Check and handle Java exceptions
     */
    static bool check_and_handle_exception(JNIEnv* env, std::string& error_message);
    
    /**
     * Get class name from jclass
     */
    static std::string get_class_name(JNIEnv* env, jclass clazz);
};

} // namespace jni
} // namespace oceanbase
