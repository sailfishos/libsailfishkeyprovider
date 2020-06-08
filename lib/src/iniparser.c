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

/*
    Simple .ini file parser / writer

    Note that this code isn't intended to be particularly performant,
    but rather simple to understand and robust.

    It does not handle repeated sections.
*/

#include "sailfishkeyprovider_iniparser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define MAX_LINESIZE 4095

#define INFO_OK       0 /* succeeded */
#define INFO_SKIPPED  1 /* whitespace only, or comment line */
#define INFO_EOF      2 /* empty stream */
#define INFO_MALLOC   3 /* malloc failed */
#define INFO_LINESIZE 4 /* line too long */
#define INFO_INVALID  5 /* invalid line format */

static const char * error_messages[] = {
    "ok",
    "skipped",
    "reached end of file",
    "malloc failed",
    "line too long",
    "invalid line"
};

/* --------------------------------------------------------- */

void free_list_and_content(char **list)
{
    int i = 0;
    if (list != NULL) {
        for (i = 0; list[i] != NULL; ++i) {
            free(list[i]);
        }
        free(list);
        list = NULL;
    }
}

void free_array_and_content(char **array, int count)
{
    int i = 0;
    if (array != NULL) {
        for (i = 0; i < count; ++i) {
            free(array[i]);
        }
        free(array);
        array = NULL;
    }
}

char ** split_string_into_array(
        const char * string,
        const char * separator,
        int * count)
{
    size_t allocated = 1;
    char **tokens = NULL;
    char *tofree = NULL;
    char *remainder = NULL;
    char *token = NULL;

    if (string == NULL || separator == NULL) {
        return NULL;
    }

    tokens = calloc(allocated, sizeof(char*));
    if (tokens == NULL) {
        return NULL;
    }

    tofree = strdup(string);
    if (tofree == NULL) {
        free_array_and_content(tokens, allocated);
        return NULL;
    }

    *count = 0;
    remainder = tofree;
    while ((token = strsep(&remainder, separator)) != NULL) {
        if (*count == (int)allocated) {
            allocated += 1;
            tokens = realloc(tokens, allocated * sizeof(char*));
        }
        tokens[*count] = strdup(token);
        *count += 1;
    }

    if (*count == 0) {
        free(tokens);
        tokens = NULL;
    }

    free(tofree);
    return tokens;
}

char * ini_read_line(FILE *stream, int *info)
{
    int i = 0, j = 0;
    int currChar = 0;
    int count = 0;
    char *retn = NULL, *buf = NULL;

    buf = (char*)malloc(MAX_LINESIZE + 1);
    if (buf == NULL) {
        *info = INFO_MALLOC;
        return NULL;
    }
    memset(buf, 0, MAX_LINESIZE + 1);

    /* check that we're not at EOF */
    currChar = getc(stream);
    if (currChar == EOF) {
        *info = INFO_EOF;
        free(buf);
        return NULL;
    }

    /* read entire line into buf. */
    while (currChar != '\n' && currChar != EOF) {
        count += 1;
        if (count >= MAX_LINESIZE) {
            *info = INFO_LINESIZE;
            free(buf);
            return NULL;
        }

        buf[i++] = currChar;
        currChar = getc(stream);
    }

    /* if the line is whitespace-only or comment-only, skip it */
    if (count == 0) {
        *info = INFO_SKIPPED;
        free(buf);
        return NULL;
    }
    for (i = 0; i < count; ++i) {
        currChar = buf[i];
        if ((isspace(currChar) && i == (count-1)) /* wsp-only line */
                    || currChar == ';') {         /* comment  line */
            *info = INFO_SKIPPED;
            free(buf);
            return NULL;
        } else if (!isspace(currChar)) {
            break;
        }
    }

    /* we have a non-skippable line. */
    retn = (char*)malloc(count + 1);
    if (retn == NULL) {
        *info = INFO_MALLOC;
        free(buf);
        return NULL;
    }
    memset(retn, 0, count + 1);

    /* strip out leading whitespace */
    for (i = 0; i < count; ++i) {
        currChar = buf[i];
        if (!isspace(currChar)) {
            break;
        }
    }

    /* copy out "real" data */
    for (; i < count; ++i) {
        currChar = buf[i];
        if (isspace(currChar) && (i < (count-1) && buf[i+1] == ';')) {
            /* the rest of the line is a comment */
            break;
        } else {
            /* copy the character to the output */
            retn[j] = currChar;
            j++;
        }
    }

    /* strip out trailing whitespace */
    for (j = j-1; j > 0; --j) {
        if (isspace(retn[j])) {
            retn[j] = '\0';
        } else {
            break;
        }
    }

    /* return the result buffer */
    *info = INFO_OK;
    free(buf);
    return retn;
}

