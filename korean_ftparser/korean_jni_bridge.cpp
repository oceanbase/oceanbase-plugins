/**
 * Copyright (c) 2023 OceanBase
 * Korean Fulltext Parser Plugin
 */

#include "korean_jni_bridge.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <new>

using namespace oceanbase::jni;

namespace oceanbase {
namespace korean_ftparser {

// Configuration implementation
KoreanJNIBridgeConfig::KoreanJNIBridgeConfig() 
    : segmenter_class_name("KoreanSegmenter")
    , segment_method_name("segment") {
    // JVM configurations are now managed by JNIConfigUtils in common library
}

// KoreanJNIBridge implementation
KoreanJNIBridge::KoreanJNIBridge() 
    : plugin_name_("korean_ftparser")
    , is_initialized_(false)
    , segmenter_class_(nullptr)
    , constructor_method_(nullptr)
    , segment_method_(nullptr) {
    clear_error();
}

KoreanJNIBridge::~KoreanJNIBridge() {
    if (is_initialized_) {
        // Unregister from global JVM manager
        oceanbase::jni::GlobalJVMManager::unregister_plugin(plugin_name_);
        is_initialized_ = false;
    }
}

int KoreanJNIBridge::initialize() {
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
        set_error(OBP_PLUGIN_ERROR, "Failed to acquire JNI environment for Korean parser initialization");
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
    std::cout << "[INFO] Korean JNI Bridge initialized successfully" << std::endl;
    return OBP_SUCCESS;
}

int KoreanJNIBridge::segment(const std::string& text, std::vector<std::string>& tokens) {
    if (!is_initialized_) {
        set_error(OBP_PLUGIN_ERROR, "Korean JNI Bridge not initialized");
        return OBP_PLUGIN_ERROR;
    }
    
    // Create scoped JNI environment for segmentation
    oceanbase::jni::ScopedJNIEnvironment jni_env(plugin_name_);
    
    if (!jni_env) {
        set_error(OBP_PLUGIN_ERROR, "Failed to acquire JNI environment for Korean segmentation");
        return OBP_PLUGIN_ERROR;
    }
    
    return do_segment(jni_env.get(), text, tokens);
}

int KoreanJNIBridge::load_java_classes(JNIEnv* env) {
    std::string error_msg;
    
    // Load Korean segmenter class
    segmenter_class_ = env->FindClass(config_.segmenter_class_name.c_str());
    if (!segmenter_class_ || oceanbase::jni::JNIUtils::check_and_handle_exception(env, error_msg)) {
        set_error(OBP_PLUGIN_ERROR, "Failed to find Korean segmenter class '" + 
                  config_.segmenter_class_name + "': " + error_msg);
        return OBP_PLUGIN_ERROR;
    }
    
    // Make it a global reference to prevent GC
    segmenter_class_ = (jclass)env->NewGlobalRef(segmenter_class_);
    if (!segmenter_class_) {
        set_error(OBP_PLUGIN_ERROR, "Failed to create global reference for Korean segmenter class");
        return OBP_PLUGIN_ERROR;
    }
    
    // Get constructor method ID
    constructor_method_ = env->GetMethodID(segmenter_class_, "<init>", "()V");
    if (!constructor_method_ || oceanbase::jni::JNIUtils::check_and_handle_exception(env, error_msg)) {
        set_error(OBP_PLUGIN_ERROR, "Failed to find Korean segmenter constructor: " + error_msg);
        return OBP_PLUGIN_ERROR;
    }
    
    // Get segment method ID
    segment_method_ = env->GetMethodID(segmenter_class_, config_.segment_method_name.c_str(), 
                                      "(Ljava/lang/String;)[Ljava/lang/String;");
    if (!segment_method_ || oceanbase::jni::JNIUtils::check_and_handle_exception(env, error_msg)) {
        set_error(OBP_PLUGIN_ERROR, "Failed to find Korean segment method '" + 
                  config_.segment_method_name + "': " + error_msg);
        return OBP_PLUGIN_ERROR;
    }
    
    std::cout << "[INFO] Korean Java classes loaded successfully" << std::endl;
    return OBP_SUCCESS;
}

int KoreanJNIBridge::do_segment(JNIEnv* env, const std::string& text, std::vector<std::string>& tokens) {
    std::string error_msg;
    tokens.clear();
    
    // Push local frame for automatic local reference cleanup
    if (env->PushLocalFrame(64) < 0) {
        set_error(OBP_PLUGIN_ERROR, "Failed to push local frame for Korean segmentation");
        return OBP_PLUGIN_ERROR;
    }
    
    // Convert C++ string to Java string
    jstring jtext = oceanbase::jni::JNIUtils::cpp_string_to_jstring(env, text);
    if (!jtext) {
        env->PopLocalFrame(nullptr);
        set_error(OBP_PLUGIN_ERROR, "Failed to convert text to Java string for Korean segmentation");
        return OBP_PLUGIN_ERROR;
    }
    
    // Create Korean segmenter instance
    jobject local_segmenter = env->NewObject(segmenter_class_, constructor_method_);
    if (!local_segmenter || oceanbase::jni::JNIUtils::check_and_handle_exception(env, error_msg)) {
        env->PopLocalFrame(nullptr);
        set_error(OBP_PLUGIN_ERROR, "Failed to create Korean segmenter instance: " + error_msg);
        return OBP_PLUGIN_ERROR;
    }
    
    std::cout << "=== OCEANBASE JNI CALL === Segmenting Korean text with Lucene: \"" 
              << text.substr(0, std::min(text.length(), size_t(100))) << "\" (length: " << text.length() << ")" << std::endl;
    
    // Call segment method
    jobjectArray jresult = (jobjectArray)env->CallObjectMethod(local_segmenter, segment_method_, jtext);
    if (oceanbase::jni::JNIUtils::check_and_handle_exception(env, error_msg)) {
        env->PopLocalFrame(nullptr);
        set_error(OBP_PLUGIN_ERROR, "Korean segmentation failed: " + error_msg);
        return OBP_PLUGIN_ERROR;
    }
    
    if (!jresult) {
        env->PopLocalFrame(nullptr);
        set_error(OBP_PLUGIN_ERROR, "Korean segmentation returned null result");
        return OBP_PLUGIN_ERROR;
    }
    
    // Convert result to C++ vector
    int ret = oceanbase::jni::JNIUtils::jstring_array_to_cpp_vector(env, jresult, tokens);
    
    // Pop local frame to clean up local references
    env->PopLocalFrame(nullptr);
    
    if (ret != OBP_SUCCESS) {
        set_error(OBP_PLUGIN_ERROR, "Failed to convert Korean segmentation result to C++ vector");
        return ret;
    }
    
    std::cout << "=== OCEANBASE JNI RESULT === Korean segmentation result: [";
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << tokens[i];
    }
    std::cout << "]" << std::endl;
    
