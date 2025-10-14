import java.io.StringReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.lucene.analysis.Analyzer;
import org.apache.lucene.analysis.TokenStream;
import org.apache.lucene.analysis.custom.CustomAnalyzer;
import org.apache.lucene.analysis.tokenattributes.CharTermAttribute;

/**
 * Japanese Segmenter using ES Database Best Practice with CustomAnalyzer
 */
public class JapaneseSegmenter {
    private Analyzer analyzer;
    private boolean initialized = false;
    
    /**
     * Constructor
     */
    public JapaneseSegmenter() {
        try {
            // Use CustomAnalyzer.builder() (including BaseForm)
            this.analyzer = CustomAnalyzer.builder()
                .withTokenizer("japanese")                    // kuromoji_tokenizer
                .addTokenFilter("japaneseBaseForm")           // kuromoji_baseform
                .addTokenFilter("japanesePartOfSpeechStop")   // kuromoji_part_of_speech
                .addTokenFilter("cjkWidth")                   // cjk_width
                .addTokenFilter("lowercase")                  // lowercase
                .addTokenFilter("stop", "words", "org/apache/lucene/analysis/ja/stopwords.txt")  // ja_stop
                .build();
                
            this.initialized = true;
            System.out.println("JapaneseSegmenter initialized with ES Complete Solution (CustomAnalyzer)");
            System.out.println("Filters: japanese + japaneseBaseForm + japanesePartOfSpeechStop + cjkWidth + lowercase + ja_stop");
        } catch (Exception e) {
            System.err.println("Failed to initialize CustomAnalyzer: " + e.getMessage());
            e.printStackTrace();
            this.initialized = false;
        }
    }
    
    /**
     * Segment Japanese text into tokens using ES Database Best Practice
     * @param text The input text to segment
     * @return Array of segmented tokens
     */
    public String[] segment(String text) {
        if (!initialized) {
            throw new IllegalStateException("JapaneseSegmenter not initialized");
        }
        
        if (text == null || text.trim().isEmpty()) {
            return new String[0];
        }
        
        System.out.println("=== OCEANBASE JNI CALL === Segmenting text with ES CustomAnalyzer: \"" + text + "\" (length: " + text.length() + ")");
        
        List<String> tokens = new ArrayList<>();
        
        try {
            TokenStream tokenStream = analyzer.tokenStream("content", new StringReader(text));
            CharTermAttribute termAttr = tokenStream.addAttribute(CharTermAttribute.class);
            
            tokenStream.reset();
            while (tokenStream.incrementToken()) {
                String token = termAttr.toString().trim();
                if (!token.isEmpty()) {
                    tokens.add(token);
                }
            }
            tokenStream.end();
            tokenStream.close();
            
        } catch (IOException e) {
            System.err.println("Error during tokenization: " + e.getMessage());
            e.printStackTrace();
            return new String[0];
        }
        
        String[] result = tokens.toArray(new String[0]);
        System.out.println("=== OCEANBASE JNI RESULT === ES CustomAnalyzer result: " + java.util.Arrays.toString(result));
        return result;
    }
    
    /**
     * Cleanup resources
     */
    public void cleanup() {
        System.out.println("JapaneseSegmenter cleanup called");
        if (analyzer != null) {
            analyzer.close();
        }
        initialized = false;
    }
    
    /**
     * Test main method
     */
    public static void main(String[] args) {
        JapaneseSegmenter segmenter = new JapaneseSegmenter();
        
        // Test with English text
        String[] result1 = segmenter.segment("Hello world");
        System.out.println("English test: " + java.util.Arrays.toString(result1));
        
        // Test with Japanese text
        String[] result2 = segmenter.segment("私は学生です");
        System.out.println("Japanese test: " + java.util.Arrays.toString(result2));
        
        // Test with mixed text (English + Japanese)
        String[] result3 = segmenter.segment("Hello こんにちは world");
        System.out.println("Mixed test: " + java.util.Arrays.toString(result3));
        
        // Test with complex Japanese text
        String[] result4 = segmenter.segment("東京都渋谷区でコンピューターを勉強しています");
        System.out.println("Complex Japanese test: " + java.util.Arrays.toString(result4));
        
        // Test with mixed English-Japanese text (OceanBase example)
        String[] result5 = segmenter.segment("OceanBaseデータベースを選ぶ理由");
        System.out.println("OceanBase mixed test: " + java.util.Arrays.toString(result5));
        
        segmenter.cleanup();
    }
}