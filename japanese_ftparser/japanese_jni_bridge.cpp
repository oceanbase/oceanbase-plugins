/**
 * Copyright (c) 2023 OceanBase
 * Japanese Fulltext Parser Plugin
 */

#include "japanese_jni_bridge.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <new>

namespace oceanbase {
namespace japanese_ftparser {

// Configuration implementation
JapaneseJNIBridgeConfig::JapaneseJNIBridgeConfig() 
    : segmenter_class_name("JapaneseSegmenter")
    , segment_method_name("segment") {
    // JVM configurations are now managed by JNIConfigUtils in common library
}

// JapaneseJNIBridge implementation
JapaneseJNIBridge::JapaneseJNIBridge() 
    : plugin_name_("japanese_ftparser")
    , is_initialized_(false)
    , segmenter_class_(nullptr)
    , segment_method_(nullptr) {
    clear_error();
}

JapaneseJNIBridge::~JapaneseJNIBridge() {
    if (is_initialized_) {
        // Unregister from global JVM manager
        oceanbase::jni::GlobalJVMManager::unregister_plugin(plugin_name_);
        is_initialized_ = false;
    }
}

int JapaneseJNIBridge::initialize() {
    std::lock_guard<std::mutex> lock(bridge_mutex_);
    
    if (is_initialized_) {
        return OBP_SUCCESS;
    }
    
    clear_error();
    
    // Register with global JVM manager
    oceanbase::jni::GlobalJVMManager::register_plugin(plugin_name_);
    
    // Create scoped JNI environment using unified configuration from common library
    oceanbase::jni::ScopedJNIEnvironment jni_env(plugin_name_);
    
    if (!jni_env) {
        set_error(OBP_PLUGIN_ERROR, "Failed to acquire JNI environment for initialization");
        oceanbase::jni::GlobalJVMManager::unregister_plugin(plugin_name_);
        return OBP_PLUGIN_ERROR;
    }
    
    // Load Java classes and cache method IDs
    int ret = load_java_classes(jni_env.get());
    if (ret != OBP_SUCCESS) {
        oceanbase::jni::GlobalJVMManager::unregister_plugin(plugin_name_);
        return ret;
    }
    
    is_initialized_ = true;
    OBP_LOG_INFO("Simplified JNI Bridge initialized successfully");
    return OBP_SUCCESS;
}

int JapaneseJNIBridge::segment(const std::string& text, std::vector<std::string>& tokens) {
    if (!is_initialized_) {
        set_error(OBP_PLUGIN_ERROR, "Bridge not initialized");
        return OBP_PLUGIN_ERROR;
    }
    
    clear_error();
    
    // Create scoped JNI environment for this operation
    oceanbase::jni::ScopedJNIEnvironment jni_env(plugin_name_);
    
    if (!jni_env) {
        set_error(OBP_PLUGIN_ERROR, "Failed to acquire JNI environment for segmentation");
        return OBP_PLUGIN_ERROR;
    }
    
    return do_segment(jni_env.get(), text, tokens);
}

int JapaneseJNIBridge::load_java_classes(JNIEnv* env) {
    if (!env) {
        set_error(OBP_PLUGIN_ERROR, "JNI environment is null");
        return OBP_PLUGIN_ERROR;
    }
    
    // Find segmenter class
    segmenter_class_ = env->FindClass(config_.segmenter_class_name.c_str());
    std::string error_msg;
    if (!segmenter_class_ || oceanbase::jni::JNIUtils::check_and_handle_exception(env, error_msg)) {
        std::string msg = "Cannot find Java class: " + config_.segmenter_class_name;
        if (!error_msg.empty()) {
            msg += " (" + error_msg + ")";
        }
        set_error(OBP_PLUGIN_ERROR, msg);
        return OBP_PLUGIN_ERROR;
    }
    
    // Make it a global reference to prevent GC
    segmenter_class_ = (jclass)env->NewGlobalRef(segmenter_class_);
    if (!segmenter_class_) {
        set_error(OBP_PLUGIN_ERROR, "Failed to create global reference for segmenter class");
        return OBP_PLUGIN_ERROR;
    }
    
    // OPTIMIZED: Get static segment method ID instead of constructor and instance method
    std::string segment_signature = "(Ljava/lang/String;)[Ljava/lang/String;";
    segment_method_ = env->GetStaticMethodID(segmenter_class_, 
                                           config_.segment_method_name.c_str(), 
                                           segment_signature.c_str());
    if (!segment_method_ || oceanbase::jni::JNIUtils::check_and_handle_exception(env, error_msg)) {
        std::string msg = "Cannot find STATIC method: " + config_.segment_method_name + 
                         " with signature: " + segment_signature;
        if (!error_msg.empty()) {
            msg += " (" + error_msg + ")";
        }
        set_error(OBP_PLUGIN_ERROR, msg);
        return OBP_PLUGIN_ERROR;
    }
    
    // OPTIMIZED: No constructor method needed for static approach
    
    OBP_LOG_INFO("Java classes loaded successfully");
    return OBP_SUCCESS;
}

int JapaneseJNIBridge::do_segment(JNIEnv* env, const std::string& text, std::vector<std::string>& tokens) {
    if (!env) {
        set_error(OBP_PLUGIN_ERROR, "JNI environment is null");
        return OBP_PLUGIN_ERROR;
    }
    
    // Convert C++ string to Java string
    jstring jtext = oceanbase::jni::JNIUtils::cpp_string_to_jstring(env, text);
    if (!jtext) {
        set_error(OBP_PLUGIN_ERROR, "Failed to convert text to Java string");
        return OBP_PLUGIN_ERROR;
    }
    
    // OPTIMIZED: Call static Java segmentation method - no instance creation!
    jobjectArray jresult = (jobjectArray)env->CallStaticObjectMethod(
        segmenter_class_, segment_method_, jtext);
    
    // Check for Java exceptions
    std::string error_msg;
    if (oceanbase::jni::JNIUtils::check_and_handle_exception(env, error_msg)) {
        std::string msg = "Static Java segmentation method threw exception";
        if (!error_msg.empty()) {
            msg += " (" + error_msg + ")";
        }
        set_error(OBP_PLUGIN_ERROR, msg);
        // Clean up local references before returning
        // Note: jresult is nullptr when exception occurs, no need to clean it
        env->DeleteLocalRef(jtext);
        return OBP_PLUGIN_ERROR;
    }
    
    if (!jresult) {
        set_error(OBP_PLUGIN_ERROR, "Static Java segmentation method returned null");
        env->DeleteLocalRef(jtext);
        return OBP_PLUGIN_ERROR;
    }
    
    // Convert Java string array to C++ vector (JNIUtils handles its own Frame management)
    int ret = oceanbase::jni::JNIUtils::jstring_array_to_cpp_vector(env, jresult, tokens);
    if (ret != 0) {
        set_error(OBP_PLUGIN_ERROR, "Failed to convert Java result to C++ vector");
        env->DeleteLocalRef(jtext);
        env->DeleteLocalRef(jresult);
        return OBP_PLUGIN_ERROR;
    }
    
    // Clean up our local references
    env->DeleteLocalRef(jtext);
    env->DeleteLocalRef(jresult);
    
    OBP_LOG_INFO("Segmentation completed, got %zu tokens", tokens.size());
    return OBP_SUCCESS;
}

void JapaneseJNIBridge::set_error(int code, const std::string& message) {
    last_error_.error_code = code;
    last_error_.error_message = message;
    // 移除直接输出，让上层统一处理错误输出
}

void JapaneseJNIBridge::clear_error() {
    last_error_.error_code = OBP_SUCCESS;
    last_error_.error_message.clear();
    last_error_.java_exception.clear();
}

// JapaneseJNIBridgeManager implementation
JapaneseJNIBridgeManager& JapaneseJNIBridgeManager::get_instance() {
    static JapaneseJNIBridgeManager instance;
    return instance;
}

std::shared_ptr<JapaneseJNIBridge> JapaneseJNIBridgeManager::get_bridge() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!bridge_) {
        bridge_ = std::make_shared<JapaneseJNIBridge>();
    }
    return bridge_;
}