    return OBP_SUCCESS;
}

void KoreanJNIBridge::set_error(int code, const std::string& message) {
    last_error_code_ = code;
    last_error_message_ = message;
    std::cout << "[ERROR][KoreanJNIBridge] " << message << std::endl;
}

void KoreanJNIBridge::clear_error() {
    last_error_code_ = OBP_SUCCESS;
    last_error_message_.clear();
}

// KoreanJNIBridgeManager implementation
KoreanJNIBridgeManager& KoreanJNIBridgeManager::get_instance() {
    static KoreanJNIBridgeManager instance;
    return instance;
}

std::shared_ptr<KoreanJNIBridge> KoreanJNIBridgeManager::get_bridge() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!bridge_) {
        bridge_ = std::make_shared<KoreanJNIBridge>();
    }
    return bridge_;
}

int KoreanJNIBridgeManager::initialize() {
    auto bridge = get_bridge();
    return bridge ? bridge->initialize() : OBP_PLUGIN_ERROR;
}

// Plugin parser structure
struct KoreanParserState {
    std::vector<std::string> tokens;
    size_t current_token_index;
    
    KoreanParserState() : current_token_index(0) {}
};

} // namespace korean_ftparser
} // namespace oceanbase

// Plugin interface implementation
extern "C" {

int korean_ftparser_init(ObPluginParamPtr param) {
    if (!param) {
        return OBP_INVALID_ARGUMENT;
    }
    
    // Don't initialize JVM here - do it lazily on first use (scan_begin)
    // This avoids issues with classpath when Observer is starting up
    std::cout << "[INFO] Korean FTParser plugin registered (JVM will be initialized on first use)" << std::endl;
    return OBP_SUCCESS;
}

int korean_ftparser_deinit(ObPluginParamPtr param) {
    if (!param) {
        return OBP_INVALID_ARGUMENT;
    }
    
    std::cout << "[INFO] Korean FTParser deinitialized" << std::endl;
    return OBP_SUCCESS;
}

int korean_ftparser_scan_begin(ObPluginFTParserParamPtr param) {
    if (!param) {
        return OBP_INVALID_ARGUMENT;
    }
    
    // Lazy initialization: initialize JVM on first actual use
    auto& manager = oceanbase::korean_ftparser::KoreanJNIBridgeManager::get_instance();
    int ret = manager.initialize();
    if (ret != OBP_SUCCESS) {
        std::cout << "[ERROR] Failed to initialize Korean JNI bridge on first use" << std::endl;
        return ret;
    }
    
    // Create parser instance
    oceanbase::korean_ftparser::KoreanParserState* kp = new (std::nothrow) oceanbase::korean_ftparser::KoreanParserState();
    if (!kp) {
        return OBP_ALLOCATE_MEMORY_FAILED;
    }
    
    // Get text to parse
    const char* fulltext = obp_ftparser_fulltext(param);
    int64_t fulltext_len = obp_ftparser_fulltext_length(param);
    
    if (!fulltext || fulltext_len <= 0) {
        delete kp;
        return OBP_INVALID_ARGUMENT;
    }
    
    std::string text(fulltext, fulltext_len);
    
    // Perform segmentation
    auto bridge = manager.get_bridge();
    if (!bridge) {
        delete kp;
        return OBP_PLUGIN_ERROR;
    }
    
    ret = bridge->segment(text, kp->tokens);
    if (ret != OBP_SUCCESS) {
        std::cout << "[ERROR] Korean segmentation failed: " << bridge->get_last_error_message() << std::endl;
        delete kp;
        return ret;
    }
    
    std::cout << "[INFO][PLUGIN]do_segment (korean_jni_bridge.cpp:XXX) Segmentation completed, got " 
              << kp->tokens.size() << " tokens" << std::endl;
    
    // Store parser state
    obp_ftparser_set_user_data(param, kp);
    
    std::cout << "[INFO] Korean scan begin completed, got " << kp->tokens.size() << " tokens" << std::endl;
    return OBP_SUCCESS;
}

int korean_ftparser_scan_end(ObPluginFTParserParamPtr param) {
    if (!param) {
        return OBP_INVALID_ARGUMENT;
    }
    
    oceanbase::korean_ftparser::KoreanParserState* kp = (oceanbase::korean_ftparser::KoreanParserState*)obp_ftparser_user_data(param);
    if (kp) {
        delete kp;
        obp_ftparser_set_user_data(param, nullptr);
    }
    
    return OBP_SUCCESS;
}

int korean_ftparser_next_token(ObPluginFTParserParamPtr param, char **word, int64_t *word_len, int64_t *char_cnt, int64_t *word_freq) {
    if (!param || !word || !word_len || !char_cnt || !word_freq) {
        return OBP_INVALID_ARGUMENT;
    }
    
    oceanbase::korean_ftparser::KoreanParserState* kp = (oceanbase::korean_ftparser::KoreanParserState*)obp_ftparser_user_data(param);
    if (!kp) {
        return OBP_PLUGIN_ERROR;
    }
    
    if (kp->current_token_index >= kp->tokens.size()) {
        return OBP_ITER_END;
    }
    
    const std::string& token = kp->tokens[kp->current_token_index++];
    
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

int korean_ftparser_get_add_word_flag(uint64_t *flag) {
    if (!flag) {
        return OBP_INVALID_ARGUMENT;
    }
    
    // Set flags for Korean language processing (same as original plugin)
    // Disable problematic filters that may reject Korean characters
    *flag = OBP_FTPARSER_AWF_CASEDOWN      // Convert to lowercase (safe for Korean)
            | OBP_FTPARSER_AWF_GROUPBY_WORD; // Group identical words (safe for Korean)
    // Disabled: OBP_FTPARSER_AWF_MIN_MAX_WORD (may filter Korean chars by length)
    // Disabled: OBP_FTPARSER_AWF_STOPWORD (may use inappropriate stopword list)
    
    return OBP_SUCCESS;
}

} // extern "C"
