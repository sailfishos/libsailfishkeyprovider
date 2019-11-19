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

#include "sailfishkeyprovider.h"
#include "sailfishkeyprovider_iniparser.h"

#include "base64ed.h"
#include "xored.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define STOREDKEYS_WRITABLE_DIRECTORY "%s/.local/share/system/privileged/Keys"
#define STOREDKEYS_WRITABLE_INIFILE "%s/.local/share/system/privileged/Keys/storedkeys.ini"
#define STOREDKEYS_STATIC_CONFIG_DIR "/usr/share/libsailfishkeyprovider/storedkeys.d/"
#define STOREDKEYS_STATIC_INIFILE "/usr/share/libsailfishkeyprovider/storedkeys.ini"
#define STOREDKEYS_ENCODINGSECTION "encoding"
#define STOREDKEYS_ENCODEDKEYSSECTION "encodedkeys"
#define STOREDKEYS_ENCODINGSECTION_SCHEME "scheme"
#define STOREDKEYS_ENCODINGSECTION_KEY "key"

/* Builds key of form: "first/second" */
char * build_ini_entry_key(const char *first, const char *second)
{
    int firstLength = strlen(first);
    int secondLength = strlen(second);
    char *retn = (char*)malloc(firstLength+secondLength+2);
    if (retn == NULL) {
        fprintf(stderr,
                "build_ini_entry_key: %s\n",
                "malloc failed");
        return NULL;
    }
    memcpy(retn, first, firstLength);
    memcpy(retn+firstLength+1, second, secondLength);
    retn[firstLength] = '/';
    retn[firstLength+secondLength+1] = '\0';
    return retn;
}

/* Try decode scheme from file*/
void try_read_decoding_scheme(const char *path, const char *psSchemeKey, const char *pSchemeKey, char** scheme)
{
    *scheme = SailfishKeyProvider_ini_read(
                path,
                STOREDKEYS_ENCODINGSECTION,
                psSchemeKey);
    if (*scheme == NULL) {
        /* try the fallback key. */
        *scheme = SailfishKeyProvider_ini_read(
                    path,
                    STOREDKEYS_ENCODINGSECTION,
                    pSchemeKey);
    }
}

/* Try decode key form file*/
void try_read_decoding_key(const char *path, const char *psKeyKey, const char *pKeyKey, char** key)
{
    *key = SailfishKeyProvider_ini_read(
                path,
                STOREDKEYS_ENCODINGSECTION,
                psKeyKey);
    if (*key == NULL) {
        /* try the fallback key. */
        *key = SailfishKeyProvider_ini_read(
                    path,
                    STOREDKEYS_ENCODINGSECTION,
                    pKeyKey);
    }
}

/* Try read key form file*/
void try_read_encoded_key(const char *path, const char *psKeyName, const char *pKeyName, char** key)
{
    *key = SailfishKeyProvider_ini_read(
                path,
                STOREDKEYS_ENCODEDKEYSSECTION,
                psKeyName);
    if (*key == NULL) {
        /* try the fallback key. */
        *key = SailfishKeyProvider_ini_read(
                    path,
                    STOREDKEYS_ENCODEDKEYSSECTION,
                    pKeyName);
    }
}

/*
 * Creates an encoded key given a \a keyValue, \a encodingScheme and
 * \a encodingKey.  Returns 0 on success, or -1 if any argument is
 * invalid or if encoding fails.  The caller owns the \a encodedKey
 * pointer and must free() it.
 *
 * Each argument must be a valid, null-terminated, Latin-1 or ASCII
 * C-string.  Currently, only "xor" is supported as an encoding scheme.
 */
