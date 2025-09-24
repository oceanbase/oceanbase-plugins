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

#include "thai_jni_bridge.h"
#include "oceanbase/ob_plugin_errno.h"
#include <sstream>
#include <cstdlib>

namespace oceanbase {
namespace thai {

// ThaiJNIBridge Implementation

// Define thread-local JNIEnv* storage
thread_local JNIEnv* ThaiJNIBridge::thread_env_ = nullptr;

ThaiJNIBridge::ThaiJNIBridge(const JNIBridgeConfig& config)
    : config_(config)
    , jvm_(nullptr)
    , is_initialized_(false)
    , jvm_created_by_us_(false)
    , segmenter_class_(nullptr)
    , constructor_method_(nullptr)
    , segment_method_(nullptr)
{
    OBP_LOG_TRACE("ThaiJNIBridge constructor called");
}

ThaiJNIBridge::~ThaiJNIBridge()
{
    cleanup();
    OBP_LOG_TRACE("ThaiJNIBridge destructor completed");
}

int ThaiJNIBridge::initialize()
{
    if (is_initialized_) {
        OBP_LOG_INFO("ThaiJNIBridge already initialized");
        return OBP_SUCCESS;
    }
    
    clear_error();
    
    OBP_LOG_INFO("Initializing Thai JNI Bridge...");
    OBP_LOG_INFO("Config: class_path=%s, class_name=%s, method=%s, heap_max=%zuMB, heap_init=%zuMB",
                 config_.java_class_path.c_str(),
                 config_.segmenter_class_name.c_str(),
                 config_.segment_method_name.c_str(),
                 config_.jvm_max_heap_mb,
                 config_.jvm_init_heap_mb);
    
    // Step 1: Create or attach to JVM
    int ret = create_or_attach_jvm();
    if (ret != OBP_SUCCESS) {
        set_error(ret, "Failed to create or attach to JVM");
        return ret;
    }
    
    // Step 2: Load Java classes and get method IDs
    ret = load_java_classes();
    if (ret != OBP_SUCCESS) {
        set_error(ret, "Failed to load Java classes");
        cleanup();
        return ret;
    }
    
    // No need to create global segmenter instance - we create fresh instances per call
    // This avoids state pollution between different parser instances
    
    is_initialized_ = true;
    OBP_LOG_INFO("Thai JNI Bridge initialized successfully");
    return OBP_SUCCESS;
}

int ThaiJNIBridge::segment(const std::string& text, std::vector<std::string>& tokens)
{
    if (!is_initialized_) {
        set_error(OBP_PLUGIN_ERROR, "JNI Bridge not initialized");
        return OBP_PLUGIN_ERROR;
    }
    
    // Ensure JNI environment is valid for this thread
    if (!thread_env_ || !jvm_) {
        OBP_LOG_WARN("JNI environment is null in segment, attempting to reattach");
        if (ensure_jni_attached() != OBP_SUCCESS) {
            return OBP_PLUGIN_ERROR;
        }
    }
    
    if (text.empty()) {
        tokens.clear();
        return OBP_SUCCESS;
    }
    
    clear_error();
    tokens.clear();
    
    // Push local reference frame to manage JNI local references automatically
    // Allocate space for: jtext, local_segmenter, jresult, plus array elements
    if (thread_env_->PushLocalFrame(64) < 0) {
        set_error(OBP_PLUGIN_ERROR, "Failed to push JNI local reference frame");
        return OBP_PLUGIN_ERROR;
    }
    
    OBP_LOG_TRACE("Segmenting text: length=%zu", text.length());
    
    // Convert C++ string to Java string
    jstring jtext = cpp_string_to_jstring(text);
    if (!jtext) {
        set_error(OBP_PLUGIN_ERROR, "Failed to convert text to Java string");
        thread_env_->PopLocalFrame(nullptr);  // Clean up local frame on error
        return OBP_PLUGIN_ERROR;
    }
    
    // Create a fresh Java segmenter instance for this call to avoid state pollution
    jobject local_segmenter = thread_env_->NewObject(segmenter_class_, constructor_method_);
    if (!local_segmenter || check_and_handle_exception()) {
        set_error(OBP_PLUGIN_ERROR, "Failed to create local segmenter instance");
        thread_env_->PopLocalFrame(nullptr);  // Clean up local frame on error
        return OBP_PLUGIN_ERROR;
    }
    
    // Call Java segmentation method on the fresh instance
    jobjectArray jresult = (jobjectArray)thread_env_->CallObjectMethod(
        local_segmenter, segment_method_, jtext);
    
    // Check for Java exceptions
    if (check_and_handle_exception()) {
        thread_env_->PopLocalFrame(nullptr);  // Clean up local frame on error
        return OBP_PLUGIN_ERROR;
    }
    
    if (!jresult) {
        set_error(OBP_PLUGIN_ERROR, "Java segmentation method returned null");
        thread_env_->PopLocalFrame(nullptr);  // Clean up local frame on error
        return OBP_PLUGIN_ERROR;
    }
    
    // Convert Java string array to C++ vector
    int ret = jstring_array_to_cpp_vector(jresult, tokens);
    
    // Pop local reference frame - this automatically cleans up all local references
    // created within this frame (jtext, local_segmenter, jresult, etc.)
    thread_env_->PopLocalFrame(nullptr);
    
    if (ret == OBP_SUCCESS) {
        OBP_LOG_TRACE("Segmentation completed: %zu tokens generated", tokens.size());
    }
    
    return ret;
}

void ThaiJNIBridge::cleanup()
{
    if (!is_initialized_) {
        return;
    }
    
    OBP_LOG_INFO("Cleaning up Thai JNI Bridge...");
    
    // Note: No need to cleanup Java instances - we use local instances that are auto-GC'd
    
    // Release Java references
    release_java_references();
    
    // Destroy JVM if we created it
    destroy_jvm();
    
    is_initialized_ = false;
    OBP_LOG_INFO("Thai JNI Bridge cleanup completed");
}

int ThaiJNIBridge::ensure_jni_attached()
{
    if (!jvm_) {
        set_error(OBP_PLUGIN_ERROR, "JVM is not initialized");
        return OBP_PLUGIN_ERROR;
    }
    
    // Use the thread state manager to get JNI environment
    thread_env_ = ThreadStateManager::get_jni_env_for_current_thread(jvm_);
    if (!thread_env_) {
        set_error(OBP_PLUGIN_ERROR, "Failed to get JNI environment for current thread");
        return OBP_PLUGIN_ERROR;
    }
    
    OBP_LOG_INFO("JNI environment ensured for current thread via ThreadStateManager");
    return OBP_SUCCESS;
}

int ThaiJNIBridge::create_or_attach_jvm()
{
    // Use the global JVM state manager
    jvm_ = JVMStateManager::get_or_create_jvm(
        config_.java_class_path,
        config_.jvm_max_heap_mb,
        config_.jvm_init_heap_mb
    );
    
    if (!jvm_) {
        set_error(OBP_PLUGIN_ERROR, "Failed to get or create JVM");
        return OBP_PLUGIN_ERROR;
    }
    
    // Register this instance with the state manager
    JVMStateManager::register_instance();
    
    // Get JNI environment for current thread
    thread_env_ = ThreadStateManager::get_jni_env_for_current_thread(jvm_);
    if (!thread_env_) {
        set_error(OBP_PLUGIN_ERROR, "Failed to get JNI environment for current thread");
        JVMStateManager::unregister_instance();
        return OBP_PLUGIN_ERROR;
    }
    
    // We don't directly create/destroy JVM anymore
    jvm_created_by_us_ = false;
    
    OBP_LOG_INFO("JVM attached successfully via state manager");
    return OBP_SUCCESS;
}

int ThaiJNIBridge::load_java_classes()
{
    // Find segmenter class
    segmenter_class_ = thread_env_->FindClass(config_.segmenter_class_name.c_str());
    if (!segmenter_class_ || check_and_handle_exception()) {
        std::string msg = "Cannot find Java class: " + config_.segmenter_class_name;
        set_error(OBP_PLUGIN_ERROR, msg);
        return OBP_PLUGIN_ERROR;
    }
    
    // Make it a global reference to prevent GC
    segmenter_class_ = (jclass)thread_env_->NewGlobalRef(segmenter_class_);
    if (!segmenter_class_) {
        set_error(OBP_PLUGIN_ERROR, "Failed to create global reference for segmenter class");
        return OBP_PLUGIN_ERROR;
    }
    
    // Get constructor method ID
    constructor_method_ = thread_env_->GetMethodID(segmenter_class_, "<init>", "()V");
    if (!constructor_method_ || check_and_handle_exception()) {
        set_error(OBP_PLUGIN_ERROR, "Cannot find constructor for segmenter class");
        return OBP_PLUGIN_ERROR;
    }
    
    // Get segment method ID
    std::string segment_signature = "(" + std::string("Ljava/lang/String;") + ")" + "[Ljava/lang/String;";
    segment_method_ = thread_env_->GetMethodID(segmenter_class_, 
                                       config_.segment_method_name.c_str(), 
                                       segment_signature.c_str());
    if (!segment_method_ || check_and_handle_exception()) {
        std::string msg = "Cannot find method: " + config_.segment_method_name + 
                         " with signature: " + segment_signature;
        set_error(OBP_PLUGIN_ERROR, msg);
        return OBP_PLUGIN_ERROR;
    }
    
    // Note: We no longer need cleanup method as we use local instances
    
    OBP_LOG_INFO("Java classes loaded successfully");
    return OBP_SUCCESS;
}

// Note: create_segmenter_instance() method removed - we now create local instances per call

jstring ThaiJNIBridge::cpp_string_to_jstring(const std::string& str)
{
    // Ensure JNI environment is valid
    if (!thread_env_ || !jvm_) {
        OBP_LOG_WARN("JNI environment is null, attempting to reattach");
        if (ensure_jni_attached() != OBP_SUCCESS) {
            return nullptr;
        }
    }
    
    jstring jstr = thread_env_->NewStringUTF(str.c_str());
    if (check_and_handle_exception()) {
        return nullptr;
    }
    
    return jstr;
}

std::string ThaiJNIBridge::jstring_to_cpp_string(jstring jstr)
{
    if (!thread_env_ || !jstr) {
        return std::string();
    }
    
    const char* chars = thread_env_->GetStringUTFChars(jstr, nullptr);
    if (!chars) {
        check_and_handle_exception();
        return std::string();
    }
    
    std::string result(chars);
    thread_env_->ReleaseStringUTFChars(jstr, chars);
    
    return result;
}

int ThaiJNIBridge::jstring_array_to_cpp_vector(jobjectArray jarray, std::vector<std::string>& result)
{
    if (!thread_env_ || !jarray) {
        return OBP_PLUGIN_ERROR;
    }
    
    jsize length = thread_env_->GetArrayLength(jarray);
    if (check_and_handle_exception()) {
        return OBP_PLUGIN_ERROR;
    }
    
    result.clear();
    result.reserve(length);
    
    // Process array in batches to prevent local reference accumulation
    const jsize BATCH_SIZE = 32;
    
    for (jsize start = 0; start < length; start += BATCH_SIZE) {
        // Push a local frame for this batch
        if (thread_env_->PushLocalFrame(BATCH_SIZE) < 0) {
            return OBP_PLUGIN_ERROR;
        }
        
        jsize end = (start + BATCH_SIZE < length) ? start + BATCH_SIZE : length;
        
        for (jsize i = start; i < end; i++) {
            jstring jstr = (jstring)thread_env_->GetObjectArrayElement(jarray, i);
            if (check_and_handle_exception()) {
                thread_env_->PopLocalFrame(nullptr);  // Clean up batch frame
                return OBP_PLUGIN_ERROR;
            }
            
            if (jstr) {
                std::string str = jstring_to_cpp_string(jstr);
                if (!str.empty()) {
                    result.push_back(str);
                }
                // jstr will be automatically cleaned up by PopLocalFrame
            }
        }
        
        // Pop the batch frame - cleans up all jstring references in this batch
        thread_env_->PopLocalFrame(nullptr);
    }
    
    return OBP_SUCCESS;
}

bool ThaiJNIBridge::check_and_handle_exception()
{
    if (!thread_env_ || !thread_env_->ExceptionCheck()) {
        return false;
    }
    
    // Get exception object
    jthrowable exception = thread_env_->ExceptionOccurred();
    thread_env_->ExceptionClear();
    
    if (exception) {
        // Get exception class and toString method
        jclass throwable_class = thread_env_->GetObjectClass(exception);
        jmethodID to_string_method = thread_env_->GetMethodID(throwable_class, "toString", "()Ljava/lang/String;");
        
        if (to_string_method) {
            jstring exception_string = (jstring)thread_env_->CallObjectMethod(exception, to_string_method);
            if (exception_string) {
                std::string exception_msg = jstring_to_cpp_string(exception_string);
                last_error_.java_exception = exception_msg;
                OBP_LOG_WARN("Java exception occurred: %s", exception_msg.c_str());
                thread_env_->DeleteLocalRef(exception_string);
            }
        }
        
        thread_env_->DeleteLocalRef(throwable_class);
        thread_env_->DeleteLocalRef(exception);
    }
    
    return true;
}

void ThaiJNIBridge::set_error(int code, const std::string& message)
{
    last_error_.error_code = code;
    last_error_.error_message = message;
    OBP_LOG_WARN("JNI Bridge error: code=%d, message=%s", code, message.c_str());
}

void ThaiJNIBridge::destroy_jvm()
{
    // Use the thread state manager for proper cleanup
    if (jvm_) {
        OBP_LOG_INFO("Cleaning up thread state via ThreadStateManager");
        ThreadStateManager::cleanup_current_thread(jvm_);
    }
    
    // Unregister this instance from the global state manager
    JVMStateManager::unregister_instance();
    
    // Clear our references
    thread_env_ = nullptr;
    jvm_ = nullptr;
    jvm_created_by_us_ = false;
}

void ThaiJNIBridge::release_java_references()
{
    if (thread_env_) {
        if (segmenter_class_) {
            thread_env_->DeleteGlobalRef(segmenter_class_);
            segmenter_class_ = nullptr;
        }
    }
    
    constructor_method_ = nullptr;
    segment_method_ = nullptr;
}

// JNIBridgeManager Implementation

JNIBridgeManager& JNIBridgeManager::instance()
{
    static JNIBridgeManager instance;
    return instance;
}

std::shared_ptr<ThaiJNIBridge> JNIBridgeManager::get_bridge(const JNIBridgeConfig& config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!bridge_) {
        bridge_ = std::make_shared<ThaiJNIBridge>(config);
    }
    return bridge_;
}

void JNIBridgeManager::release_bridge()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bridge_.reset();
}

