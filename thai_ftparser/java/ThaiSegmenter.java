import java.io.StringReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.lucene.analysis.Analyzer;
import org.apache.lucene.analysis.TokenStream;
import org.apache.lucene.analysis.th.ThaiAnalyzer;
import org.apache.lucene.analysis.tokenattributes.CharTermAttribute;

/**
 * Thai Segmenter using static method to reduce memory usage
 */
public class ThaiSegmenter {
    // 简单直接的静态初始化 - 只创建一次，绝对保证
    private static final ThaiAnalyzer STATIC_ANALYZER = createAnalyzer();
    
    /**
     * 创建分析器 - 只在类加载时调用一次
     */
    private static ThaiAnalyzer createAnalyzer() {
        try {
            System.out.println("ThaiSegmenter: Creating static analyzer (once per JVM)");
            return new ThaiAnalyzer();
        } catch (Exception e) {
            System.err.println("Failed to initialize ThaiSegmenter: " + e.getMessage());
            e.printStackTrace();
            throw new RuntimeException("Cannot initialize ThaiAnalyzer", e);
        }
    }
    
    /**
     * Segment Thai text into tokens
     * @param text The input text to segment
     * @return Array of segmented tokens
     */
    public static String[] segment(String text) {
        if (text == null || text.trim().isEmpty()) {
            return new String[0];
        }
        
        List<String> tokens = new ArrayList<>();
        
        try (TokenStream tokenStream = STATIC_ANALYZER.tokenStream("content", new StringReader(text))) {
            CharTermAttribute termAttr = tokenStream.addAttribute(CharTermAttribute.class);
            
            tokenStream.reset();
            while (tokenStream.incrementToken()) {
                String token = termAttr.toString();
                tokens.add(token);
            }
            tokenStream.end();
            
        } catch (IOException e) {
            System.err.println("Error during Thai tokenization: " + e.getMessage());
            return new String[0];
        }
        
        return tokens.toArray(new String[0]);
    }
    
    /**
     * 检查静态分析器是否已初始化 (总是返回true，因为如果初始化失败会抛异常)
     */
    public static boolean isStaticInitialized() {
        return STATIC_ANALYZER != null;
    }
    
    public static void main(String[] args) {
        // Test cases
        String[] result1 = ThaiSegmenter.segment("Hello world");
        System.out.println("English: " + java.util.Arrays.toString(result1));
        
        String[] result2 = ThaiSegmenter.segment("สวัสดีครับ");
        System.out.println("Thai: " + java.util.Arrays.toString(result2));
        
        String[] result3 = ThaiSegmenter.segment("Hello สวัสดี world");
        System.out.println("Mixed: " + java.util.Arrays.toString(result3));
        
        String[] result4 = ThaiSegmenter.segment("ฐานข้อมูล OceanBase เป็นระบบจัดการฐานข้อมูล");
        System.out.println("Complex: " + java.util.Arrays.toString(result4));
    }
}