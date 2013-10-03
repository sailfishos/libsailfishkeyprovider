/*
 * LICENSE - TBD
 * Copyright 2013 Jolla Ltd. <chris.adams@jollamobile.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sailfishkeyprovider.h"
#include "base64ed.h"
#include "xored.h"

#define TEST_PASS 0
#define TEST_SKIP 1
#define TEST_FAIL 2

int test_b64_encode();
int test_b64_decode();
int test_b64_roundtrip();
int test_xor_encode();
int test_xor_decode();
int test_xor_roundtrip();
int test_key_encdec_roundtrip();
int test_stored_key();
int test_store_key();

int generate_keys();

int main(int argc, char *argv[])
{
    /* if you need to regenerate keys, call generate_keys():
         char *inputs[] = { "some_oauth_key_value", "another" };
         generate_keys(2, inputs, "xor", "xorKey");
       we call it with null arguments below to avoid warning. */
    char *inputs[] = { "" };
    int generatedCount = generate_keys(0, inputs, "xor", "TestKey123");


    /* Now perform the unit test */


    int passCount = 0, failCount = 0, skipCount = 0;

    int i = 0;
    int testCount = 9;
    int results[] = {
        test_b64_encode(),
        test_b64_decode(),
        test_b64_roundtrip(),
        test_xor_encode(),
        test_xor_decode(),
        test_xor_roundtrip(),
        test_key_encdec_roundtrip(),
        test_stored_key(),
        test_store_key()
    };

    (void)argc;
    (void)argv;

    for (i = 0; i < testCount; ++i) {
        switch (results[i]) {
            case TEST_PASS: passCount++; break;
            case TEST_SKIP: skipCount++; break;
            case TEST_FAIL: failCount++; break;
            default: {
                fprintf(stderr,
                        "erroneous result[%d] = %d",
                        i, results[i]);
                break;
            }
        }
    }

    fprintf(stdout,
            "Passed: %d  Failed: %d  Skipped: %d\n",
            passCount, failCount, skipCount);

    if (generatedCount) {
        fprintf(stdout,
                "Generated: %d encoded keys for embedding\n",
                generatedCount);
    }

    if (failCount == 0)
        return TEST_PASS;
    return TEST_FAIL;
}

int test_b64_encode()
{
    size_t encoded_size = 0;
    char *encoded_data = 0;
    char *plaintext = "Hello, World!";
    char *expected = "SGVsbG8sIFdvcmxkIQ==";
    size_t plaintext_size = strlen(plaintext);
    size_t expected_size = strlen(expected);

    /* test invalid parameters */
    encoded_size = SailfishKeyProvider_base64_encode(
                        NULL, plaintext_size, &encoded_data);
    if (encoded_size != 0) {
        fprintf(stdout,
                "%s\n",
                "FAIL!    test_b64_encode: invalid data");
        return TEST_FAIL;
    }

    encoded_size = SailfishKeyProvider_base64_encode(
            plaintext, 0, &encoded_data);
    if (encoded_size != 0) {
        fprintf(stdout,
                "%s\n",
                "FAIL!    test_b64_encode: invalid data_size");
        return TEST_FAIL;
    }

    encoded_size = SailfishKeyProvider_base64_encode(
            plaintext, plaintext_size, 0);
    if (encoded_size != 0) {
        fprintf(stdout,
                "%s\n",
                "FAIL!    test_b64_encode: invalid encoded_data");
        return TEST_FAIL;
    }

    /* test valid parameters */
    encoded_size = SailfishKeyProvider_base64_encode(
            plaintext, plaintext_size, &encoded_data);
    if (encoded_size == 0) {
        fprintf(stdout,
                "%s\n",
                "FAIL!    test_b64_encode: parameters were valid");
        return TEST_FAIL;
    }

    if (encoded_size != expected_size) {
        fprintf(stdout,
                "%s\n        actual: %d\n        expected: %d\n",
                "FAIL!    test_b64_encode: invalid encoded size:",
                encoded_size,
                expected_size);
        return TEST_FAIL;
    }

    if (encoded_data[expected_size] != 0) {
        fprintf(stdout,
                "%s\n",
                "FAIL!    test_b64_encode: encoded data not null terminated");
        return TEST_FAIL;
    }

    if (strncmp(expected, (char*)encoded_data, expected_size) != 0) {
        fprintf(stdout,
                "%s\n        actual: %s\n        expected: %s\n",
                "FAIL!    test_b64_encode: encoded data not expected",
                encoded_data,
                expected);
        return TEST_FAIL;
    }

    fprintf(stdout,
            "%s\n",
            "PASS!    test_b64_encode");
    free(encoded_data);
    return TEST_PASS;
}