void ini_parse_parts(const char *line, int *info, char **section, char **key, char **value)
{
    size_t i = 0;
    size_t lineLength = strlen(line);
    *section = NULL;
    *key = NULL;
    *value = NULL;
    if (lineLength <= 2) {
        *info = INFO_INVALID;
        return;
    }

    /* could be a section like "[sectionName]" */
    if (line[0] == '[' && line[lineLength-1] == ']') {
        *section = (char*)malloc(lineLength-1);
        if (*section == NULL) {
            *info = INFO_MALLOC;
            return;
        }
        (*section)[lineLength-2] = '\0';
        memcpy(*section, (line+1), lineLength-2);
        *info = INFO_OK;
        return;
    }

    /* otherwise, will be of the form "some/key=someValue".
       note that the value can be empty, ie: "some/key=". */
    for (i = 1; i < lineLength; ++i) {
        if (line[i] == '=') {
            *key = (char*)malloc(i+1);
            if (*key == NULL) {
                *info = INFO_MALLOC;
                return;
            }
            (*key)[i] = '\0';
            memcpy(*key, line, i);

            *value = (char*)malloc(lineLength - i);
            if (*value == NULL) {
                free(*key);
                *key = NULL;
                *info = INFO_MALLOC;
                return;
            }
            (*value)[lineLength - i - 1] = '\0';
            memcpy(*value, (line+i+1), lineLength - i - 1);

            *info = INFO_OK;
            return;
        }
    }

    /* invalid line */
    *info = INFO_INVALID;
    return;
}

char ** ini_read_sections(FILE *stream, int *info)
{
    char *line = NULL;
    char *readSection = NULL;
    char *readKey = NULL;
    char *readValue = NULL;

    int retnSize = 1;
    char ** retn = (char**)malloc(sizeof(char*));
    if (retn == NULL) {
        *info = INFO_MALLOC;
        return NULL;
    }
    retn[0] = NULL;

    rewind(stream);

    while (1) {
        line = ini_read_line(stream, info);
        if (*info == INFO_OK) {
            ini_parse_parts(line, info, &readSection, &readKey, &readValue);
            if (*info != INFO_OK) {
                break;
            }

            if (readSection != NULL) {
                /* found a section - append it */
                char **newList = (char**)realloc(retn,
                        (retnSize + 1) * sizeof(char*));
                if (newList == NULL) {
                    free(readSection);
                    *info = INFO_MALLOC;
                    break;
                }
                retn = newList;
                retn[retnSize-1] = readSection;
                retn[retnSize] = NULL;
                retnSize = retnSize + 1;

                readSection = NULL;
            } else {
                /* this is a key/value pair within a section */
                if (readKey)     free(readKey);
                if (readValue)   free(readValue);
                readKey = NULL;
                readValue = NULL;
            }

            free(line);
            line = NULL;
            continue;
        } else if (*info == INFO_SKIPPED) {
            free(line);
            line = NULL;
            continue;
        } else if (*info == INFO_EOF) {
            /* we're finished reading the file.  return our list */
            *info = INFO_OK;
            free(line);
            line = NULL;
            return retn;
        } else {
            break;
        }
    }

    /* clean up and return null */
    free_list_and_content(retn);
    free(line);
    return NULL;
}

