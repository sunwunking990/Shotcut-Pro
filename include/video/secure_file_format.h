#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#else
// Mock OpenSSL types when not available
typedef void EVP_CIPHER_CTX;
typedef void EVP_MD_CTX;
typedef void EVP_CIPHER;
#define EVP_MAX_IV_LENGTH 16
#define EVP_MAX_KEY_LENGTH 32
#define EVP_MAX_BLOCK_LENGTH 16
#endif

namespace Shotcut::Video {

// Encryption algorithms
enum class EncryptionAlgorithm : uint8_t {
    AES256_GCM,     // AES-256 GCM mode
    ChaCha20_Poly1305, // ChaCha20-Poly1305
    AES256_CBC,      // AES-256 CBC mode (legacy)
    None            // No encryption
};

// Compression algorithms
enum class CompressionAlgorithm : uint8_t {
    LZ4,            // LZ4 compression
    ZSTD,            // Zstandard compression
    None             // No compression
};

// File format versions
enum class FileFormatVersion : uint16_t {
    V1_0 = 0x0100,  // Initial version
    V1_1 = 0x0101,  // Added metadata encryption
    V1_2 = 0x0102,  // Added delta compression
    CURRENT = V1_2
};

// Secure file format header
struct SecureFileHeader {
    // Magic identifier (4 bytes): "SCVF" (ShotCut Video Format)
    static constexpr uint32_t MAGIC = 0x53435646; // "SCVF"
    
    // File format version (2 bytes)
    FileFormatVersion version = FileFormatVersion::CURRENT;
    
    // Flags (2 bytes)
    uint16_t flags = 0;
    
    // Encryption algorithm (1 byte)
    EncryptionAlgorithm encryption = EncryptionAlgorithm::AES256_GCM;
    
    // Compression algorithm (1 byte)
    CompressionAlgorithm compression = CompressionAlgorithm::LZ4;
    
    // Reserved (8 bytes)
    uint8_t reserved[8] = {0};
    
    // File metadata
    uint64_t original_size = 0;           // Original uncompressed size
    uint64_t compressed_size = 0;          // Compressed size
    uint64_t encrypted_size = 0;           // Final encrypted size
    uint32_t frame_count = 0;             // Number of video frames
    uint32_t audio_frame_count = 0;       // Number of audio frames
    float frame_rate = 0.0f;              // Video frame rate
    uint16_t width = 0;                    // Video width
    uint16_t height = 0;                   // Video height
    uint8_t audio_channels = 0;             // Audio channel count
    uint32_t audio_sample_rate = 0;        // Audio sample rate
    
    // Timestamps
    std::chrono::system_clock::time_point creation_time;
    std::chrono::system_clock::time_point modification_time;
    std::chrono::duration<int64_t> duration{0}; // Video duration
    
    // Checksums (32 bytes)
    uint8_t header_checksum[32];     // SHA-256 of header
    uint8_t metadata_checksum[32];   // SHA-256 of metadata
    uint8_t content_checksum[32];    // SHA-256 of entire content
    
    // Encryption metadata
    uint8_t encryption_iv[16];       // Initialization vector
    uint8_t encryption_salt[16];     // Key derivation salt
    uint32_t encryption_iterations = 100000; // PBKDF2 iterations
    
    // Flag bits
    struct Flags {
        uint16_t encrypted : 1;           // Content is encrypted
        uint16_t compressed : 1;           // Content is compressed
        uint16_t metadata_encrypted : 1;    // Metadata is encrypted
        uint16_t delta_compressed : 1;      // Uses delta compression
        uint16_t has_audio : 1;            // Contains audio data
        uint16_t has_subtitles : 1;         // Contains subtitle data
        uint16_t has_chapters : 1;          // Contains chapter markers
        uint16_t reserved1 : 8;            // Reserved for future use
    };
    