int test_b64_decode()
{
    char *expected = "Hello, World!";
    char *encoded_data = "SGVsbG8sIFdvcmxkIQ==";
    size_t encoded_size = strlen(encoded_data);
    size_t expected_size = strlen(expected);
    size_t decoded_size = 0;
    char *decoded_data = 0;

    /* test invalid parameters */
    decoded_size = SailfishKeyProvider_base64_decode(
                        NULL, encoded_size, &decoded_data);
    if (decoded_size != 0) {
        fprintf(stdout,
                "%s\n",
                "FAIL!    test_b64_decode: invalid encoded_data");
        return TEST_FAIL;
    }

    decoded_size = SailfishKeyProvider_base64_decode(
            encoded_data, 0, &decoded_data);
    if (decoded_size != 0) {
        fprintf(stdout,
                "%s\n",
                "FAIL!    test_b64_decode: invalid encoded_size");
        return TEST_FAIL;
    }

    decoded_size = SailfishKeyProvider_base64_decode(
            encoded_data, encoded_size, 0);
    if (decoded_size != 0) {
        fprintf(stdout,
                "%s\n",
                "FAIL!    test_b64_decode: invalid decoded_data");
        return TEST_FAIL;
    }

    /* test valid parameters */
    decoded_size = SailfishKeyProvider_base64_decode(
            encoded_data, encoded_size, &decoded_data);
    if (decoded_size == 0) {
        fprintf(stdout,
                "%s\n",
                "FAIL!    test_b64_decode: parameters were valid");
        return TEST_FAIL;
    }

    if (decoded_size != expected_size) {
        fprintf(stdout,
                "%s\n        actual: %d\n        expected: %d\n",
                "FAIL!    test_b64_decode: invalid decoded size:",
                decoded_size,
                expected_size);
        return TEST_FAIL;
    }

    if (decoded_data[expected_size] != 0) {
        fprintf(stdout,
                "%s\n",
                "FAIL!    test_b64_decode: decoded data not null terminated");
        return TEST_FAIL;
    }

    if (strncmp(expected, (char*)decoded_data, expected_size) != 0) {
        fprintf(stdout,
                "%s\n        actual: %s\n        expected: %s\n",
                "FAIL!    test_b64_decode: decoded data not expected",
                decoded_data,
                expected);
        return TEST_FAIL;
    }

    fprintf(stdout,
            "%s\n",
            "PASS!    test_b64_decode");
    free(decoded_data);
    return TEST_PASS;
}

int test_b64_roundtrip()
{
    char *encoded_data = 0;
    char *decoded_data = 0;

    int inputs_size = 9;
    int idx = inputs_size - 1;
    char * inputs[] = {
        "A",
        "AB",
        "ABC",
        "Hello, World!",
        "The quick red fox jumped over the lazy brown dog...",
        "6 on one hand; half a dozen the other?",
        " ",
        "9",
        "b0-=+;/:\"\"'"
    };

    while (idx >= 0) {
        char *data = inputs[idx];
        size_t data_size = strlen(data);
        size_t encoded_size = SailfishKeyProvider_base64_encode(
                                    data,
                                    data_size,
                                    &encoded_data);

        if (encoded_size == 0 || encoded_data == NULL) {
            fprintf(stdout,
                    "FAIL!    test_b64_roundtrip: encoding failed: %s\n",
                    data);
            return TEST_FAIL;
        } else {
            size_t decoded_size = SailfishKeyProvider_base64_decode(
                                    encoded_data,
                                    encoded_size,
                                    &decoded_data);
            if (decoded_size == 0 || decoded_data == NULL) {
                fprintf(stdout,
                        "FAIL!    test_b64_roundtrip: decoding failed: %s\n",
                        data);
                return TEST_FAIL;
            } else if (strncmp(decoded_data,
                               (char*)data,
                               data_size) != 0) {
                fprintf(stdout,
                        "%s\n        actual: %s\n        expected: %s\n",
                        "FAIL!    test_b64_roundtrip: round trip failed:",
                        decoded_data,
                        data);
                return TEST_FAIL;
            }

            /* success. */
            free(decoded_data);
            free(encoded_data);
            idx--;
        }
    }

    fprintf(stdout,
            "%s\n",
            "PASS!    test_b64_roundtrip");
    return TEST_PASS;
}