int SailfishKeyProvider_encodeKey(
                    const char * keyValue,
                    const char * encodingScheme,
                    const char * encodingKey,
                    char ** encodedKey)
{
    if (encodedKey != NULL) {
        *encodedKey = NULL;
    }

    if (keyValue == NULL
            || encodingScheme == NULL
            || encodingKey == NULL
            || encodedKey == NULL) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_encodeKey(): invalid arguments");
        return -1;
    } else if ((strncmp(encodingScheme, "xor", 3) != 0)
            || (strlen(encodingScheme) != 3)) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_encodeKey(): invalid encoding scheme");
        return -1;
    } else if (strlen(keyValue) == 0
            || strlen(encodingScheme) == 0
            || strlen(encodingKey) == 0) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_encodeKey(): empty arguments");
        return -1;
    } else {
        /* XOR encoded is actually xor encoded then base64 encoded
           so that the returned string is a valid 7-bit ASCII c-string */
        size_t plain_text_size = strlen(keyValue);
        char *xor_encoded_value = SailfishKeyProvider_xor_encode(
            keyValue,
            plain_text_size,
            encodingKey,
            strlen(encodingKey));

        char *b64_xor_encoded_value = 0;
        size_t base64_encoded_size = SailfishKeyProvider_base64_encode(
                xor_encoded_value,
                plain_text_size,
                &b64_xor_encoded_value);

        free(xor_encoded_value);

        if (base64_encoded_size > 0) {
            *encodedKey = b64_xor_encoded_value;
            return 0; // Success.
        }

        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_encodeKey(): base64 encoding failed");
        return -1;
    }
}

/*
 * Creates a decoded key given a \a encodedKeyValue, \a decodingScheme and
 * \a decodingKey.  Returns 0 on success or -1 if any argument is invalid
 * or if encoding fails.  The caller owns the \a decodedKey pointer and
 * must free() it.
 *
 * Each argument must be a valid, null-terminated, Latin-1 or ASCII
 * C-string.  Currently, only "xor" is supported as an encoding scheme.
 */
int SailfishKeyProvider_decodeKey(
                    const char * encodedKeyValue,
                    const char * decodingScheme,
                    const char * decodingKey,
                    char ** decodedKey)
{
    if (decodedKey != NULL) {
        *decodedKey = NULL;
    }

    if (encodedKeyValue == NULL
            || decodingScheme == NULL
            || decodingKey == NULL
            || decodedKey == NULL) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_decodeKey(): invalid arguments");
        return -1;
    } else if ((strncmp(decodingScheme, "xor", 3) != 0)
            || (strlen(decodingScheme) != 3)) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_decodeKey(): invalid decoding scheme");
        return -1;
    } else if (strlen(encodedKeyValue) == 0
            || strlen(decodingScheme) == 0
            || strlen(decodingKey) == 0) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_decodeKey(): empty arguments");
        return -1;
    } else {
        /* XOR encoded is actually xor encoded then base64 encoded
           so that the returned string was a valid 7-bit ASCII c-string;
           thus to decode it, we first decode from base64 then decode XOR */
        size_t b64_encoded_value_size = strlen(encodedKeyValue);
        char *xor_encoded_value = 0;
        size_t xor_encoded_size = SailfishKeyProvider_base64_decode(
            encodedKeyValue,
            b64_encoded_value_size,
            &xor_encoded_value);

        if (xor_encoded_size > 0) {
            char *plain_text_value = SailfishKeyProvider_xor_decode(
                    xor_encoded_value,
                    xor_encoded_size,
                    decodingKey,
                    strlen(decodingKey));

            free(xor_encoded_value);
            *decodedKey = plain_text_value;
            return 0; // Success.
        }

        free(xor_encoded_value);
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_decodeKey(): base64 decoding failed");
        return -1;
    }
}

