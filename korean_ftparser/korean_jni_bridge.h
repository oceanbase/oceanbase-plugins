/**
 * Copyright (c) 2023 OceanBase
 * Korean Fulltext Parser Plugin
 */

#pragma once

#include "oceanbase/ob_plugin_ftparser.h"
#include "jni_manager.h"  // 统一JNI管理库
#include <string>
#include <vector>
#include <mutex>

namespace oceanbase {
namespace korean_ftparser {

/**
 * Korean JNI Bridge Configuration
 * @brief Plugin-specific configuration for Korean text segmentation
 * @details JVM-level configurations are now managed by JNIConfigUtils in common library
 */
struct KoreanJNIBridgeConfig {
    // Plugin-specific configurations only
    std::string segmenter_class_name;
    std::string segment_method_name;
    
    KoreanJNIBridgeConfig();
};

/**
 * Korean JNI Bridge for Korean Segmentation
 * @brief Korean parser using the common JNI library
 * @details This class uses oceanbase::jni::ScopedJNIEnvironment for
 * automatic JVM and thread management, eliminating the need for
 * complex manual resource management.
 */
class KoreanJNIBridge {
private:
    KoreanJNIBridgeConfig config_;
    std::string plugin_name_;
    bool is_initialized_;
    std::mutex bridge_mutex_;
    
    // Java class and method references (cached for performance)
    jclass segmenter_class_;
    jmethodID constructor_method_;
    jmethodID segment_method_;
    
    // Error handling
    int last_error_code_;
    std::string last_error_message_;

public:
    KoreanJNIBridge();
    
    /**
     * Destructor - automatically unregisters from global JVM manager
     */
    ~KoreanJNIBridge();
    
    /**
     * Initialize the JNI bridge
     * @return OBP_SUCCESS on success, error code on failure
     */
    int initialize();
    
    /**
     * Segment Korean text using Lucene Korean analyzer
     * @param text Input text to segment
     * @param tokens Output vector to store segmented tokens
     * @return OBP_SUCCESS on success, error code on failure
     */
    int segment(const std::string& text, std::vector<std::string>& tokens);
    
    // Error handling
    int get_last_error_code() const { return last_error_code_; }
    const std::string& get_last_error_message() const { return last_error_message_; }

private:
    /**
     * Load Java classes and cache method IDs
     */
    int load_java_classes(JNIEnv* env);
    
    /**
     * Perform actual segmentation using JNI
     */
    int do_segment(JNIEnv* env, const std::string& text, std::vector<std::string>& tokens);
    
    // Error handling helpers
    void set_error(int code, const std::string& message);
    void clear_error();
    
    // Disable copy and move
    KoreanJNIBridge(const KoreanJNIBridge&) = delete;
    KoreanJNIBridge& operator=(const KoreanJNIBridge&) = delete;
};

/**
 * Korean JNI Bridge Manager (Singleton)
 * @brief Manages a global Korean JNI bridge instance
 */
class KoreanJNIBridgeManager {
public:
    /**
     * Get the singleton instance
     */
    static KoreanJNIBridgeManager& get_instance();
    
    /**
     * Get the shared bridge instance
     */
    std::shared_ptr<KoreanJNIBridge> get_bridge();
    
    /**
     * Initialize the bridge (lazy initialization)
     */
    int initialize();

private:
    std::shared_ptr<KoreanJNIBridge> bridge_;
    std::mutex mutex_;
    
    KoreanJNIBridgeManager() = default;
    ~KoreanJNIBridgeManager() = default;
    
    // Disable copy
    KoreanJNIBridgeManager(const KoreanJNIBridgeManager&) = delete;
    KoreanJNIBridgeManager& operator=(const KoreanJNIBridgeManager&) = delete;
};

} // namespace korean_ftparser
} // namespace oceanbase

// Plugin interface functions (declarations only)
extern "C" {
    int korean_ftparser_init(ObPluginParamPtr param);
    int korean_ftparser_deinit(ObPluginParamPtr param);
    int korean_ftparser_scan_begin(ObPluginFTParserParamPtr param);
    int korean_ftparser_scan_end(ObPluginFTParserParamPtr param);
    int korean_ftparser_next_token(ObPluginFTParserParamPtr param, char **word, int64_t *word_len, int64_t *char_cnt, int64_t *word_freq);
    int korean_ftparser_get_add_word_flag(uint64_t *flag);
}
