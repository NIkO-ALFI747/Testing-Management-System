#include <stdint.h>
#include <io.h>

#ifndef ECB
#define ECB 1
#endif

#ifndef CBC
#define CBC 1
#endif

#ifndef CFB
#define CFB 1
#endif

#define Nstb 16
#define Nwb 4
#define Nstw Nstb / Nwb

#define Nk 4
#define Nr 10

class AES {
private:
    typedef uint8_t key_t[Nk][Nwb];
    typedef uint8_t round_key_t[Nstw * (Nr + 1)][Nwb];
    static void key_expansion(round_key_t* round_key, const key_t* key);
    static uint8_t get_sbox_value(uint8_t num);
    static void rot_word(uint8_t* word);
    static void sub_word(uint8_t* word);
    typedef uint8_t state_t[Nwb][Nstw];
    static void add_round_key(state_t* state, uint8_t round_index, const round_key_t* round_key);
    static void sub_bytes(state_t* state);
    static void shift_rows(state_t* state);
    static uint8_t xtime(uint8_t x);
    static void mix_columns(state_t* state);
    static uint8_t multiply(uint8_t x, uint8_t y);
    static void encrypt(state_t* state, const round_key_t* round_key);

#if (defined(ECB) && ECB == 1) || (defined(CBC) && CBC == 1)
    static uint8_t get_inv_sbox_value(uint8_t num);
    static void inv_mix_columns(state_t* state);
    static void inv_sub_bytes(state_t* state);
    static void inv_shift_rows(state_t* state);
    static void decrypt(state_t* state, const round_key_t* round_key);
#endif

#if (defined(CBC) && (CBC == 1)) || (defined(CFB) && (CFB == 1))
    static void xor_state_blocks(uint8_t* buf1, const uint8_t* buf2);
#endif

public:
#if defined(ECB) && (ECB == 1)
    static void ECB_encrypt(const uint8_t* key, uint8_t* buf, size_t buf_size);
    static void ECB_decrypt(const uint8_t* key, uint8_t* buf, size_t buf_size);
    static void test_ECB();
#endif

#if defined(CBC) && (CBC == 1)
    static void CBC_encrypt(const uint8_t* key, const uint8_t* iv, uint8_t* buf, size_t buf_size);
    static void CBC_decrypt(const uint8_t* key, const uint8_t* iv, uint8_t* buf, size_t buf_size);
    static void test_CBC();
#endif

#if defined(CFB) && (CFB == 1)
    static void CFB_encrypt(const uint8_t* key, const uint8_t* iv, uint8_t* buf, size_t buf_size);
    static void CFB_decrypt(const uint8_t* key, const uint8_t* iv, uint8_t* buf, size_t buf_size);
    static void test_CFB();
#endif
};