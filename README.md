# Testing Management System

A secure, console-based testing platform developed in C++20 that implements cryptographic protection for answer keys and provides a robust template-driven architecture for educational assessments.

## Overview

The Testing Management System is a command-line application designed for creating and administering educational tests with built-in security features. The system separates test creation from test administration, encrypting answer keys using AES encryption to prevent unauthorized access while maintaining flexible test formats.

## Technical Architecture

### Core Components

#### 1. **AES Encryption Module** (`aes.cpp`, `aes.h`)
- **Implementation**: Custom AES-128 implementation supporting three cipher modes
  - ECB (Electronic Codebook)
  - CBC (Cipher Block Chaining)
  - CFB (Cipher Feedback) - Primary mode used for answer encryption
- **Key Management**: Fixed 128-bit key with initialization vector
- **Block Operations**: 16-byte block size with PKCS padding
- **Design Rationale**: CFB mode selected for streaming encryption of variable-length answer files
- **Algorithm Specification**: FIPS-197 compliant AES-128 with 10 transformation rounds

**Core AES Transformations** (per FIPS-197 standard):

1. **SubBytes Transformation**: Non-linear byte substitution using precomputed S-box lookup table (256-byte array). Each byte in the 4x4 state matrix is replaced with its S-box value, providing confusion in the cipher. The S-box is derived from the multiplicative inverse in GF(2^8) followed by an affine transformation.

2. **ShiftRows Transformation**: Cyclical left shift of state matrix rows - row 0 unchanged, row 1 shifted by 1 byte, row 2 by 2 bytes, row 3 by 3 bytes. This provides diffusion by mixing bytes across columns.

3. **MixColumns Transformation**: Matrix multiplication in Galois Field GF(2^8) using polynomial `{03}x^3 + {01}x^2 + {01}x + {02}` for encryption (and its inverse for decryption). Implemented via the `xtime()` operation for efficient GF multiplication, where `xtime(x) = (x << 1) ^ (0x1b if msb set)`. Each column is treated as a four-term polynomial and multiplied with a fixed polynomial modulo x^4 + 1.

4. **AddRoundKey**: XOR each byte of the state with the corresponding byte from the expanded round key. This is the only transformation that incorporates the secret key material directly.

5. **Key Expansion**: Generates 11 round keys (176 bytes total) from the 128-bit input key using `RotWord`, `SubWord`, and round constant XOR operations. The expansion algorithm applies the S-box transformation to rotated key words and XORs them with round constants from the Rijndael polynomial x^i mod (x^8 + x^4 + x^3 + x + 1).

**CFB Mode Implementation Details**:
- **Encryption**: `IV → Encrypt(IV) → XOR with plaintext → ciphertext`. The ciphertext block becomes the next IV, creating a self-synchronizing stream cipher.
- **Decryption**: Identical encryption function but XORed with ciphertext to recover plaintext. This is a key advantage - decryption doesn't require the inverse AES transformation.
- **Streaming Property**: Processes data in 16-byte segments without requiring full-block padding on final segment during decryption. Errors in ciphertext propagate only to the affected block and the next block (limited error propagation).
- **State Chaining**: Each encrypted block depends on all previous plaintext blocks via IV feedback, preventing identical plaintext blocks from producing identical ciphertext.

**Galois Field Arithmetic**:
```cpp
// Efficient GF(2^8) multiplication via repeated xtime operations
uint8_t multiply(uint8_t x, uint8_t y) {
    return (((y & 1) * x) ^                        // 1x
            ((y >> 1 & 1) * xtime(x)) ^            // 2x  
            ((y >> 2 & 1) * xtime(xtime(x))) ^     // 4x
            ((y >> 3 & 1) * xtime(xtime(xtime(x)))); // 8x
}

// Polynomial reduction in GF(2^8) with modulus 0x1b (x^8 + x^4 + x^3 + x + 1)
uint8_t xtime(uint8_t x) {
    return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}
```

**Encryption Pipeline**:
```cpp
// 10-round transformation sequence
void encrypt(state_t* state, const round_key_t* round_key) {
    add_round_key(state, 0, round_key);      // Initial key addition
    for (round = 1; round < 10; ++round) {
        sub_bytes(state);                     // S-box substitution
        shift_rows(state);                    // Row shifting
        mix_columns(state);                   // Column mixing (rounds 1-9 only)
        add_round_key(state, round, round_key);
    }
    sub_bytes(state);                         // Final round (no MixColumns)
    shift_rows(state);
    add_round_key(state, 10, round_key);
}
```