int JapaneseJNIBridgeManager::initialize() {
    auto bridge = get_bridge();
    return bridge ? bridge->initialize() : OBP_PLUGIN_ERROR;
}

// Plugin parser structure
struct JapaneseParserState {
    std::vector<std::string> tokens;
    size_t current_token_index;
    
    JapaneseParserState() : current_token_index(0) {}
};

} // namespace japanese_ftparser
} // namespace oceanbase

// Plugin interface implementation
extern "C" {

int japanese_ftparser_init(ObPluginParamPtr param) {
    if (!param) {
        return OBP_INVALID_ARGUMENT;
    }
    
    // Don't initialize JVM here - do it lazily on first use (scan_begin)
    // This avoids issues with classpath when Observer is starting up
    OBP_LOG_INFO("Japanese FTParser plugin registered (JVM will be initialized on first use)");
    return OBP_SUCCESS;
}

int japanese_ftparser_deinit(ObPluginParamPtr param) {
    if (!param) {
        return OBP_INVALID_ARGUMENT;
    }
    
    OBP_LOG_INFO("Japanese FTParser plugin deinitialized");
    return OBP_SUCCESS;
}

int japanese_ftparser_scan_begin(ObPluginFTParserParamPtr param) {
    if (!param) {
        return OBP_INVALID_ARGUMENT;
    }
    
    // Lazy initialization: initialize JVM on first actual use
    auto& manager = oceanbase::japanese_ftparser::JapaneseJNIBridgeManager::get_instance();
    int     ret = manager.initialize();
    if (ret != OBP_SUCCESS) {
        OBP_LOG_WARN("Failed to initialize JNI bridge on first use (error_code: %d)", ret);
        return ret;
    }
    
    // Create parser instance
    oceanbase::japanese_ftparser::JapaneseParserState* jp = new (std::nothrow) oceanbase::japanese_ftparser::JapaneseParserState();
    if (!jp) {
        return OBP_ALLOCATE_MEMORY_FAILED;
    }
    
    // Get text to parse
    const char* doc = obp_ftparser_fulltext(param);
    int64_t length = obp_ftparser_fulltext_length(param);
    
    if (!doc || length <= 0) {
        delete jp;
        return OBP_INVALID_ARGUMENT;
    }
    
    std::string text(doc, length);
    
    // Segment text using simplified JNI bridge
    auto bridge = manager.get_bridge();
    if (!bridge) {
        delete jp;
        return OBP_PLUGIN_ERROR;
    }
    
    ret = bridge->segment(text, jp->tokens);
    if (ret != OBP_SUCCESS) {
        const auto& error = bridge->get_last_error();
        OBP_LOG_WARN("Segmentation failed: %s (error_code: %d)", 
                     error.error_message.c_str(), error.error_code);
        delete jp;
        return ret;
    }
    
    jp->current_token_index = 0;
    
    // Store parser instance in user data
    obp_ftparser_set_user_data(param, jp);
    
    OBP_LOG_INFO("Segmentation completed: %zu tokens extracted from %zu characters", 
                 jp->tokens.size(), text.length());
    return OBP_SUCCESS;
}

int japanese_ftparser_scan_end(ObPluginFTParserParamPtr param) {
    if (!param) {
        return OBP_INVALID_ARGUMENT;
    }
    
    oceanbase::japanese_ftparser::JapaneseParserState* jp = (oceanbase::japanese_ftparser::JapaneseParserState*)obp_ftparser_user_data(param);
    if (jp) {
        delete jp;
        obp_ftparser_set_user_data(param, nullptr);
    }
    
    return OBP_SUCCESS;
}

int japanese_ftparser_next_token(ObPluginFTParserParamPtr param, char **word, int64_t *word_len, int64_t *char_cnt, int64_t *word_freq) {
    if (!param || !word || !word_len || !char_cnt || !word_freq) {
        return OBP_INVALID_ARGUMENT;
    }
    
    oceanbase::japanese_ftparser::JapaneseParserState* jp = (oceanbase::japanese_ftparser::JapaneseParserState*)obp_ftparser_user_data(param);
    if (!jp) {
        return OBP_PLUGIN_ERROR;
    }
    
    if (jp->current_token_index >= jp->tokens.size()) {
        return OBP_ITER_END;
    }
    
    const std::string& token = jp->tokens[jp->current_token_index++];
    
    // Set word properties
    *word = const_cast<char*>(token.c_str());
    *word_len = token.length();
    
    // Calculate character count (UTF-8 character count, not byte count)
    // Use the same algorithm as the original plugin
    int64_t char_count = 0;
    const char* p = token.c_str();
    const char* end = p + token.length();
    
    while (p < end) {
        unsigned char c = *p;
        
        if ((c & 0x80) == 0) {
            p += 1;  // ASCII character
        } else if ((c & 0xE0) == 0xC0) {
            p += 2;  // 2-byte UTF-8 character
        } else if ((c & 0xF0) == 0xE0) {
            p += 3;  // 3-byte UTF-8 character (most CJK characters)
        } else if ((c & 0xF8) == 0xF0) {
            p += 4;  // 4-byte UTF-8 character
        } else {
            p += 1;  // Invalid UTF-8, skip
            continue;  // Don't count invalid characters
        }
        
        char_count++;
    }
    
    *char_cnt = char_count;  // Set character count
    *word_freq = 1;          // Set word frequency to 1
    
    return OBP_SUCCESS;
}

int japanese_ftparser_get_add_word_flag(uint64_t *flag) {
    if (!flag) {
        return OBP_INVALID_ARGUMENT;
    }
    
    // Set flags for Japanese language processing (same as original plugin)
    // Disable problematic filters that may reject Japanese characters
    *flag = OBP_FTPARSER_AWF_CASEDOWN      // Convert to lowercase (safe for Japanese)
            | OBP_FTPARSER_AWF_GROUPBY_WORD; // Group identical words (safe for Japanese)
    // Disabled: OBP_FTPARSER_AWF_MIN_MAX_WORD (may filter Japanese chars by length)
    // Disabled: OBP_FTPARSER_AWF_STOPWORD (may use inappropriate stopword list)
    
    return OBP_SUCCESS;
}

} // extern "C"
