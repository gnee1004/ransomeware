#ifndef XOR_DECODER_H
#define XOR_DECODER_H

#include <stddef.h>   // size_t
#include <wchar.h>    // wchar_t

#ifdef __cplusplus
extern "C" {
#endif

	void xor_decrypt(char* data, size_t len, char     key);
	void xor_decrypt_wide(wchar_t* data, size_t len, wchar_t key);

#ifdef __cplusplus
}
#endif
#endif
#ifndef XOR_DECODER_H
#define XOR_DECODER_H

#include <stddef.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

	void xor_decrypt(char* data, size_t len, char key);
	void xor_decrypt_wide(wchar_t* data, size_t len, wchar_t key);

#ifdef __cplusplus
}
#endif
#endif
