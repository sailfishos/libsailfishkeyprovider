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
    Base64 encoding and decoding functions

    Note that this code isn't intended to be particularly performant,
    but rather simple to understand and robust.
*/

#include "base64ed.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char b64_charset[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

static const uint8_t b64_reverse[] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /*   0 -   7 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /*   8 -  15 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /*  16 -  23 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /*  24 -  31 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /*  32 -  39 */
    0x0, 0x0, 0x0,  62, 0x0, 0x0, 0x0,  63,     /*  40 -  47 */
     52,  53,  54,  55,  56,  57,  58,  59,     /*  48 -  55 */
     60,  61, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /*  56 -  63 */
    0x0,   0,   1,   2,   3,   4,   5,   6,     /*  64 -  71 */
      7,   8,   9,  10,  11,  12,  13,  14,     /*  72 -  79 */
     15,  16,  17,  18,  19,  20,  21,  22,     /*  80 -  87 */
     23,  24,  25, 0x0, 0x0, 0x0, 0x0, 0x0,     /*  88 -  95 */
    0x0,  26,  27,  28,  29,  30,  31,  32,     /*  96 - 103 */
     33,  34,  35,  36,  37,  38,  39,  40,     /* 104 - 111 */
     41,  42,  43,  44,  45,  46,  47,  48,     /* 112 - 119 */
     49,  50,  51, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 120 - 127 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 128 - 135 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 136 - 143 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 144 - 151 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 152 - 159 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 160 - 167 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 168 - 175 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 176 - 183 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 184 - 191 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 192 - 199 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 200 - 207 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 208 - 215 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 216 - 223 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 224 - 231 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 232 - 239 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,     /* 240 - 247 */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0      /* 248 - 255 */
};

/*
    Encodes the given \a data with Base64 encoding.
    Returns the size of the \a encoded_data on success,
    or 0 on error.
*/
size_t SailfishKeyProvider_base64_encode(
                    const char *data,
                    size_t data_size,
                    char **encoded_data)
{
    size_t remaining_size = data_size;
    const char *curr_data = data;
    char *curr_encoded = 0;
    uint32_t block = 0;
    size_t encoded_size = 0;
    uint8_t overrides = 0x0;

    /* ensure the input arguments are valid */
    if (encoded_data == NULL || data_size == 0 || data == NULL) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_base64_encode: invalid arguments");
        return 0;
    }

    /* calculate the size of encoded data and allocate buffer */
    encoded_size = (4 * ((data_size + 2) / 3)) + 1;
    curr_encoded = (char *)malloc(encoded_size);
    *encoded_data = curr_encoded;
    if (curr_encoded == NULL) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_base64_encode: malloc failed");
        return 0;
    }
    memset(curr_encoded, 0, encoded_size);

    /*
        Algorithm:

        Grab 3 bytes, concatenate them into a 24 bit block.
        Each block consists of 4 x 6 bit printable char indexes.
        Append to the encoded data string the 4 characters defined by
        the characters at those indexes from the Base64 character set.

        If the input data size doesn't divide cleanly by 3, right-pad
        it with zero-bytes until it does.  Overwrite pad bytes in the
        output with '=' characters.
    */
    while (remaining_size > 0) {
        block = 0;
        if (remaining_size == 1) {
            block = ((*((uint8_t*)(curr_data))) << 16);
            overrides = 0x2;
            remaining_size = 0;
        } else if (remaining_size == 2) {
            block = ((*((uint8_t*)(curr_data  ))) << 16)
                  | ((*((uint8_t*)(curr_data+1))) << 8);
            overrides = 0x1;
            remaining_size = 0;
        } else {
            block = ((*((uint8_t*)(curr_data  ))) << 16)
                  | ((*((uint8_t*)(curr_data+1))) << 8)
                  | ((*((uint8_t*)(curr_data+2))));
            remaining_size -= 3;
        }

        /* look up the printable char from the base 64 character set  */
        curr_encoded[0] = b64_charset[((block>>18) & 0x3F)];
        curr_encoded[1] = b64_charset[((block>>12) & 0x3F)];
        curr_encoded[2] = b64_charset[((block>>6 ) & 0x3F)];
        curr_encoded[3] = b64_charset[( block      & 0x3F)];

        curr_data += 3;
        curr_encoded += 4;
    }

    /* if we padded the block, we need to override the pad chars */
    curr_encoded -= 4;
    if (overrides == 0x1) {
        curr_encoded[3] = '=';
    } else if (overrides == 0x2) {
        curr_encoded[3] = '=';
        curr_encoded[2] = '=';
    }

    return encoded_size - 1; /* minus the null terminator */
}

