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

#ifndef SAILFISHKEYPROVIDER_H
#define SAILFISHKEYPROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif
int SailfishKeyProvider_storeKey(
                    const char * providerName,
                    const char * serviceName,
                    const char * encodedKeyName,
                    const char * encodedValue,
                    const char * encodingScheme,
                    const char * encodingKey);

int SailfishKeyProvider_storedKey(
                    const char * providerName,
                    const char * serviceName,
                    const char * encodedKeyName,
                    char ** storedKey);

int SailfishKeyProvider_decodeKey(
                    const char * encodedKeyValue,
                    const char * decodingScheme,
                    const char * decodingKey,
                    char ** decodedKey);

int SailfishKeyProvider_encodeKey(
                    const char * keyValue,
                    const char * encodingScheme,
                    const char * encodingKey,
                    char ** encodedKey);
#ifdef __cplusplus
}
#endif

#endif /* SAILFISHKEYPROVIDER_H */
