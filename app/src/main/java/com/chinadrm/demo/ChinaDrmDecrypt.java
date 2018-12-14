package com.chinadrm.demo;

public class ChinaDrmDecrypt {
    static {
        System.loadLibrary("chinadrmdecrypt");
        //System.loadLibrary("gnustl_shared");
    }

    public native String decrypt(String ticket, String srcFileName, String destFileName);
    public native void setSdcardPath(String sdcardPath);
}