/*
 * Retrieves the decoded value of the key with the given \a keyName
 * for the given \a providerName valid for the given \a serviceName.
 * The value will be decoded using the decoding scheme and key
 * specified in the key storage ini file.
 *
 * Currently:
 *    - "xor" is the only valid decoding scheme
 *    - valid values for the \a keyName parameter depend on the provider
 *        - Twitter uses "consumer_key","consumer_secret"
 *        - Facebook uses "client_id"
 *        - Google uses "client_id","client_secret"
 *
 * Returns 0 if the key was retrieved successfully.
 * Returns -1 if the operation could not be completed due to invalid
 * arguments, or if an error occurs.
 * Returns 1 if no key with the given name exists.
 *
 * The caller owns the returned pointer and must free() it after use.
 *
 * Example:
 *
 *   char * buf;
 *   int success = SailfishKeyProvider_storedKey(
 *                            "facebook",
 *                            "facebook-sync",
 *                            "client_id",
 *                            &buf);
 *   // ... use client_id in OAuth2 request
 *   free(buf); // caller owns the returned buffer and must free().
 *
 */
int SailfishKeyProvider_storedKey(
                    const char * providerName,
                    const char * serviceName,
                    const char * keyName,
                    char ** storedKey)
{
    char writableIniFile[1024];

    /* "provider/service", "provider/service/scheme", "provider/service/key" */
    char *psKey = NULL;
    char *psSchemeKey = NULL;
    char *psKeyKey = NULL;

    /* "provider/scheme", "provider/key" - fallbacks */
    char *pSchemeKey = NULL;
    char *pKeyKey = NULL;

    /* "provider/service/keyName" */
    char *psKeyName;

    /* "provider/keyName" - fallback */
    char *pKeyName;

    /* outputs read from the ini file */
    char *decodingScheme = NULL;
    char *decodingKey = NULL;
    char *encodedKeyValue = NULL;

    /* whether the key came from the writable or static ini file */
    int whichIni = 0; /* 0 = writable, 1 = static */

    /* return value. */
    int retn = -1;

    if (storedKey != NULL) {
        *storedKey = NULL;
    }

    /* check parameters */
    if (providerName == NULL
            || serviceName == NULL
            || keyName == NULL
            || storedKey == NULL) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_storedKey(): error: null argument");
        return -1;
    }

    /* build ini entry keys */
    psKey = build_ini_entry_key(providerName, serviceName);
    psSchemeKey = build_ini_entry_key(psKey, STOREDKEYS_ENCODINGSECTION_SCHEME);
    psKeyKey = build_ini_entry_key(psKey, STOREDKEYS_ENCODINGSECTION_KEY);
    pSchemeKey = build_ini_entry_key(providerName, STOREDKEYS_ENCODINGSECTION_SCHEME);
    pKeyKey = build_ini_entry_key(providerName, STOREDKEYS_ENCODINGSECTION_KEY);
    psKeyName = build_ini_entry_key(psKey, keyName);
    pKeyName = build_ini_entry_key(providerName, keyName);

    snprintf(writableIniFile, sizeof(writableIniFile),
             STOREDKEYS_WRITABLE_INIFILE,
             getenv("HOME"));

    /* read the decoding scheme and the decoding key from .ini file */
    decodingScheme = SailfishKeyProvider_ini_read(
                                        writableIniFile,
                                        STOREDKEYS_ENCODINGSECTION,
                                        psSchemeKey);
    if (decodingScheme == NULL) {
        /* try the fallback key. */
        decodingScheme = SailfishKeyProvider_ini_read(
                                        writableIniFile,
                                        STOREDKEYS_ENCODINGSECTION,
                                        pSchemeKey);
    }

    decodingKey = SailfishKeyProvider_ini_read(
                                        writableIniFile,
                                        STOREDKEYS_ENCODINGSECTION,
                                        psKeyKey);
    if (decodingKey == NULL) {
        /* try the fallback key. */
        decodingKey = SailfishKeyProvider_ini_read(
                                        writableIniFile,
                                        STOREDKEYS_ENCODINGSECTION,
                                        pKeyKey);
    }

    if (decodingScheme == NULL || decodingKey == NULL) {
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir (STOREDKEYS_STATIC_CONFIG_DIR)) != NULL) {
            /* print all the files and directories within directory */
            while ((ent = readdir (dir)) != NULL) {
                char path[PATH_MAX] = "";
                strcat(path, STOREDKEYS_STATIC_CONFIG_DIR);
                strcat(path, ent->d_name);

                try_read_decoding_scheme(path, psSchemeKey, pSchemeKey, &decodingScheme);
                try_read_decoding_key(path, psKeyKey, pKeyKey, &decodingKey);

                if (decodingScheme || decodingKey) {
                    break;
                }
            }
            closedir (dir);
        }
    }

    if (decodingScheme == NULL || decodingKey == NULL) {
        /* even the fallback keys were empty.  Try reading from the static .ini file */
        decodingScheme = SailfishKeyProvider_ini_read(
                                            STOREDKEYS_STATIC_INIFILE,
                                            STOREDKEYS_ENCODINGSECTION,
                                            psSchemeKey);
        if (decodingScheme == NULL) {
            /* try the fallback key. */
            decodingScheme = SailfishKeyProvider_ini_read(
                                            STOREDKEYS_STATIC_INIFILE,
                                            STOREDKEYS_ENCODINGSECTION,
                                            pSchemeKey);
        }

        decodingKey = SailfishKeyProvider_ini_read(
                                            STOREDKEYS_STATIC_INIFILE,
                                            STOREDKEYS_ENCODINGSECTION,
                                            psKeyKey);
        if (decodingKey == NULL) {
            /* try the fallback key. */
            decodingKey = SailfishKeyProvider_ini_read(
                                            STOREDKEYS_STATIC_INIFILE,
                                            STOREDKEYS_ENCODINGSECTION,
                                            pKeyKey);
        }

        if (decodingScheme == NULL || decodingKey == NULL) {
            /* Not found in static ini file either.  Error. */
            free(psKey);
            free(psSchemeKey);
            free(psKeyKey);
            free(pSchemeKey);
            free(pKeyKey);
            free(psKeyName);
            free(pKeyName);
            free(decodingScheme);
            free(decodingKey);
            fprintf(stderr,
                    "SailfishKeyProvider_storedKey(): %s\n",
                    "error: no scheme or key found for provider/service");
            return 1;
        } else {
            whichIni = 1; /* found in the static ini file */
        }
    }

    free(psKey);
    free(psSchemeKey);
    free(psKeyKey);
    free(pSchemeKey);
    free(pKeyKey);

    /* now read the encoded key value for the given keyName from the ini */
    encodedKeyValue = SailfishKeyProvider_ini_read(
                                        whichIni ? STOREDKEYS_STATIC_INIFILE : writableIniFile,
                                        STOREDKEYS_ENCODEDKEYSSECTION,
                                        psKeyName);
    if (encodedKeyValue == NULL) {
        /* try the fallback key. */
        encodedKeyValue = SailfishKeyProvider_ini_read(
                                        whichIni ? STOREDKEYS_STATIC_INIFILE : writableIniFile,
                                        STOREDKEYS_ENCODEDKEYSSECTION,
                                        pKeyName);
    }

    if (encodedKeyValue == NULL) {
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir (STOREDKEYS_STATIC_CONFIG_DIR)) != NULL) {
            /* print all the files and directories within directory */
            while ((ent = readdir (dir)) != NULL) {
                char path[PATH_MAX] = "";
                strcat(path, STOREDKEYS_STATIC_CONFIG_DIR);
                strcat(path, ent->d_name);

                try_read_encoded_key(path, psKeyName, pKeyName, &encodedKeyValue);

                if (encodedKeyValue) {
                    break;
                }
            }
            closedir (dir);
        }
    }

    if (encodedKeyValue == NULL) {
        /* even the fallback keys were empty */
        free(psKeyName);
        free(pKeyName);
        free(decodingScheme);
        free(decodingKey);
        fprintf(stderr,
                "SailfishKeyProvider_storedKey():%s\n",
                "error: no such stored key exists");
        return 1;
    }

    /* we have the encoded key value.  Decode and return. */
    if (strlen(encodedKeyValue) == 0) {
        fprintf(stderr,
                "SailfishKeyProvider_storedKey(): %s\n",
                "error: empty key value");
    } else {
        /* attempt to decode it with the given scheme/key */
        retn = SailfishKeyProvider_decodeKey(encodedKeyValue,
                                             decodingScheme,
                                             decodingKey,
                                             storedKey);
    }

    free(psKeyName);
    free(pKeyName);
    free(decodingScheme);
    free(decodingKey);
    free(encodedKeyValue);
    return retn;
}