// ============================================================================
// JVMStateManager Implementation
// ============================================================================

std::mutex JVMStateManager::global_mutex_;
JavaVM* JVMStateManager::global_jvm_ = nullptr;
std::atomic<int> JVMStateManager::instance_count_{0};
bool JVMStateManager::jvm_created_by_us_ = false;

// Configuration consistency tracking
std::string JVMStateManager::first_instance_classpath_;
size_t JVMStateManager::first_instance_max_heap_mb_ = 0;
size_t JVMStateManager::first_instance_init_heap_mb_ = 0;
bool JVMStateManager::config_recorded_ = false;

JavaVM* JVMStateManager::get_or_create_jvm(const std::string& classpath, 
                                          size_t max_heap_mb, 
                                          size_t init_heap_mb)
{
    std::lock_guard<std::mutex> lock(global_mutex_);
    
    // Validate configuration consistency (with warnings if mismatch)
    validate_config_consistency(classpath, max_heap_mb, init_heap_mb);
    
    // If JVM already exists, return it
    if (global_jvm_) {
        OBP_LOG_INFO("Using existing global JVM instance");
        return global_jvm_;
    }
    
    // Try to get existing JVM from the process
    jsize jvm_count = 0;
    jint result = JNI_GetCreatedJavaVMs(&global_jvm_, 1, &jvm_count);
    
    if (result == JNI_OK && jvm_count > 0) {
        // Found existing JVM
        OBP_LOG_INFO("Found existing JVM in process, reusing it");
        jvm_created_by_us_ = false;
        return global_jvm_;
    }
    
    // Create new JVM
    OBP_LOG_INFO("Creating new JVM with classpath: %s", classpath.c_str());
    
    JavaVMInitArgs vm_args;
    JavaVMOption options[5];
    
    // Build JVM options
    std::string classpath_option = "-Djava.class.path=" + classpath;
    std::string max_heap_option = "-Xmx" + std::to_string(max_heap_mb) + "m";
    std::string init_heap_option = "-Xms" + std::to_string(init_heap_mb) + "m";
    
    options[0].optionString = const_cast<char*>(classpath_option.c_str());
    options[1].optionString = const_cast<char*>(max_heap_option.c_str());
    options[2].optionString = const_cast<char*>(init_heap_option.c_str());
    options[3].optionString = const_cast<char*>("-XX:+UseG1GC");
    options[4].optionString = const_cast<char*>("-Dfile.encoding=UTF-8");
    
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 5;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = JNI_FALSE;
    
    JNIEnv* env = nullptr;
    result = JNI_CreateJavaVM(&global_jvm_, (void**)&env, &vm_args);
    
    if (result != JNI_OK) {
        OBP_LOG_WARN("Failed to create JVM, error code: %d", result);
        global_jvm_ = nullptr;
        return nullptr;
    }
    
    jvm_created_by_us_ = true;
    OBP_LOG_INFO("JVM created successfully");
    return global_jvm_;
}

