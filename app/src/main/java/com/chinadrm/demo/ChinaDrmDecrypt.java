package com.chinadrm.demo;

public class ChinaDrmDecrypt {
    static {
        System.loadLibrary("chinadrmdecrypt");
        System.loadLibrary("gnustl_shared");
    }

    public native boolean decrypt(String ticket, String srcFileName, String destFileName);
}