    // Validate header integrity
    bool is_valid() const;
    bool verify_checksums() const;
    void calculate_checksums();
};

// Frame entry header
struct FrameEntry {
    uint64_t offset = 0;              // Offset from start of data section
    uint64_t size = 0;                 // Compressed size of frame
    uint64_t original_size = 0;          // Original uncompressed size
    uint32_t frame_number = 0;           // Frame sequence number
    uint32_t timestamp_ms = 0;           // Frame timestamp in milliseconds
    uint16_t width = 0;                 // Frame width (can vary)
    uint16_t height = 0;                // Frame height (can vary)
    uint8_t type = 0;                   // Frame type (I, P, B frame)
    uint8_t quality = 0;                 // Frame quality level
    uint8_t flags = 0;                  // Frame-specific flags
    uint8_t checksum[16];                // Frame checksum
};

// Metadata entry
struct MetadataEntry {
    std::string key;
    std::vector<uint8_t> value;
    uint8_t type = 0;                  // Metadata type
    uint16_t flags = 0;                 // Entry flags
};

// Chunk types for the file format
enum class ChunkType : uint32_t {
    Header = 0x48454144,         // "HEAD"
    VideoData = 0x56494445,       // "VIDE"
    AudioData = 0x41554445,       // "AUDE"
    SubtitleData = 0x53554445,     // "SUBT"
    ChapterData = 0x43484150,       // "CHAP"
    Metadata = 0x4D455441,         // "META"
    Index = 0x494E4445,             // "INDE"
    Signature = 0x5349474E,         // "SIGN"
    End = 0x454E44                   // "END"
};

// Chunk header
struct ChunkHeader {
    ChunkType type;
    uint32_t size;
    uint32_t checksum;
    uint8_t flags;
    uint8_t reserved[3] = {0};
};

// Encryption context
class EncryptionContext {
private:
    EncryptionAlgorithm algorithm_;
    std::vector<uint8_t> key_;
    std::vector<uint8_t> iv_;
    std::vector<uint8_t> salt_;
    
    // OpenSSL contexts
    EVP_CIPHER_CTX* cipher_ctx_;
    const EVP_CIPHER* cipher_;
    
    bool initialized_;
    
public:
    EncryptionContext(EncryptionAlgorithm algorithm);
    ~EncryptionContext();
    
    // Key management
    bool set_key(const std::vector<uint8_t>& key);
    bool set_key_from_password(const std::string& password, 
                              const std::vector<uint8_t>& salt,
                              uint32_t iterations);
    bool generate_key();
    
    // Encryption/decryption
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext);
    
    // Stream operations
    bool encrypt_stream(std::istream& input, std::ostream& output);
    bool decrypt_stream(std::istream& input, std::ostream& output);
    
    // Properties
    EncryptionAlgorithm get_algorithm() const { return algorithm_; }
    size_t get_key_size() const;
    size_t get_iv_size() const;
    bool is_initialized() const { return initialized_; }
    
private:
    const EVP_CIPHER* get_cipher() const;
    bool setup_cipher();
    void cleanup();
};

// Compression context
class CompressionContext {
private:
    CompressionAlgorithm algorithm_;
    
    // Compression state
    void* compressor_;
    void* decompressor_;
    
    bool initialized_;
    
public:
    CompressionContext(CompressionAlgorithm algorithm);
    ~CompressionContext();
    
    // Compression/decompression
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& data,
                                   size_t original_size);
    
    // Stream operations
    bool compress_stream(std::istream& input, std::ostream& output);
    bool decompress_stream(std::istream& input, std::ostream& output,
                         size_t expected_size = 0);
    
    // Properties
    CompressionAlgorithm get_algorithm() const { return algorithm_; }
    bool is_initialized() const { return initialized_; }
    
private:
    bool setup_compressor();
    bool setup_decompressor();
    void cleanup();
};

// Secure file writer
class SecureFileWriter {
private:
    std::string file_path_;
    std::ofstream file_stream_;
    
    // Contexts
    std::unique_ptr<EncryptionContext> encryption_ctx_;
    std::unique_ptr<CompressionContext> compression_ctx_;
    
    // File header
    SecureFileHeader header_;
    
    // Current writing state
    uint64_t current_offset_;
    std::vector<FrameEntry> frame_index_;
    std::vector<MetadataEntry> metadata_;
    
    // Buffers
    std::vector<uint8_t> write_buffer_;
    size_t buffer_size_;
    
    // Statistics
    uint64_t bytes_written_;
    uint32_t frames_written_;
    std::chrono::steady_clock::time_point start_time_;
    
    // Integrity checking
    EVP_MD_CTX* checksum_ctx_;
    bool verify_integrity_;
    
public:
    SecureFileWriter(const std::string& file_path);
    ~SecureFileWriter();
    
    // File operations
    bool open();
    bool close();
    bool is_open() const;
    
    // Configuration
    void set_encryption(EncryptionAlgorithm algorithm, const std::vector<uint8_t>& key);
    void set_encryption_from_password(const std::string& password);
    void set_compression(CompressionAlgorithm algorithm);
    void set_metadata(const std::vector<MetadataEntry>& metadata);
    void set_video_properties(uint16_t width, uint16_t height, float frame_rate);
    void set_audio_properties(uint8_t channels, uint32_t sample_rate);
    