/*
    Decodes the given \a encoded_data from Base64 encoding.
    Returns the size of the \a decoded_data on success,
    or 0 on error.
*/
size_t SailfishKeyProvider_base64_decode(
                    const char *encoded_data,
                    size_t encoded_size,
                    char **decoded_data)
{
    size_t decoded_size = (3 * (encoded_size / 4)) + 1; /* incl null term */
    size_t remaining_size = encoded_size;
    const char *curr_encoded = encoded_data;
    char *curr_decoded = 0;
    uint32_t paddings = 0;
    uint32_t block = 0;
    uint8_t b1 = 0, b2 = 0, b3 = 0, b4 = 0; /* block bytes */
    uint8_t c1 = 0, c2 = 0, c3 = 0, c4 = 0; /* chunk bytes */

    /* check for invalid inputs */
    if (encoded_size == 0 || encoded_size % 4
            || encoded_data == NULL || decoded_data == NULL) {
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_base64_decode: invalid arguments");
        return 0;
    }

    /* detect if padding chars exist */
    if (encoded_data[encoded_size-2] == '=') {
        paddings = 2;
    } else if (encoded_data[encoded_size-1] == '=') {
        paddings = 1;
    }

    /* allocate buffer for decoded data */
    curr_decoded = (char *)malloc(decoded_size);
    *decoded_data = curr_decoded;
    if (curr_decoded == NULL) {
        /* unable to allocate buffer */
        fprintf(stderr,
                "%s\n",
                "SailfishKeyProvider_base64_decode: malloc failed");
        return 0;
    }
    memset(curr_decoded, 0, decoded_size);

    /*
        Algorithm:

        Grab a 4 byte chunk, and use each byte as an index into the reverse
        lookup table.  The value at that index in the reverse lookup table
        will be a 6 bit value.  Concatenate the 4 x 6 bit indexes into a
        single 24 bit block, and split it into 3 decoded bytes.

        If the last byte or two bytes of the encoded data is '=' then
        they are padding bytes.  These are overwritten with zeros for
        decoding into the 24 bit block, but then only the zeroth byte
        in that block gets appended to the decoded data buffer.
    */
    while (remaining_size > 0) {
        /* grab 4 byte chunk of encoded data */
        c1 = (uint8_t)(curr_encoded[0]);
        c2 = (uint8_t)(curr_encoded[1]);
        c3 = (uint8_t)(curr_encoded[2]);
        c4 = (uint8_t)(curr_encoded[3]);

        /* undo any padding */
        if (remaining_size == 4) {
            if (paddings == 2) {
                c3 = 'A';
                c4 = 'A';
            } else if (paddings == 1) {
                c4 = 'A';
            }
        }

        /* use each byte to look up a 6 bit index from reverse table */
        b1 = b64_reverse[c1];
        b2 = b64_reverse[c2];
        b3 = b64_reverse[c3];
        b4 = b64_reverse[c4];

        /* check that the encoded data is valid */
        if ((b1 == 0x0 && c1 != 'A') ||
            (b2 == 0x0 && c2 != 'A') ||
            (b3 == 0x0 && c3 != 'A') ||
            (b4 == 0x0 && c4 != 'A')) {
            /* the encoded data contains characters
               that aren't in the base64 charset */
            fprintf(stderr,
                    "%s: %s\n",
                    "SailfishKeyProvider_base64_decode: invalid data",
                    encoded_data);
            free(*decoded_data);
            *decoded_data = NULL;
            return 0;
        }

        /* construct a 24 bit block from the 4 x 6 bit indexes */
        block = (b1 << 18) | (b2 << 12) | (b3 << 6) | b4;

        /* break the block into 3 decoded bytes - ignore padding */
        curr_decoded[0] = (char)((block >> 16) & 0xFF);
        if (remaining_size != 4 || paddings == 0) {
            curr_decoded[1] = (char)((block >> 8) & 0xFF);
            curr_decoded[2] = (char)((block >> 0) & 0xFF);
        } else if (remaining_size == 4 && paddings == 1) {
            curr_decoded[1] = (char)((block >> 8) & 0xFF);
        }

        remaining_size -= 4;
        curr_encoded += 4;
        curr_decoded += 3;
    }

    return decoded_size - paddings - 1; /* minus the null terminator */
}