char ** ini_read_keys(FILE *stream, const char *section, int *info)
{
    char *line = NULL;
    char *readSection = NULL;
    char *readKey = NULL;
    char *readValue = NULL;
    int found = 0;

    int retnSize = 1;
    char ** retn = (char**)malloc(sizeof(char*));
    retn[0] = NULL;

    rewind(stream);

    /* if they've specified a section, seek to it */
    while (section != NULL && found == 0) {
        line = ini_read_line(stream, info);
        if (*info == INFO_OK) {
            ini_parse_parts(line, info, &readSection, &readKey, &readValue);
            if (*info != INFO_OK) {
                goto cleanup_and_return_null;
            }

            if (readSection != NULL && strcmp(section, readSection) == 0) {
                /* found the section! */
                found = 1;
                free(readSection);
                free(line);
                readSection = NULL;
                line = NULL;
                break;
            } else {
                /* did not find the section */
                if (readSection) free(readSection);
                if (readKey)     free(readKey);
                if (readValue)   free(readValue);
                free(line);
                readSection = NULL;
                readKey = NULL;
                readValue = NULL;
                line = NULL;
                continue;
            }
        } else if (*info == INFO_SKIPPED) {
            free(line);
            line = NULL;
            continue;
        } else if (*info == INFO_EOF) {
            free(line);
            line = NULL;
            break;
        } else {
            goto cleanup_and_return_null;
        }
    }

    /* if it hasn't been found, it doesn't exist */
    if (found == 0) {
        return retn; /* return empty list of keys */
    }

    /* now read the keys for the section. */
    while (1) {
        line = ini_read_line(stream, info);
        if (*info == INFO_OK) {
            ini_parse_parts(line, info, &readSection, &readKey, &readValue);
            if (*info != INFO_OK) {
                goto cleanup_and_return_null;
            }

            if (readSection != NULL) {
                /* we've changed sections, so we're finished. */
                free(line);
                free(readSection);
                free(readKey);
                free(readValue);
                return retn;
            } else {
                /* this key is in this section */
                char **newList = (char**)realloc(retn,
                        (retnSize + 1) * sizeof(char*));
                if (newList == NULL) {
                    free(readKey);
                    free(readValue);
                    *info = INFO_MALLOC;
                    break;
                }
                retn = newList;
                retn[retnSize-1] = readKey;
                retn[retnSize] = NULL;
                retnSize = retnSize + 1;

                free(readValue);
                free(line);
                line = NULL;
                continue;
            }
        } else if (*info == INFO_SKIPPED) {
            /* skipping a comment/wsp line */
            free(line);
            line = NULL;
            continue;
        } else if (*info == INFO_EOF) {
            /* reached the end of the file. */
            *info = INFO_OK;
            free(line);
            return retn;
        } else {
            goto cleanup_and_return_null;
        }
    }

cleanup_and_return_null:
    free_list_and_content(retn);
    free(line);
    return NULL;
}

char * ini_read_value(
                    FILE *stream,
                    const char * section,
                    const char * key,
                    int *info)
{
    char *line = NULL;
    char *readSection = NULL;
    char *readKey = NULL;
    char *readValue = NULL;
    int found = 0;

    rewind(stream);

    /* if they've specified a section, seek to it */
    while (section != NULL && found == 0) {
        line = ini_read_line(stream, info);
        if (*info == INFO_OK) {
            ini_parse_parts(line, info, &readSection, &readKey, &readValue);
            if (*info != INFO_OK) {
                free(line);
                return NULL;
            }

            if (readSection != NULL && strcmp(section, readSection) == 0) {
                /* found the section! */
                found = 1;
                free(readSection);
                free(line);
                readSection = NULL;
                line = NULL;
                break;
            } else {
                /* did not find the section */
                if (readSection) free(readSection);
                if (readKey)     free(readKey);
                if (readValue)   free(readValue);
                free(line);
                readSection = NULL;
                readKey = NULL;
                readValue = NULL;
                line = NULL;
                continue;
            }
        } else if (*info == INFO_SKIPPED) {
            free(line);
            line = NULL;
            continue;
        } else if (*info == INFO_EOF) {
            free(line);
            line = NULL;
            break;
        } else {
            free(line);
            return NULL;
        }
    }

    /* if it hasn't been found, it doesn't exist */
    if (found == 0) {
        return NULL;
    }

    /* we should have moved the cursor to the section; find the key/value */
    while (1) {
        line = ini_read_line(stream, info);
        if (*info == INFO_OK) {
            ini_parse_parts(line, info, &readSection, &readKey, &readValue);
            if (*info != INFO_OK) {
                free(line);
                return NULL;
            }

            if (readKey != NULL && strcmp(key, readKey) == 0) {
                /* found the key/value! */
                free(readKey);
                free(line);
                return readValue;
            } else {
                /* did not find the key/value */
                if (readSection) free(readSection);
                if (readKey)     free(readKey);
                if (readValue)   free(readValue);
                free(line);
                line = NULL;
                readKey = NULL;
                readValue = NULL;
                if (readSection != NULL) {
                    break; /* changed section; key/value mustn't exist */
                }

                /* continue searching the section for the key/value */
                readSection = NULL;
                continue;
            }
        } else if (*info == INFO_SKIPPED) {
            free(line);
            line = NULL;
            continue;
        } else {
            free(line);
            return NULL;
        }
    }

    return NULL;
}