void JVMStateManager::register_instance()
{
    int count = ++instance_count_;
    OBP_LOG_INFO("JVM instance registered, total count: %d", count);
}

void JVMStateManager::unregister_instance()
{
    int count = --instance_count_;
    OBP_LOG_INFO("JVM instance unregistered, remaining count: %d", count);
    
    if (count == 0) {
        std::lock_guard<std::mutex> lock(global_mutex_);
        OBP_LOG_INFO("Last instance unregistered, keeping JVM alive for stability");
        // Don't destroy JVM for stability - let it stay alive
        // This prevents crashes in multi-threaded environments
    }
}

int JVMStateManager::get_instance_count()
{
    return instance_count_.load();
}

void JVMStateManager::force_shutdown_jvm()
{
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (global_jvm_ && jvm_created_by_us_) {
        OBP_LOG_WARN("Force shutting down JVM");
        global_jvm_->DestroyJavaVM();
        global_jvm_ = nullptr;
        jvm_created_by_us_ = false;
    }
}

bool JVMStateManager::validate_config_consistency(const std::string& classpath, 
                                                 size_t max_heap_mb, 
                                                 size_t init_heap_mb)
{
    if (!config_recorded_) {
        // First instance - record the configuration
        first_instance_classpath_ = classpath;
        first_instance_max_heap_mb_ = max_heap_mb;
        first_instance_init_heap_mb_ = init_heap_mb;
        config_recorded_ = true;
        OBP_LOG_INFO("JVM configuration recorded: classpath=%s, max_heap=%zuMB, init_heap=%zuMB", 
                     classpath.c_str(), max_heap_mb, init_heap_mb);
        return true;
    }
    
    // Subsequent instances - validate consistency
    bool is_consistent = true;
    
    if (classpath != first_instance_classpath_) {
        OBP_LOG_WARN("JVM classpath mismatch detected:");
        OBP_LOG_WARN("  First instance: %s", first_instance_classpath_.c_str());
        OBP_LOG_WARN("  Current instance: %s", classpath.c_str());
        is_consistent = false;
    }
    
    if (max_heap_mb != first_instance_max_heap_mb_) {
        OBP_LOG_WARN("JVM max heap size mismatch detected:");
        OBP_LOG_WARN("  First instance: %zuMB", first_instance_max_heap_mb_);
        OBP_LOG_WARN("  Current instance: %zuMB", max_heap_mb);
        is_consistent = false;
    }
    
    if (init_heap_mb != first_instance_init_heap_mb_) {
        OBP_LOG_WARN("JVM initial heap size mismatch detected:");
        OBP_LOG_WARN("  First instance: %zuMB", first_instance_init_heap_mb_);
        OBP_LOG_WARN("  Current instance: %zuMB", init_heap_mb);
        is_consistent = false;
    }
    
    if (!is_consistent) {
        OBP_LOG_WARN("Configuration mismatch detected - using first instance configuration");
        OBP_LOG_WARN("This may cause unexpected behavior if different configurations are required");
    } else {
        OBP_LOG_INFO("JVM configuration consistency validated successfully");
    }
    
    return is_consistent;
}

