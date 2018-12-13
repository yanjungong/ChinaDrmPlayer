#include  <android/log.h>

#define  TAG    "ChinaDrm"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)

#define UNILOGD LOGI
#define UNILOGE LOGI
#define UNILOGW LOGI