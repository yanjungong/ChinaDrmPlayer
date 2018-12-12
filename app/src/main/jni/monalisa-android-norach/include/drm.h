#ifndef __CPALYER_CNETSTREAM_CIPHER_H__
#define __CPALYER_CNETSTREAM_CIPHER_H__

//extern "C" {

	// 1. Set License
	int monalisa_set_license(
		char * license_base64ed         // license string ( should be base64ed )
		, int license_base64ed_len      // license string length
		, int obj_id                    // instance id
		, unsigned char * uid);         // uid
	// return code:
	// 1 - success
	// 4011 - unsupport license format
	// 4012 - license parse error
	// 4013 - uid wrong

	// 2. Decrypt Data
	int monalisa_decrypt_data(
		unsigned char * in_buf_ptr      // input buffer ptr
		, int in_buf_size               // input buffer size
		, unsigned char * out_buf_ptr   // output buffer ptr
		, int * out_buf_size            // output buffer size in, clear data length out
		, int obj_id);                  // instance id
	// return code:
	// 1 - success
	// 4021 - buf length too short
//}

#endif