// ============================================================================
// ThreadStateManager Implementation
// ============================================================================

std::mutex ThreadStateManager::thread_mutex_;
std::unordered_set<std::thread::id> ThreadStateManager::attached_threads_;

JNIEnv* ThreadStateManager::get_jni_env_for_current_thread(JavaVM* jvm)
{
    if (!jvm) {
        OBP_LOG_WARN("JVM is null, cannot get JNI environment");
        return nullptr;
    }
    
    std::thread::id current_thread_id = std::this_thread::get_id();
    
    // Check if current thread is already attached
    JNIEnv* env = nullptr;
    jint result = jvm->GetEnv((void**)&env, JNI_VERSION_1_8);
    
    if (result == JNI_OK && env) {
        // Already attached
        OBP_LOG_TRACE("Thread already attached to JVM");
        return env;
    } else if (result == JNI_EDETACHED) {
        // Need to attach current thread
        OBP_LOG_INFO("Attaching current thread to JVM");
        result = jvm->AttachCurrentThread((void**)&env, nullptr);
        
        if (result != JNI_OK) {
            OBP_LOG_WARN("Failed to attach current thread to JVM, error code: %d", result);
            return nullptr;
        }
        
        // Track attached thread
        {
            std::lock_guard<std::mutex> lock(thread_mutex_);
            attached_threads_.insert(current_thread_id);
        }
        
        OBP_LOG_INFO("Thread successfully attached to JVM");
        return env;
    } else {
        OBP_LOG_WARN("Unexpected JVM GetEnv result: %d", result);
        return nullptr;
    }
}

void ThreadStateManager::cleanup_current_thread(JavaVM* jvm)
{
    if (!jvm) {
        return;
    }
    
    std::thread::id current_thread_id = std::this_thread::get_id();
    
    // Check if this thread was attached by us
    bool was_attached = false;
    {
        std::lock_guard<std::mutex> lock(thread_mutex_);
        auto it = attached_threads_.find(current_thread_id);
        if (it != attached_threads_.end()) {
            attached_threads_.erase(it);
            was_attached = true;
        }
    }
    
    if (was_attached) {
        OBP_LOG_INFO("Detaching current thread from JVM");
        jvm->DetachCurrentThread();
    }
}

int ThreadStateManager::get_attached_thread_count()
{
    std::lock_guard<std::mutex> lock(thread_mutex_);
    return static_cast<int>(attached_threads_.size());
}

} // namespace thai
} // namespace oceanbase