char ** SailfishKeyProvider_ini_sections(
                    const char * filename)
{
    FILE *stream = NULL;
    char **existingSections = NULL;
    int info = INFO_OK;

    if (filename == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_sections: %s\n",
                "invalid parameters");
        return NULL;
    }

    existingSections = ini_read_sections(stream, &info);
    if (info != INFO_OK) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_sections: %s\n",
                error_messages[info]);
    }

    if (fclose(stream) != 0) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_sections: %s\n",
                "error closing ini file");
    }

    return existingSections;
}

char ** SailfishKeyProvider_ini_keys(
                    const char * filename,
                    const char * section)
{
    FILE *stream = NULL;
    char **existingKeys = NULL;
    int info = INFO_OK;

    if (filename == NULL || section == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_keys: %s\n",
                "invalid parameters");
        return NULL;
    }

    existingKeys = ini_read_keys(stream, section, &info);
    if (info != INFO_OK) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_keys: %s\n",
                error_messages[info]);
    }

    if (fclose(stream) != 0) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_keys: %s\n",
                "error closing ini file");
    }

    return existingKeys;
}

char * SailfishKeyProvider_ini_read(
                    const char * filename,
                    const char * section,
                    const char * key)
{
    char *retn = NULL;
    FILE *stream = NULL;
    int info = INFO_OK;

    if (filename == NULL || key == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_read: %s\n",
                "invalid parameters");
        return NULL;
    }

    stream = fopen(filename, "r");
    if (stream == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_read: %s\n",
                "unable to open file");
        return NULL;
    }

    retn = ini_read_value(stream, section, key, &info);
    if (info != INFO_OK && info != INFO_EOF) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_read: %s\n",
                error_messages[info]);
    }

    if (fclose(stream) != 0) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_read: %s\n",
                "error closing ini file");
    }
    return retn;
}

char ** SailfishKeyProvider_ini_read_multiple(
                    const char * filename,
                    const char * section,
                    const char * keys,
                    const char * separator)
{
    FILE *stream = NULL;
    int info = INFO_OK;
    int k = 0;
    int numKeys = 0;
    size_t allocated = 1;
    char **splitKeys = NULL;
    char **retnValues = NULL;

    if (filename == NULL || keys == NULL || separator == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_read_multiple: %s\n",
                "invalid parameters");
        return NULL;
    }

    splitKeys = split_string_into_array(keys, separator, &numKeys);
    if (splitKeys == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_read_multiple: %s\n",
                "unable to parse keys parameter");
        return NULL;
    }

    stream = fopen(filename, "r");
    if (stream == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_read_multiple: %s\n",
                "unable to open file");
        free_array_and_content(splitKeys, numKeys);
        return NULL;
    }

    retnValues = calloc(allocated, sizeof(char*));
    if (retnValues == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_read_multiple: %s\n",
                "unable to allocate values array");
    } else {
        for (k = 0; k < numKeys; ++k) {
            if (k == (int)allocated) {
                allocated += 1;
                retnValues = realloc(retnValues, allocated * sizeof(char*));
            }
            retnValues[k] = ini_read_value(stream, section, splitKeys[k], &info);
            if (info != INFO_OK) {
                fprintf(stderr,
                        "SailfishKeyProvider_ini_read_multiple: %s\n",
                        error_messages[info]);
            }
        }
    }

    if (fclose(stream) != 0) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_read_multiple: %s\n",
                "error closing ini file");
    }
    free_array_and_content(splitKeys, numKeys);
    return retnValues;
}