int test_xor_encode()
{
    /* TODO: test various key and value inputs / sizes */
    fprintf(stdout,
            "SKIPPED! %s\n",
            "test_xor_encode: functionality tested by roundtrip test");
    return TEST_SKIP;
}

int test_xor_decode()
{
    /* TODO: test various key and value inputs / sizes */
    fprintf(stdout,
            "SKIPPED! %s\n",
            "test_xor_decode: functionality tested by roundtrip test");
    return TEST_SKIP;
}

int test_xor_roundtrip()
{
    char plaintext[] = "Hello, World!";
    char key[] = "TestKey999Z+";
    size_t ptLen = strlen(plaintext);
    size_t keyLen = strlen(key);

    char *encoded = SailfishKeyProvider_xor_encode(
                                    plaintext,
                                    ptLen,
                                    key,
                                    keyLen);

    if (encoded == NULL) {
        fprintf(stdout,
                "FAIL!    %s\n",
                "test_xor_roundtrip: encoding failed");
        return TEST_FAIL;
    }

    char *decoded = SailfishKeyProvider_xor_decode(
                                    encoded,
                                    ptLen,
                                    key,
                                    keyLen);

    if (decoded == NULL) {
        fprintf(stdout,
                "FAIL!    %s\n",
                "test_xor_roundtrip: decoding failed");
        free(encoded);
        return TEST_FAIL;
    }

    if (strlen(decoded) != ptLen /* we don't test encoded's length */
            || strncmp(decoded, (char*)plaintext, ptLen) != 0) {
        fprintf(stdout,
                "%s\n        actual: %s\n        expected: %s\n",
                "FAIL!    test_xor_roundtrip: round trip failed:",
                decoded,
                plaintext);
        free(decoded);
        free(encoded);
        return TEST_FAIL;
    }

    fprintf(stdout,
            "%s\n",
            "PASS!    test_xor_roundtrip");
    free(decoded);
    free(encoded);
    return TEST_PASS;
}

int test_key_encdec_roundtrip()
{
    int inputs_size = 11;
    int idx = inputs_size - 1;
    char * inputs[] = {
        "A",
        "AB",
        "ABC",
        "Hello, World!",
        "The quick red fox jumped over the lazy brown dog...",
        "6 on one hand; half a dozen the other?",
        " ",
        "9",
        "b0-=+;/:\"\"'",
        "ABCD12345",
        "12345678_99999"
    };

    while (idx >= 0) {
        char *keyValue = inputs[idx];
        char *encoded = NULL;
        char *decoded = NULL;
        int success = SailfishKeyProvider_encodeKey(
                                    keyValue,
                                    "xor",
                                    "TestKey123",
                                    &encoded);

        if (success != 0 || encoded == NULL || strlen(encoded) == 0) {
            fprintf(stdout,
                    "FAIL!    %s: %s\n",
                    "test_key_encdec_roundtrip: encoding failed",
                    keyValue);
            return TEST_FAIL;
        }

        success = SailfishKeyProvider_decodeKey(
                                    encoded,
                                    "xor",
                                    "TestKey123",
                                    &decoded);

        if (success != 0 || decoded == NULL || strlen(decoded) == 0) {
            fprintf(stdout,
                    "FAIL!    %s: %s\n",
                    "test_key_encdec_roundtrip: decoding failed",
                    keyValue);
            return TEST_FAIL;
        }

        if (strlen(keyValue) != strlen(decoded)
                || strncmp(keyValue,
                           (char*)decoded,
                           strlen(keyValue)) != 0) {
            fprintf(stdout,
                    "FAIL!    %s:\n        actual: %s\n        expected: %s\n",
                    "test_key_encdec_roundtrip: round trip failed",
                    decoded,
                    keyValue);
            return TEST_FAIL;
        }

        /* success */
        free(decoded);
        free(encoded);
        idx--;
    }

    fprintf(stdout,
            "%s\n",
            "PASS!    test_key_encdec_roundtrip");
    return TEST_PASS;
}

