#ifndef DECRYPTOR_H
#define DECRYPTOR_H

#define AES_KEY_SIZE 32
#define AES_BLOCK_SIZE 16

int decrypt_file(const char* encrypted_path, const unsigned char* key, const unsigned char* iv);
void restore_key(unsigned char* key_out);
void restore_iv(unsigned char* iv_out);

#endif // DECRYPTOR_H
