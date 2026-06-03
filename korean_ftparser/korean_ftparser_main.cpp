/**
 * Copyright (c) 2023 OceanBase
 * Korean Fulltext Parser Plugin - Main Entry Point
 */

#include "korean_jni_bridge.h"

extern "C" {

/**
 * Plugin initialization function
 * @param plugin Plugin parameter pointer
 * @return OBP_SUCCESS on success, error code on failure
 */
int plugin_init_korean(ObPluginParamPtr plugin)
{
    int ret = OBP_SUCCESS;
    
    if (0 == plugin) {
        ret = OBP_INVALID_ARGUMENT;
        return ret;
    }
    
    // Define the ftparser plugin descriptor
    ObPluginFTParser parser = {
        .init              = korean_ftparser_init,
        .deinit            = korean_ftparser_deinit,
        .scan_begin        = korean_ftparser_scan_begin,
        .scan_end          = korean_ftparser_scan_end,
        .next_token        = korean_ftparser_next_token,
        .get_add_word_flag = korean_ftparser_get_add_word_flag
    };
    
    // Register the ftparser plugin with OceanBase
    ret = OBP_REGISTER_FTPARSER(plugin,
                                "korean_ftparser",
                                parser,
                                "Korean language fulltext parser");
    
    return ret;
}

// Plugin declaration
OBP_DECLARE_PLUGIN(korean_ftparser)
{
    OBP_AUTHOR_OCEANBASE,                    // Plugin author
    OBP_MAKE_VERSION(0, 0, 1),              // Plugin version
    OBP_LICENSE_MULAN_PSL_V2,               // Plugin license
    plugin_init_korean,                     // Plugin initialization function
    nullptr,                                // Plugin deinitialization function (not needed)
} OBP_DECLARE_PLUGIN_END;

} // extern "C"
