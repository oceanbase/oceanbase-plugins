/*
 * Copyright (c) 2025 OceanBase.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <jni.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <atomic>
#include <thread>
#include <unordered_set>
#include "oceanbase/ob_plugin_ftparser.h"

/**
 * @defgroup ThaiJNIBridge Thai JNI Bridge
 * @brief JNI bridge for calling Java-based Thai segmentation libraries
 * @{
 */

namespace oceanbase {
namespace thai {

/**
 * JNI Bridge Configuration
 */
struct JNIBridgeConfig {
    std::string java_class_path;        // Java classpath
    std::string segmenter_class_name;   // Full Java class name
    std::string segment_method_name;    // Segmentation method name
    size_t jvm_max_heap_mb;            // JVM max heap size in MB
    size_t jvm_init_heap_mb;           // JVM initial heap size in MB
    
    JNIBridgeConfig() 
        : segmenter_class_name("RealThaiSegmenter")
        , segment_method_name("segment")
        , jvm_max_heap_mb(512)
        , jvm_init_heap_mb(128) {
        // Check environment variable first, then use default paths
        const char* env_classpath = std::getenv("THAI_PARSER_CLASSPATH");
        if (env_classpath && strlen(env_classpath) > 0) {
            java_class_path = env_classpath;
        } else {
            // Default fallback paths with explicit JAR files (JNI doesn't expand wildcards)
            java_class_path = "../java/lib/lucene-core-8.11.2.jar:"
                             "../java/lib/lucene-analyzers-common-8.11.2.jar:"
                             "../java:"
                             "./java/lib/lucene-core-8.11.2.jar:"
                             "./java/lib/lucene-analyzers-common-8.11.2.jar:"
                             "./java:"
                             "../../java/lib/lucene-core-8.11.2.jar:"
                             "../../java/lib/lucene-analyzers-common-8.11.2.jar:"
                             "../../java";
        }
    }
};

/**
 * JNI Error Information
 */
struct JNIErrorInfo {
    int error_code;
    std::string error_message;
    std::string java_exception;
    
    JNIErrorInfo() : error_code(0) {}
    
    void clear() {
        error_code = 0;
        error_message.clear();
        java_exception.clear();
    }
    
    bool has_error() const {
        return error_code != 0;
    }
};

/**
 * Thai JNI Bridge Class
 * @details This class manages the JNI bridge to Java-based Thai segmentation libraries.
 * It handles JVM lifecycle, Java class loading, method invocation, and data conversion
 * between C++ and Java.
 */
class ThaiJNIBridge {
public:
    /**
     * Constructor
     * @param config JNI bridge configuration
     */
    explicit ThaiJNIBridge(const JNIBridgeConfig& config = JNIBridgeConfig());
    
    /**
     * Destructor
     * @details Automatically cleans up JVM and releases resources
     */
    ~ThaiJNIBridge();
    
    /**
     * Initialize the JNI bridge
     * @return OBP_SUCCESS on success, error code on failure
     * @details This method creates the JVM, loads Java classes, and prepares
     * the segmentation environment.
     */
    int initialize();
    
    /**
     * Check if the bridge is initialized
     * @return true if initialized, false otherwise
     */
    bool is_initialized() const { return is_initialized_; }
    
    /**
     * Segment Thai text into tokens
     * @param text The Thai text to be segmented
     * @param tokens Output vector to store the segmented tokens
     * @return OBP_SUCCESS on success, error code on failure
     * @details This method calls the Java segmentation library via JNI
     * and converts the results back to C++ strings.
     */
    int segment(const std::string& text, std::vector<std::string>& tokens);
    
    /**
     * Get the last error information
     * @return Reference to the error information
     */
    const JNIErrorInfo& get_last_error() const { return last_error_; }
    
    /**
     * Clear the last error
     */
    void clear_error() { last_error_.clear(); }
    
    /**
     * Cleanup and release resources
     * @details This method destroys the JVM and releases all JNI resources.
     * It's safe to call multiple times.
     */
    void cleanup();

private:
    // Configuration
    JNIBridgeConfig config_;
    
    // JVM and JNI environment
    JavaVM* jvm_;
    // Thread-local JNI environment pointer to ensure per-thread safety
    static thread_local JNIEnv* thread_env_;
    bool is_initialized_;
    bool jvm_created_by_us_;
    
    // Java objects and method IDs
    jclass segmenter_class_;
    jmethodID constructor_method_;
    jmethodID segment_method_;
    // Note: We create local segmenter instances per call, no global instance needed
    
    // Error handling
    mutable JNIErrorInfo last_error_;
    
    /**
     * Create or attach to JVM
     * @return OBP_SUCCESS on success, error code on failure
     */
    int create_or_attach_jvm();
    