#define APPEND_NEWLINE_TO_BUF(buf)                                         \
    do {                                                                   \
        int startOfNewSpace = strlen(buf);                                 \
        int sizeOfNewSpace = 2; /* \n \0 */                                \
        char *rbuf = (char*)realloc(buf, startOfNewSpace + sizeOfNewSpace);\
        if (rbuf == NULL) {                                                \
            goto cleanup_and_return_malloc_fail;                           \
        }                                                                  \
        buf = rbuf;                                                        \
        sprintf(buf+startOfNewSpace, "\n");                                \
        buf[startOfNewSpace+sizeOfNewSpace-1] = '\0';                      \
    } while (0)

#define APPEND_COMMENT_TO_BUF(comment, buf)                                \
    do {                                                                   \
        int startOfNewSpace = strlen(buf);                                 \
        int sizeOfNewSpace = strlen(comment) + 3; /* ; \n \0 */            \
        char *rbuf = (char*)realloc(buf, startOfNewSpace + sizeOfNewSpace);\
        if (rbuf == NULL) {                                                \
            goto cleanup_and_return_malloc_fail;                           \
        }                                                                  \
        buf = rbuf;                                                        \
        sprintf(buf+startOfNewSpace, ";%s\n", comment);                    \
        buf[startOfNewSpace+sizeOfNewSpace-1] = '\0';                      \
    } while (0)

#define APPEND_SECTION_TO_BUF(sectionName, buf)                            \
    do {                                                                   \
        int startOfNewSpace = strlen(buf);                                 \
        int sizeOfNewSpace = strlen(sectionName) + 4; /* [ ] \n \0 */      \
        char *rbuf = (char*)realloc(buf, startOfNewSpace + sizeOfNewSpace);\
        if (rbuf == NULL) {                                                \
            goto cleanup_and_return_malloc_fail;                           \
        }                                                                  \
        buf = rbuf;                                                        \
        sprintf(buf+startOfNewSpace, "[%s]\n", sectionName);               \
        buf[startOfNewSpace+sizeOfNewSpace-1] = '\0';                      \
    } while (0)

#define APPEND_KEYVAL_TO_BUF(key, val, buf)                                \
    do {                                                                   \
        int startOfNewSpace = strlen(buf);                                 \
        int sizeOfNewSpace = strlen(key) + 1 + strlen(val) + 1 + 1;        \
        char *rbuf = (char*)realloc(buf, startOfNewSpace + sizeOfNewSpace);\
        if (rbuf == NULL) {                                                \
            goto cleanup_and_return_malloc_fail;                           \
        }                                                                  \
        buf = rbuf;                                                        \
        sprintf(buf+startOfNewSpace, "%s=%s\n", key, val);                 \
        buf[startOfNewSpace+sizeOfNewSpace-1] = '\0';                      \
    } while (0)