**Key Technical Decisions**:
```cpp
// CFB mode provides streaming capability for text encryption
// without requiring padding to block boundaries during decryption
AES::CFB_encrypt(key, iv, buffer, buffer_size);
AES::CFB_decrypt(key, iv, buffer, buffer_size);
```

#### 2. **Template Parser** (`questions_answers.cpp`, `questions_answers.h`)
- **Pattern Matching Engine**: Hierarchical content validation system
- **Content Classes**:
  - `Content`: Base class for pattern generation
  - `SingleLevelContent`: Handles linear content structures (open questions)
  - `TwoLevelContent`: Manages nested structures (questions with multiple choice options)
  - `TestContent`: Top-level orchestrator for complete test documents
- **Parsing Strategy**: Character-by-character state machine with delimiter detection
- **Validation**: Strict template conformance checking before processing

**Architecture Pattern**:
```
TestContent
├── Title + Metadata
├── Close Section (TwoLevelContent)
│   ├── Question 1
│   │   ├── Answer 1.1
│   │   ├── Answer 1.2
│   │   └── Answer 1.3
│   └── Question N...
└── Open Section (SingleLevelContent)
    ├── Question 1
    └── Question N...
```

#### 3. **Main Application Logic** (`Testing-Management-System.cpp`)
- **Mode Selection**: Dual-mode operation (Development vs. Testing)
- **UTF-8 Support**: Wide character handling throughout via `wstring`
- **File Operations**: Separate handlers for text (UTF-8) and binary (encrypted) formats
- **Scoring Algorithm**: Weighted evaluation across question types

### Data Flow Architecture

#### Test Creation Workflow
```
1. Read questions file (plaintext UTF-8)
   └─> Validate against question template schema
2. Read answers file (plaintext UTF-8)
   └─> Validate against answer template schema
3. UTF-8 to byte conversion
   └─> PKCS padding to 16-byte boundaries
4. AES-CFB encryption with static key/IV
5. Write encrypted binary answer file
```

#### Test Administration Workflow
```
1. Load questions file (plaintext)
   └─> Parse and extract question structures
2. Load encrypted answers file (binary)
   └─> AES-CFB decryption
   └─> UTF-8 reconstruction
   └─> Validate and extract answer keys
3. Present questions interactively
4. Collect user responses
5. Compare responses against decrypted answers
6. Calculate and display score
```

## Technical Highlights

### Security Implementation

**Encryption Pipeline**:
```cpp
// UTF-8 to binary conversion with padding
wstring_convert<codecvt_utf8<wchar_t>> converter;
string utf8str = converter.to_bytes(content);
size_t buffer_size = (size_t)(16 * ceil(text_size / 16.0));  // 16-byte alignment

// CFB encryption maintains streaming properties
AES::CFB_encrypt(key, iv, buffer, buffer_size);
```

**Challenges Addressed**:
- **Padding Strategy**: Dynamic buffer allocation to ensure 16-byte block alignment
- **Null Termination**: Preserved during encryption for decryption boundary detection
- **Encoding Safety**: Exception handling for UTF-8 conversion edge cases

### Template Validation System

**Pattern Matching Approach**:
```cpp
// Hierarchical parsing with state tracking
bool TwoLevelContent::get_rows_content(
    wstring str, 
    vector<wstring>& rows_content, 
    vector<vector<wstring>>& twolevel_rows_content, 
    size_t& char_num
)
```

**Design Considerations**:
- **Delimiter-Based**: Uses numbered lists (1., 1.1., etc.) for structure detection
- **Stateful Parsing**: Maintains character position and row counters
- **Strict Validation**: Rejects malformed input rather than attempting recovery
- **Bidirectional Processing**: Supports both question extraction and answer key matching

### Unicode Handling

**Wide Character Implementation**:
```cpp
_setmode(_fileno(stdout), _O_U16TEXT);  // Console UTF-16 output
_setmode(_fileno(stdin), _O_U16TEXT);   // Console UTF-16 input

// File I/O with UTF-8 codecvt
file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t>));
```

**Rationale**: 
- Windows console requires UTF-16 for non-ASCII display
- File storage uses UTF-8 for cross-platform compatibility
- Intermediate `wstring` representation for unified processing

## Build Configuration