int test_stored_key()
{
    /* This is the most important test, as components rely on stored keys. */
    char expected_consumer_key[] = "ABCD12345";
    char expected_consumer_secret[] = "12345678_99999";
    char *stored_consumer_key = NULL;
    char *stored_consumer_secret = NULL;

    int ckSuccess = SailfishKeyProvider_storedKey(
            "tst_keyprovider",
            "test_stored_key",
            "consumer_key",
            &stored_consumer_key);

    int csSuccess = SailfishKeyProvider_storedKey(
            "tst_keyprovider",
            "test_stored_key",
            "consumer_secret",
            &stored_consumer_secret);

    size_t expected_ck_size = strlen(expected_consumer_key);
    size_t expected_cs_size = strlen(expected_consumer_secret);
    if (ckSuccess != 0 || csSuccess != 0
            || stored_consumer_key == NULL || stored_consumer_secret == NULL
            || strlen(stored_consumer_key) != expected_ck_size
            || strlen(stored_consumer_secret) != expected_cs_size
            || strncmp(stored_consumer_key,
                       (char*)expected_consumer_key,
                       expected_ck_size) != 0
            || strncmp(stored_consumer_secret,
                       (char*)expected_consumer_secret,
                       expected_cs_size) != 0) {
        fprintf(stdout,
                "FAIL!    %s\n",
                "test_stored_key: fatal error");
        return TEST_FAIL;
    }

    free(stored_consumer_secret);
    free(stored_consumer_key);
    fprintf(stdout,
            "%s\n",
            "PASS!    test_stored_key");
    return TEST_PASS;
}

int test_store_key()
{
    int storeSuccess = 0;
    size_t expected_ck_size = 0;
    char *stored_consumer_key = NULL;
    char expected_consumer_key[] = "EFGH12345";

    char *encoded_consumer_key = NULL;
    int success = SailfishKeyProvider_encodeKey(
            expected_consumer_key,
            "xor",
            "TestKey456",
            &encoded_consumer_key);

    if (success != 0 || encoded_consumer_key == NULL) {
        fprintf(stdout,
                "FAIL!    %s\n",
                "test_store_key: failed to create encoded key");
        return TEST_FAIL;
    }

    storeSuccess = SailfishKeyProvider_storeKey(
            "tst_keyprovider",
            "test_store_key",
            "consumer_key",
            encoded_consumer_key,
            "xor",
            "TestKey456");
    if (storeSuccess == -1) {
        fprintf(stdout,
                "FAIL!    %s\n",
                "test_store_key: failed to store encoded key");
        free(encoded_consumer_key);
        return TEST_FAIL;
    }

    success = SailfishKeyProvider_storedKey(
            "tst_keyprovider",
            "test_store_key",
            "consumer_key",
            &stored_consumer_key);
    if (success != 0 || stored_consumer_key == NULL) {
        fprintf(stdout,
                "FAIL!    %s\n",
                "test_store_key: failed to retrieve stored key");
        free(encoded_consumer_key);
        return TEST_FAIL;
    }

    expected_ck_size = strlen(expected_consumer_key);
    if (strlen(stored_consumer_key) != expected_ck_size
            || strncmp(stored_consumer_key,
                       (char*)expected_consumer_key,
                       expected_ck_size) != 0) {
        fprintf(stdout,
                "FAIL!    %s\n",
                "test_store_key: fatal error");
        free(stored_consumer_key);
        free(encoded_consumer_key);
        return TEST_FAIL;
    }

    free(stored_consumer_key);
    free(encoded_consumer_key);
    fprintf(stdout,
            "%s\n",
            "PASS!    test_stored_key");
    return TEST_PASS;
}

/*
    The following code is used to generate encoded keys
*/
int generate_keys(int inputsSize, char *inputs[], char *encodingScheme, char *encodingKey)
{
    int idx = inputsSize - 1;
    while (idx >= 0) {
        char *keyValue = inputs[idx];
        char *encoded = NULL;
        char *decoded = NULL;
        int ekSuccess = SailfishKeyProvider_encodeKey(
                                    keyValue,
                                    encodingScheme,
                                    encodingKey,
                                    &encoded);

        int dkSuccess = SailfishKeyProvider_decodeKey(
                                    encoded,
                                    encodingScheme,
                                    encodingKey,
                                    &decoded);

        fprintf(stdout,
                "generate_keys:\n    input: %s\n    encoded: %d %s\n    roundtrip: %d %s\n",
                keyValue, ekSuccess, encoded, dkSuccess, decoded);

        free(decoded);
        free(encoded);
        idx--;
    }

    return inputsSize;
}
