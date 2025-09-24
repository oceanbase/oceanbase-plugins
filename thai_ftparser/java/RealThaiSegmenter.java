import java.io.StringReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.lucene.analysis.Analyzer;
import org.apache.lucene.analysis.TokenStream;
import org.apache.lucene.analysis.th.ThaiAnalyzer;
import org.apache.lucene.analysis.tokenattributes.CharTermAttribute;

/**
 * Real Thai Segmenter using Apache Lucene ThaiTokenizer
 */
public class RealThaiSegmenter {
    private ThaiAnalyzer analyzer;
    private boolean initialized = false;
    
    /**
     * Constructor
     */
    public RealThaiSegmenter() {
        try {
            this.analyzer = new ThaiAnalyzer();
            this.initialized = true;
            System.out.println("RealThaiSegmenter initialized with Apache Lucene ThaiAnalyzer");
        } catch (Exception e) {
            System.err.println("Failed to initialize ThaiAnalyzer: " + e.getMessage());
            this.initialized = false;
        }
    }
    
    /**
     * Segment Thai text into tokens using Lucene ThaiTokenizer
     * @param text The input text to segment
     * @return Array of segmented tokens
     */
    public String[] segment(String text) {
        if (!initialized) {
            throw new IllegalStateException("RealThaiSegmenter not initialized");
        }
        
        if (text == null || text.trim().isEmpty()) {
            return new String[0];
        }
        
        System.out.println("Segmenting text with Lucene: \"" + text + "\" (length: " + text.length() + ")");
        
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
            return new String[0];
        }
        
        String[] result = tokens.toArray(new String[0]);
        System.out.println("Lucene segmentation result: " + java.util.Arrays.toString(result));
        return result;
    }
    
    /**
     * Cleanup resources
     */
    public void cleanup() {
        System.out.println("RealThaiSegmenter cleanup called");
        if (analyzer != null) {
            analyzer.close();
        }
        initialized = false;
    }
    
    /**
     * Test main method
     */
    public static void main(String[] args) {
        RealThaiSegmenter segmenter = new RealThaiSegmenter();
        
        // Test with English text
        String[] result1 = segmenter.segment("Hello world");
        System.out.println("English test: " + java.util.Arrays.toString(result1));
        
        // Test with Thai text
        String[] result2 = segmenter.segment("สวัสดีครับ");
        System.out.println("Thai test: " + java.util.Arrays.toString(result2));
        
        // Test with mixed text
        String[] result3 = segmenter.segment("Hello สวัสดี world");
        System.out.println("Mixed test: " + java.util.Arrays.toString(result3));
        
        segmenter.cleanup();
    }
}