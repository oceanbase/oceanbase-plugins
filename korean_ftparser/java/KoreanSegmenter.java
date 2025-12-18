import java.io.StringReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.lucene.analysis.Analyzer;
import org.apache.lucene.analysis.TokenStream;
import org.apache.lucene.analysis.custom.CustomAnalyzer;
import org.apache.lucene.analysis.tokenattributes.CharTermAttribute;

/**
 * Korean Segmenter using static method to reduce memory usage
 */
public class KoreanSegmenter {
    // 简单直接的静态初始化 - 只创建一次，绝对保证
    private static final Analyzer STATIC_ANALYZER = createAnalyzer();
    
    /**
     * 创建分析器 - 只在类加载时调用一次
     */
    private static Analyzer createAnalyzer() {
        try {
            System.out.println("KoreanSegmenter: Creating static analyzer (once per JVM)");
            return CustomAnalyzer.builder()
                .withTokenizer("korean", "decompoundMode", "mixed")  // MIXED mode!
                .addTokenFilter("lowercase")    // lowercase (basic normalization)
                .build();
        } catch (Exception e) {
            System.err.println("Failed to initialize KoreanSegmenter: " + e.getMessage());
            e.printStackTrace();
            throw new RuntimeException("Cannot initialize KoreanAnalyzer", e);
        }
    }
    
    /**
     * Segment Korean text into tokens
     * @param text The Korean text to segment
     * @return Array of token strings
     */
    public static String[] segment(String text) {
        if (text == null || text.trim().isEmpty()) {
            return new String[0];
        }

        List<String> tokens = new ArrayList<>();

        try (TokenStream tokenStream = STATIC_ANALYZER.tokenStream("content", new StringReader(text))) {
            CharTermAttribute attr = tokenStream.addAttribute(CharTermAttribute.class);
            
            tokenStream.reset();
            while (tokenStream.incrementToken()) {
                String token = attr.toString();
                tokens.add(token);
            }
            tokenStream.end();

        } catch (IOException e) {
            System.err.println("Error during Korean tokenization: " + e.getMessage());
            return new String[0];
        }

        return tokens.toArray(new String[0]);
    }
    
    public static boolean isStaticInitialized() {
        return STATIC_ANALYZER != null;
    }
    
    public static void main(String[] args) {
        // Test cases
        String[] testTexts = {
            "안녕하세요",
            "데이터베이스",
            "학교에 갑니다",
            "한국어 처리",
            "OceanBase 데이터베이스 관리 시스템"
        };

        for (String text : testTexts) {
            System.out.println("Testing: \"" + text + "\"");
            String[] tokens = KoreanSegmenter.segment(text);
            System.out.println("Tokens: " + java.util.Arrays.toString(tokens));
        }
    }
}