    /**
     * Ensure JNI environment is attached to current thread
     * @return OBP_SUCCESS on success, error code on failure
     */
    int ensure_jni_attached();
    
    /**
     * Load Java classes and get method IDs
     * @return OBP_SUCCESS on success, error code on failure
     */
    int load_java_classes();
    
    // Note: create_segmenter_instance() removed - we now create local instances per call
    
    /**
     * Convert C++ string to Java string
     * @param str C++ string
     * @return Java string (jstring), nullptr on failure
     */
    jstring cpp_string_to_jstring(const std::string& str);
    
    /**
     * Convert Java string to C++ string
     * @param jstr Java string
     * @return C++ string
     */
    std::string jstring_to_cpp_string(jstring jstr);
    
    /**
     * Convert Java string array to C++ string vector
     * @param jarray Java string array
     * @param result Output vector
     * @return OBP_SUCCESS on success, error code on failure
     */
    int jstring_array_to_cpp_vector(jobjectArray jarray, std::vector<std::string>& result);
    
    /**
     * Check and handle Java exceptions
     * @return true if exception occurred, false otherwise
     * @details This method checks for Java exceptions, logs them,
     * and stores the exception information in last_error_.
     */
    bool check_and_handle_exception();
    
    /**
     * Set error information
     * @param code Error code
     * @param message Error message
     */
    void set_error(int code, const std::string& message);
    
    /**
     * Destroy JVM if created by us
     */
    void destroy_jvm();
    
    /**
     * Release Java references
     */
    void release_java_references();
    
    // Disable copy constructor and assignment operator
    ThaiJNIBridge(const ThaiJNIBridge&) = delete;
    ThaiJNIBridge& operator=(const ThaiJNIBridge&) = delete;
};

/**
 * JNI Bridge Manager (Singleton)
 * @details This class manages a global JNI bridge instance to avoid
 * multiple JVM creation overhead.
 */
class JNIBridgeManager {
public:
    /**
     * Get the singleton instance
     * @return Reference to the JNI bridge manager
     */
    static JNIBridgeManager& instance();
    
    /**
     * Get or create JNI bridge
     * @param config Configuration for the bridge
     * @return Shared pointer to the JNI bridge
     */
    std::shared_ptr<ThaiJNIBridge> get_bridge(const JNIBridgeConfig& config = JNIBridgeConfig());
    
    /**
     * Release the bridge
     */
    void release_bridge();

private:
    std::shared_ptr<ThaiJNIBridge> bridge_;
    std::mutex mutex_;  // Thread safety for bridge access
    
    JNIBridgeManager() = default;
    ~JNIBridgeManager() = default;
    
    // Disable copy
    JNIBridgeManager(const JNIBridgeManager&) = delete;
    JNIBridgeManager& operator=(const JNIBridgeManager&) = delete;
};

/**
 * Global JVM State Manager
 * @brief Centralized JVM lifecycle management for thread safety
 * @details This class manages the global JVM instance and ensures
 * proper creation, sharing, and cleanup across multiple plugin instances.
 */
class JVMStateManager {
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
     * Register a new bridge instance using the JVM
     */
    static void register_instance();
    
    /**
     * Unregister a bridge instance and cleanup if it's the last one
     */
    static void unregister_instance();
    
    /**
     * Get the current instance count
     */
    static int get_instance_count();
    
    /**
     * Force shutdown JVM (only for testing or emergency)
     */
    static void force_shutdown_jvm();

private:
    static std::mutex global_mutex_;
    static JavaVM* global_jvm_;
    static std::atomic<int> instance_count_;
    static bool jvm_created_by_us_;
    
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
    JVMStateManager() = delete;
    ~JVMStateManager() = delete;
};

/**
 * Thread State Manager  
 * @brief Manages JNI environment pointers for each thread
 * @details Ensures each thread has its own JNI environment pointer
 * and handles thread attachment/detachment automatically.
 */
class ThreadStateManager {
public:
    /**
     * Get JNI environment for current thread
     * @param jvm The JVM instance to attach to
     * @return JNI environment pointer for current thread, or nullptr on failure
     */
    static JNIEnv* get_jni_env_for_current_thread(JavaVM* jvm);
    
    /**
     * Cleanup current thread's JNI environment
     * @param jvm The JVM instance to detach from
     */
    static void cleanup_current_thread(JavaVM* jvm);
    
    /**
     * Get the number of attached threads
     */
    static int get_attached_thread_count();

private:
    static std::mutex thread_mutex_;
    static std::unordered_set<std::thread::id> attached_threads_;
    
    // Disable construction
    ThreadStateManager() = delete;
    ~ThreadStateManager() = delete;
};

} // namespace thai
} // namespace oceanbase

/** @} */