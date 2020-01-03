#pragma once 

/**
 * base64编码
 * 成功返回pDst长度,失败返回-1
 */
int EncodeBase64(const unsigned char* pSrc, unsigned char* pDst, int nSrcLen);

/**
 * base64解码
 * 成功返回pDst长度,失败返回-1
 */
int DecodeBase64(const unsigned char* pSrc, unsigned char* pDst, int nSrcLen);