int SailfishKeyProvider_ini_write_multiple_impl(
                    const char * directory,
                    const char * filename, /* must contain full path */
                    const char * section,
                    const char * keys,     /* separator-separated list of keys */
                    const char * values,   /* separator-separated list of values */
                    const char * separator)/* if null, assume single key/value */
{
    int createFd = -1;
    int info = INFO_OK;
    int i = 0, j = 0, k = 0;
    int numKeys = 0;
    int numValues = 0;
    int sectionFound = 0, thisSection = 0;
    int keyFound = 0;
    FILE *stream = NULL;
    char **existingKeys = NULL;
    char *currKey = NULL;
    char **existingSections = NULL;
    char *currSection = NULL;
    char *newFileData = NULL;
    char **splitKeys = NULL;
    char **splitValues = NULL;

    if (filename == NULL || section == NULL || keys == NULL || values == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_write_multiple: %s\n",
                "invalid parameters");
        return -1;
    }

    if (separator != NULL) {
        splitKeys = split_string_into_array(keys, separator, &numKeys);
        splitValues = split_string_into_array(values, separator, &numValues);
        if (numKeys != numValues || numKeys <= 0) {
            fprintf(stderr,
                    "SailfishKeyProvider_ini_write_multiple: %s\n",
                    "unable to parse keys and values, or count mismatch");
            goto cleanup_and_return_fail;
        }
    }

    /* allocate a buffer to which we'll write file data */
    newFileData = (char*)malloc(1);
    if (newFileData == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_write_multiple: %s\n",
                "unable to allocate file data buffer");
        goto cleanup_and_return_fail;
    }
    newFileData[0] = '\0';

    /* first, create the directory and file if it doesn't exist. */
    if (mkdir(directory, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP) < 0) {
        if (errno != EEXIST) {
            /* The directory does not exist, and we couldn't create it */
            fprintf(stderr,
                    "SailfishKeyProvider_ini_write_multiple: %s\n",
                    "unable to create writable ini directory");
            goto cleanup_and_return_fail;
        }
        /* The directory already exists; continue as we were */
    }

    createFd = open(filename, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (createFd < 0) {
        if (errno != EEXIST) {
            /* The file does not exist, but we cannot create it.
             * This is probably a permissions error */
            fprintf(stderr,
                    "SailfishKeyProvider_ini_write_multiple: %s\n",
                    "unable to create writable ini file");
            goto cleanup_and_return_fail;
        }
        /* The file exists, we didn't need to create it */
    } else {
        /* having created the file, immediately close it */
        if (close(createFd) != 0) {
            fprintf(stderr,
                    "SailfishKeyProvider_ini_write_multiple: %s\n",
                    "unable to close writable ini file after creating it");
            goto cleanup_and_return_fail;
        }
    }

    /* open the file in read mode.  We read in the entire file,
       overwrite or add the appropriate key/value, and then close
       the file so that we can reopen in write mode later. */
    stream = fopen(filename, "r");
    if (stream == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_write_multiple: %s\n",
                "unable to open file for read");
        goto cleanup_and_return_fail;
    }

    existingSections = ini_read_sections(stream, &info);
    if (info != INFO_OK) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_write_multiple: %s\n",
                "unable to read existing sections");
        if (fclose(stream) != 0) {
            fprintf(stderr,
                    "SailfishKeyProvider_ini_write_multiple: %s\n",
                    "error closing ini file");
        }
        goto cleanup_and_return_fail;
    }

    currSection = existingSections[0];
    for (i = 1; currSection != NULL; ++i) {
        APPEND_SECTION_TO_BUF(currSection, newFileData);
        thisSection = 0;
        if (strcmp(currSection, section) == 0) {
            /* key/value should be inserted into this section. */
            sectionFound = 1;
            thisSection = 1;
        }

        /* enumerate the keys in this section */
        existingKeys = ini_read_keys(stream, currSection, &info);
        if (info != INFO_OK) {
            fprintf(stderr,
                    "SailfishKeyProvider_ini_write_multiple: %s\n",
                    "unable to read existing keys");
            if (fclose(stream) != 0) {
                fprintf(stderr,
                        "SailfishKeyProvider_ini_write_multiple: %s\n",
                        "error closing ini file");
            }
            goto cleanup_and_return_fail;
        }

        /* for each old key/value either overwrite the key/value with the new one,
         * or just append that old key/value if no new key/value is given for it */
        currKey = existingKeys[0];
        keyFound = 0;
        for (j = 1; currKey != NULL; ++j) {
            if (separator != NULL) {
                /* multiple keys/values mode, the keys+values args are arrays */
                for (k = 0; k != numKeys; ++k) {
                    if (thisSection == 1 && strcmp(currKey, splitKeys[k]) == 0) {
                        /* we need to replace this key/value with the new value */
                        keyFound = 1;
                        APPEND_KEYVAL_TO_BUF(currKey, splitValues[k], newFileData);
                    }
                }
                if (!keyFound) {
                    /* just append this pre-existing key/value */
                    char *currVal = ini_read_value(stream, currSection, currKey, &info);
                    APPEND_KEYVAL_TO_BUF(currKey, currVal, newFileData);
                    free(currVal);
                }
            } else {
                /* Single key/value mode, the keys+values args are single values */
                if (thisSection == 1 && strcmp(currKey, keys) == 0) {
                    /* we need to replace this key/value with the new value */
                    keyFound = 1;
                    APPEND_KEYVAL_TO_BUF(currKey, values, newFileData);
                } else {
                    /* just append this pre-existing key/value */
                    char *currVal = ini_read_value(stream, currSection, currKey, &info);
                    APPEND_KEYVAL_TO_BUF(currKey, currVal, newFileData);
                    free(currVal);
                }
            }
            currKey = existingKeys[j];
            keyFound = 0;
        }

        /* then find any new key/value which isn't represented in the existingKeys
         * and append it to the .ini file */
        if (thisSection == 1) {
            /* reached the end of the section and haven't found this new key/value; append it. */
            if (separator != NULL) {
                /* multiple key/values given, check whether each one exists */
                for (k = 0; k != numKeys; ++k) {
                    keyFound = 0;
                    for (j = 0; existingKeys[j] != NULL; ++j) {
                        if (strcmp(existingKeys[j], splitKeys[k]) == 0) {
                            keyFound = 1;
                            break;
                        }
                    }
                    if (!keyFound) {
                        APPEND_KEYVAL_TO_BUF(splitKeys[k], splitValues[k], newFileData);
                    }
                }
            } else {
                /* single key/value given, check whether it already exists */
                keyFound = 0;
                for (j = 0; existingKeys[j] != NULL; ++j) {
                    if (strcmp(existingKeys[j], keys) == 0) {
                        keyFound = 1;
                        break;
                    }
                }
                if (!keyFound) {
                    APPEND_KEYVAL_TO_BUF(keys, values, newFileData);
                }
            }
        }

        free_list_and_content(existingKeys);
        APPEND_NEWLINE_TO_BUF(newFileData);
        currSection = existingSections[i];
    }

    free_list_and_content(existingSections);

    /* if the section doesn't already exist, we need to create it */
    if (sectionFound == 0) {
        APPEND_SECTION_TO_BUF(section, newFileData);
        if (separator != NULL) {
            for (k = 0; k < numKeys; k++) {
                APPEND_KEYVAL_TO_BUF(splitKeys[k], splitValues[k], newFileData);
            }
        } else {
            APPEND_KEYVAL_TO_BUF(keys, values, newFileData);
        }
    }

    free_array_and_content(splitKeys, numKeys);
    free_array_and_content(splitValues, numValues);

    /* now close the file, reopen in write mode, and write the new data */
    if (fclose(stream) != 0) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_write_multiple: %s\n",
                "error closing ini file prior to write");
        free(newFileData);
        return -1;
    }

    stream = fopen(filename, "w");
    if (stream == NULL) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_write_multiple: %s\n",
                "error opening ini file in write mode");
        free(newFileData);
        return -1;
    }

    fprintf(stream, "%s", newFileData);
    free(newFileData);

    if (fclose(stream) != 0) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_write_multiple: %s\n",
                "error closing ini file after write");
        return -1;
    }

    /* success */
    return 0;