    // Writing operations
    bool write_header();
    bool write_video_frame(const std::vector<uint8_t>& frame_data,
                          uint32_t frame_number,
                          uint32_t timestamp_ms,
                          uint8_t frame_type = 0,
                          uint8_t quality = 100);
    
    bool write_audio_frame(const std::vector<uint8_t>& audio_data,
                          uint32_t frame_number,
                          uint32_t timestamp_ms);
    
    bool write_metadata(const std::string& key, const std::vector<uint8_t>& value);
    bool write_chunk(ChunkType type, const std::vector<uint8_t>& data);
    
    // Progress tracking
    struct WriteProgress {
        uint64_t bytes_processed = 0;
        uint64_t total_bytes = 0;
        float percentage = 0.0f;
        uint32_t frames_written = 0;
        float processing_speed_mbps = 0.0f;
        std::chrono::milliseconds elapsed_time{0};
        std::chrono::milliseconds estimated_time_remaining{0};
    };
    
    WriteProgress get_progress() const;
    
    // Error handling
    std::string get_last_error() const { return last_error_; }
    
private:
    bool write_chunk_header(ChunkType type, uint32_t size);
    bool write_frame_entry(const FrameEntry& entry);
    bool finalize_file();
    
    void update_checksum(const uint8_t* data, size_t size);
    void update_statistics(size_t bytes_written);
    void flush_buffer();
    
    std::string last_error_;
};

// Secure file reader
class SecureFileReader {
private:
    std::string file_path_;
    std::ifstream file_stream_;
    
    // Contexts
    std::unique_ptr<EncryptionContext> encryption_ctx_;
    std::unique_ptr<CompressionContext> compression_ctx_;
    
    // File header
    SecureFileHeader header_;
    
    // Index information
    std::vector<FrameEntry> frame_index_;
    std::vector<MetadataEntry> metadata_;
    uint64_t data_start_offset_;
    
    // Reading state
    uint64_t current_offset_;
    uint32_t current_frame_;
    std::vector<uint8_t> read_buffer_;
    size_t buffer_size_;
    
    // Statistics
    uint64_t bytes_read_;
    std::chrono::steady_clock::time_point start_time_;
    
    // Integrity checking
    EVP_MD_CTX* checksum_ctx_;
    bool verify_integrity_;
    
public:
    SecureFileReader(const std::string& file_path);
    ~SecureFileReader();
    
    // File operations
    bool open();
    bool close();
    bool is_open() const;
    
    // Verification
    bool verify_file_integrity();
    bool verify_header_checksum();
    bool verify_content_checksum();
    
    // Header information
    const SecureFileHeader& get_header() const { return header_; }
    bool is_encrypted() const;
    bool is_compressed() const;
    EncryptionAlgorithm get_encryption_algorithm() const;
    CompressionAlgorithm get_compression_algorithm() const;
    
    // Access operations
    bool unlock_with_password(const std::string& password);
    bool unlock_with_key(const std::vector<uint8_t>& key);
    
    // Reading operations
    std::vector<uint8_t> read_video_frame(uint32_t frame_number);
    std::vector<uint8_t> read_audio_frame(uint32_t frame_number);
    std::vector<MetadataEntry> read_all_metadata();
    MetadataEntry get_metadata(const std::string& key);
    
    // Navigation
    bool seek_to_frame(uint32_t frame_number);
    bool seek_to_time(std::chrono::milliseconds time);
    uint32_t get_frame_count() const;
    std::chrono::milliseconds get_duration() const;
    
    // Index access
    const std::vector<FrameEntry>& get_frame_index() const { return frame_index_; }
    FrameEntry get_frame_entry(uint32_t frame_number) const;
    
    // Progress tracking
    struct ReadProgress {
        uint64_t bytes_processed = 0;
        uint64_t total_bytes = 0;
        float percentage = 0.0f;
        uint32_t frames_read = 0;
        float processing_speed_mbps = 0.0f;
        std::chrono::milliseconds elapsed_time{0};
        std::chrono::milliseconds estimated_time_remaining{0};
    };
    
    ReadProgress get_progress() const;
    
    // Error handling
    std::string get_last_error() const { return last_error_; }
    
private:
    bool read_header();
    bool read_chunk_header(ChunkHeader& header);
    bool read_frame_index();
    bool read_metadata();
    
