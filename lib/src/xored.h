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

#ifndef XORED_H
#define XORED_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
char * SailfishKeyProvider_xor_encode(
                    const char * plainTextValue,
                    size_t ptvLen,
                    const char * encodingKey,
                    size_t ekLen);

char * SailfishKeyProvider_xor_decode(
                    const char * xorEncodedValue,
                    size_t xevLen,
                    const char * decodingKey,
                    size_t dkLen);
#ifdef __cplusplus
}
#endif

#endif /* XORED_H */