cleanup_and_return_malloc_fail:
    fprintf(stderr,
            "SailfishKeyProvider_ini_write_multiple: %s\n",
            "malloc failed during file regeneration");
cleanup_and_return_fail:
    if (stream != NULL && fclose(stream) != 0) {
        fprintf(stderr,
                "SailfishKeyProvider_ini_write_multiple: %s\n",
                "error closing ini file after failed write");
    }
    free_array_and_content(splitKeys, numKeys);
    free_array_and_content(splitValues, numValues);
    free_list_and_content(existingKeys);
    free_list_and_content(existingSections);
    free(newFileData);
    return -1;
}

int SailfishKeyProvider_ini_write(
                    const char * directory,
                    const char * filename, /* must contain full path */
                    const char * section,
                    const char * key,
                    const char * value)
{
    return SailfishKeyProvider_ini_write_multiple_impl(
                directory, filename, section, key, value, NULL);
}

int SailfishKeyProvider_ini_write_multiple(
                    const char * directory,
                    const char * filename, /* must contain full path */
                    const char * section,
                    const char * keys,     /* separator-separated list of keys */
                    const char * values,   /* separator-separated list of values */
                    const char * separator)
{
    return SailfishKeyProvider_ini_write_multiple_impl(
                directory, filename, section, keys, values, separator);
}
