/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Chris Adams <chris.adams@jollamobile.com>
** All rights reserved.
**
** You may use this file under the terms of the GNU Lesser General
** Public License version 2.1 as published by the Free Software Foundation
** and appearing in the file license.lgpl included in the packaging
** of this file.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file license.lgpl included in the packaging
** of this file.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Lesser General Public License for more details.
**
****************************************************************************/

#include "xored.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/*
    Encodes the given \a plainTextValue via bytewise XOR with the
    given \a encodingKey.  The returned buffer will have the same
    length as the given \a ptvLen.  The caller owns the returned
    buffer.
*/
char * SailfishKeyProvider_xor_encode(
                    const char * plainTextValue,
                    size_t ptvLen,
                    const char * encodingKey,
                    size_t ekLen)
{
    /* this function assumes that the input is xor encoded only. */
    size_t i = 0;
    size_t keyIdx = ekLen;
    char * retn = (char *)malloc(ptvLen+1);
    if (retn == NULL) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_xor_encode: malloc failed");
        return NULL;
    }
    memset(retn, 0, ptvLen+1);

    for (i = 0; i < ptvLen; ++i) {
        keyIdx += 1;
        if (keyIdx >= ekLen) {
            keyIdx = 0;
        }

        retn[i] = plainTextValue[i] ^ encodingKey[keyIdx];
    }

    return retn; /* caller takes ownership and must free() */
}

/*
    \see SailfishKeyProvider_xor_encode()
*/
char * SailfishKeyProvider_xor_decode(
                    const char * xorEncodedValue,
                    size_t xevLen,
                    const char * decodingKey,
                    size_t dkLen)
{
    return SailfishKeyProvider_xor_encode(
                xorEncodedValue, xevLen, decodingKey, dkLen);
}