/*
    Stores the given \a encodedValue to the key storage ini file
    for the given \a providerName, \a serviceName, \a encodedKeyName
    tuple.  The encoding scheme and encoding key associated with the
    value are stored in the key storage ini file also.

    Returns zero on success, -1 on failure.
*/
int SailfishKeyProvider_storeKey(
                    const char * providerName,
                    const char * serviceName,
                    const char * encodedKeyName,
                    const char * encodedValue,
                    const char * encodingScheme,
                    const char * encodingKey)
{
    int retn = 0;
    char *psKey = NULL;
    char *psSchemeKey = NULL;
    char *psKeyKey = NULL;
    char *pskKey = NULL;
    char writableDirectory[1024];
    char writableIniFile[1024];

    if (providerName == NULL
            || serviceName == NULL
            || encodedKeyName == NULL
            || encodedValue == NULL
            || encodingScheme == NULL
            || encodingKey == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_storeKey(): %s\n",
                "error: invalid parameters");
        return -1;
    }

    psKey = build_ini_entry_key(providerName, serviceName);
    psSchemeKey = build_ini_entry_key(psKey, STOREDKEYS_ENCODINGSECTION_SCHEME);
    psKeyKey = build_ini_entry_key(psKey, STOREDKEYS_ENCODINGSECTION_KEY);
    pskKey = build_ini_entry_key(psKey, encodedKeyName);

    snprintf(writableDirectory, sizeof(writableDirectory),
             STOREDKEYS_WRITABLE_DIRECTORY,
             getenv("HOME"));
    snprintf(writableIniFile, sizeof(writableIniFile),
             STOREDKEYS_WRITABLE_INIFILE,
             getenv("HOME"));

    /* write the encoding scheme */
    retn = SailfishKeyProvider_ini_write(
                    writableDirectory,
                    writableIniFile,
                    STOREDKEYS_ENCODINGSECTION,
                    psSchemeKey,
                    encodingScheme);
    if (retn == -1) {
        fprintf(stderr,
                "SailfishKeyProvider_storeKey(): %s\n",
                "error: unable to write scheme");
        goto free_keys_and_return;
    }

    /* write the encoding key */
    retn = SailfishKeyProvider_ini_write(
                    writableDirectory,
                    writableIniFile,
                    STOREDKEYS_ENCODINGSECTION,
                    psKeyKey,
                    encodingKey);
    if (retn == -1) {
        fprintf(stderr,
                "SailfishKeyProvider_storeKey(): %s\n",
                "error: unable to write encoding key");
        goto free_keys_and_return;
    }

    /* write the encoded key value */
    retn = SailfishKeyProvider_ini_write(
                    writableDirectory,
                    writableIniFile,
                    STOREDKEYS_ENCODEDKEYSSECTION,
                    pskKey,
                    encodedValue);
    if (retn == -1) {
        fprintf(stderr,
                "SailfishKeyProvider_storeKey(): %s\n",
                "error: unable to write encoding key");
        goto free_keys_and_return;
    }

free_keys_and_return:
    free(psKey);
    free(psSchemeKey);
    free(psKeyKey);
    free(pskKey);
    return retn;
}