    std::vector<uint8_t> read_chunk_data(ChunkType type);
    std::vector<uint8_t> decompress_if_needed(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decrypt_if_needed(const std::vector<uint8_t>& data);
    
    void update_checksum(const uint8_t* data, size_t size);
    void update_statistics(size_t bytes_read);
    
    std::string last_error_;
};

// File format utilities
namespace SecureFileUtils {
    // File validation
    bool is_secure_file(const std::string& file_path);
    FileFormatVersion detect_version(const std::string& file_path);
    bool verify_file_signature(const std::string& file_path);
    
    // Conversion utilities
    bool convert_to_secure_format(const std::string& input_path,
                               const std::string& output_path,
                               EncryptionAlgorithm encryption = EncryptionAlgorithm::AES256_GCM,
                               const std::string& password = "");
    
    bool convert_from_secure_format(const std::string& input_path,
                                 const std::string& output_path,
                                 const std::string& password = "");
    
    // Key management
    std::vector<uint8_t> generate_key(EncryptionAlgorithm algorithm);
    std::vector<uint8_t> derive_key_from_password(const std::string& password,
                                                 const std::vector<uint8_t>& salt,
                                                 uint32_t iterations,
                                                 size_t key_length);
    
    // Security utilities
    std::vector<uint8_t> generate_salt();
    std::vector<uint8_t> generate_iv(size_t size);
    std::vector<uint8_t> compute_checksum(const uint8_t* data, size_t size);
    bool verify_checksum(const uint8_t* data, size_t size, const uint8_t* expected_checksum);
    
    // Metadata utilities
    std::vector<MetadataEntry> extract_metadata_from_file(const std::string& file_path);
    bool add_metadata_to_file(const std::string& file_path,
                            const std::vector<MetadataEntry>& metadata,
                            const std::string& password = "");
    
    // Format information
    struct FileInformation {
        std::string format_name;
        FileFormatVersion version;
        bool is_encrypted;
        bool is_compressed;
        EncryptionAlgorithm encryption_type;
        CompressionAlgorithm compression_type;
        uint64_t original_size;
        uint64_t file_size;
        uint32_t frame_count;
        float frame_rate;
        uint16_t width;
        uint16_t height;
        bool has_audio;
        uint8_t audio_channels;
        uint32_t audio_sample_rate;
        std::chrono::milliseconds duration;
        std::chrono::system_clock::time_point creation_time;
        std::chrono::system_clock::time_point modification_time;
    };
    
    FileInformation get_file_information(const std::string& file_path);
}

// Sandbox processor for secure file operations
class SecureFileSandbox {
private:
    std::string sandbox_directory_;
    std::vector<std::string> allowed_paths_;
    std::vector<std::string> denied_paths_;
    
    // Security policies
    bool allow_network_access_;
    bool allow_filesystem_access_;
    bool allow_subprocess_execution_;
    size_t max_memory_usage_;
    std::chrono::seconds max_execution_time_;
    
    // Monitoring
    std::atomic<bool> is_running_;
    std::thread monitoring_thread_;
    
public:
    SecureFileSandbox(const std::string& sandbox_dir);
    ~SecureFileSandbox();
    
    // Sandbox configuration
    void set_security_policies(bool network, bool filesystem, bool subprocess);
    void set_memory_limit(size_t max_bytes);
    void set_execution_time_limit(std::chrono::seconds max_time);
    void add_allowed_path(const std::string& path);
    void add_denied_path(const std::string& path);
    
    // Sandbox operations
    bool initialize();
    bool cleanup();
    bool is_active() const { return is_running_; }
    
    // File operations within sandbox
    bool process_file_securely(const std::string& input_path,
                             const std::string& output_path,
                             std::function<void(const SecureFileReader::ReadProgress&)> progress_callback = nullptr);
    
    bool validate_file_integrity(const std::string& file_path);
    bool repair_corrupted_file(const std::string& file_path);
    
    // Security monitoring
    struct SecurityViolation {
        enum class Type {
            PathViolation,        // Access to denied path
            MemoryViolation,      // Memory limit exceeded
            TimeViolation,        // Execution time exceeded
            NetworkViolation,     // Network access attempted
            IntegrityViolation    // File integrity compromised
        } type;
        
        std::string description;
        std::chrono::system_clock::time_point timestamp;
        std::string file_path;
    };
    
    std::vector<SecurityViolation> get_security_violations() const;
    void clear_security_violations();
    
private:
    void monitoring_function();
    bool is_path_allowed(const std::string& path);
    void report_violation(SecurityViolation::Type type, const std::string& path, const std::string& description);
    
    std::vector<SecurityViolation> security_violations_;
    mutable std::mutex violations_mutex_;
};

} // namespace Shotcut::Video