### Platform Requirements
- **OS**: Windows (Win32/x64)
- **Compiler**: MSVC with C++20 standard
- **Toolset**: Visual Studio Platform Toolset v145
- **Target**: Windows 10 SDK

### Compiler Flags
```xml
<LanguageStandard>stdcpp20</LanguageStandard>
<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
<CharacterSet>Unicode</CharacterSet>
```

### Dependencies
- **Standard Library**: `<iostream>`, `<fstream>`, `<codecvt>`, `<vector>`, `<unordered_set>`
- **Platform-Specific**: `<fcntl.h>`, `<io.h>` for Windows file mode control

## File Structure

```
Testing-Management-System/
├── Testing-Management-System.cpp   # Main application entry point
├── aes.cpp                         # AES cipher implementation
├── aes.h                           # AES class interface
├── questions_answers.cpp           # Template parser implementation
├── questions_answers.h             # Content class definitions
├── examples/
│   ├── example_1.5_prog_questions.txt    # Sample questions file
│   ├── example_1.4_prog_answers.txt      # Sample plaintext answers
│   └── enc_prog_answers.bin              # Encrypted answer key
└── Testing-Management-System.vcxproj     # Visual Studio project
```

## Template Specification

### Questions File Format
```
Вопросы и варианты ответов.
Название теста: [Test Name]
-------------------------------------------------------------------
Закрытые вопросы и возможные ответы к ним:
1. [Multiple Choice Question Text]
1.1. [Option A]
1.2. [Option B]
1.3. [Option C]
-------------------------------------------------------------------
Открытые вопросы:
1. [Open-Ended Question Text]
2. [Another Open Question]
-------------------------------------------------------------------
```

### Answers File Format
```
Правильные ответы.
Название теста: [Test Name]
-------------------------------------------------------------------
По закрытым вопросам.
1.
1.3.
2.
2.1.
2.4.
-------------------------------------------------------------------
По открытым вопросам.
1. [Expected Answer Text]
2. [Expected Answer Text]
-------------------------------------------------------------------
```

### Format Requirements
- **Strict Delimiters**: Three-dash line separators (`---`) required
- **Numbered Lists**: Sequential numbering starting from 1
- **Nested Numbering**: Format `N.M.` for sub-items under question N
- **Text Encoding**: UTF-8 with BOM for file storage
- **Answer Multiplicity**: Multiple correct answers supported for closed questions
- **Single Answer Constraint**: Open questions accept only one correct answer

## Technical Challenges & Solutions

### Challenge 1: Stateful Template Parsing
**Problem**: Need to validate complex nested structures without regex support in C++20  
**Solution**: Implemented character-by-character state machine with position tracking
```cpp
size_t char_num = 0;  // Global position tracker
bool get_row_content(..., size_t& char_num);  // Pass-by-reference for state
```

### Challenge 2: Encryption Key Distribution
**Problem**: Secure key management for educational context  
**Solution**: Hardcoded key/IV pair in binary (not for production security, but sufficient for academic integrity enforcement)
```cpp
uint8_t key[16] = { 0x2b, 0x7e, ..., 0x3c };
uint8_t iv[16] = { 0x00, 0x01, ..., 0x0f };
```
**Rationale**: Prevents casual answer key access while maintaining simplicity for instructors

### Challenge 3: Cross-Platform Text Encoding
**Problem**: Windows console expects UTF-16, files use UTF-8  
**Solution**: Dual conversion layer with explicit mode setting
```cpp
// Console: UTF-16LE
_setmode(_fileno(stdout), _O_U16TEXT);

// File: UTF-8 with codecvt
wstring_convert<codecvt_utf8<wchar_t>> converter;
```

### Challenge 4: Dynamic Memory Management for Encryption
**Problem**: Unknown answer file size requires dynamic allocation  
**Solution**: Measure-allocate-encrypt-cleanup pattern
```cpp
size_t buffer_size = (size_t)(16 * ceil(text_size / 16.0));
uint8_t* buffer = new uint8_t[buffer_size];
// ... encryption ...
delete[] buffer;
```

## Testing Modes

### Development Mode
- **Purpose**: Create and encrypt answer keys
- **Input**: Two plaintext files (questions and answers)
- **Validation**: Template conformance for both files
- **Output**: Encrypted binary answer file
- **Security**: Immediate encryption prevents plaintext answer exposure

