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

#ifndef INIPARSER_H
#define INIPARSER_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
char ** SailfishKeyProvider_ini_sections(
                    const char * filename);

char ** SailfishKeyProvider_ini_keys(
                    const char * filename,
                    const char * section);

char * SailfishKeyProvider_ini_read(
                    const char * filename,
                    const char * section,
                    const char * key);

int SailfishKeyProvider_ini_write(
                    const char * directory,
                    const char * filename, /* must include full path */
                    const char * section,
                    const char * key,
                    const char * value);
#ifdef __cplusplus
}
#endif

#endif /* INIPARSER_H */
