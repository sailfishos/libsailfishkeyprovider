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

/*
    The following code is used to generate encoded keys
*/
static int
generate_keys(int inputsSize, char *inputs[], char *encodingScheme, char *encodingKey)
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

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("Usage: %s <method> <key> <key-1> [key-2] [...]\n", argv[0]);
        return 1;
    }

    char *method = argv[1];
    char *key = argv[2];

    generate_keys(argc-3, argv+3, method, key);

    return 0;
}