### Testing Mode
- **Purpose**: Administer tests with encrypted answers
- **Input**: Questions file (plaintext) + encrypted answers (binary)
- **Process**: Interactive CLI-based question delivery
- **Answer Collection**: 
  - Closed questions: Comma-separated indices (e.g., "1,3,4")
  - Open questions: Free text input
- **Evaluation**: Real-time scoring with percentage calculation

## Answer Evaluation Logic

### Closed Questions
```cpp
// Set-based comparison for unordered correct answers
unordered_set<size_t> user_answers;
for (auto answer_index : correct_indices) {
    if (user_answers.find(answer_index) != user_answers.end())
        num_correct++;
}
```

### Open Questions
```cpp
// Exact match with optional quote handling
if ((expected == "\"" + user_input + "\"") || (expected == user_input))
    num_correct++;
```

### Scoring Formula
```cpp
double score = ((double)num_correct_answers / total_correct_answers) * 100.0;
```

## Limitations & Design Trade-offs

### Current Limitations
1. **Windows-Only**: Platform-specific console APIs and file mode handling
2. **Fixed Encryption Key**: Hardcoded key/IV reduces security for high-stakes testing
3. **No Answer Feedback**: Only final score displayed, no per-question breakdown
4. **Template Rigidity**: Strict format requirements may frustrate users with minor formatting errors
5. **No Question Randomization**: Fixed question order for all test takers
6. **No Time Limits**: No built-in time constraints for test administration

### Design Trade-offs
- **Security vs. Usability**: Chose hardcoded keys for simplicity in educational context
- **Flexibility vs. Validation**: Strict templates ensure consistent parsing but reduce format flexibility
- **Performance vs. Readability**: Character-by-character parsing prioritizes clarity over speed
- **Storage vs. Security**: Plaintext questions allow preview but encrypted answers prevent key leaks

## Potential Enhancements

### Security Improvements
- Key derivation from instructor-provided passphrase (PBKDF2)
- Answer file integrity verification (HMAC)
- Secure key storage using Windows DPAPI

### Functional Extensions
- Question bank import/export (JSON/XML)
- Randomized question order and answer shuffling
- Partial credit for incomplete correct answer sets
- Test session logging and result persistence
- Web-based interface for remote administration

### Code Quality
- Unit test coverage for template parser
- Cross-platform support (Linux/macOS) via portable file I/O
- Configuration file for template customization
- Better error messages with line number reporting
- Refactor template validation into separate module

## Usage Example

### Creating a Test
```bash
> Testing-Management-System.exe
- Выберите режим работы программы:
1. Разработка теста
2. Тестирование
- 1

# Follow prompts to select:
# 1. questions.txt (plaintext)
# 2. answers.txt (plaintext)
# 3. encrypted_answers.bin (output)
```

### Administering a Test
```bash
> Testing-Management-System.exe
- Выберите режим работы программы:
1. Разработка теста
2. Тестирование
- 2

# Follow prompts to select:
# 1. questions.txt (plaintext)
# 2. encrypted_answers.bin (encrypted)

# Answer questions interactively
# Receive final score at completion
```

## Technical Insights

### Why CFB Mode?
- **Stream Processing**: No padding required during decryption
- **Error Propagation**: Limited to single block (16 bytes)
- **IV Requirement**: Prevents pattern recognition in encrypted answers

### Why Wide Strings?
- **Windows Console**: Native UTF-16 support for non-ASCII characters
- **Internationalization**: Supports Cyrillic and other non-Latin scripts
- **Standard Library**: `<codecvt>` provides UTF-8 ↔ UTF-16 conversion

### Why Template-Based Parsing?
- **Predictability**: Known format enables optimized parsing
- **Validation**: Reject malformed input early
- **Maintainability**: Clear structure for future enhancements

## Compilation

### Visual Studio
```bash
# Open Testing-Management-System.sln
# Build > Build Solution (Ctrl+Shift+B)
# Configuration: Release x64 recommended
```

### Command Line (MSVC)
```bash
cl /EHsc /std:c++20 /utf-8 /Fe:TestingSystem.exe ^
   Testing-Management-System.cpp aes.cpp questions_answers.cpp
```

## License

This project is licensed under the MIT License – see the LICENSE file for details.

## Author

**Note**: The hardcoded encryption key makes this unsuitable for high-security applications.

---

**Language**: C++20  
**Platform**: Windows (x86/x64)  
**Paradigm**: Procedural with OOP for content modeling  
**Primary Libraries**: STL, Windows CRT
