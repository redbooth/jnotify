/*******************************************************************************
 * JNotify - Allow java applications to register to File system events.
 * 
 * Copyright (C) 2005 - Content Objects
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 ******************************************************************************
 *
 * Content Objects, Inc., hereby disclaims all copyright interest in the
 * library `JNotify' (a Java library for file system events). 
 * 
 * Yahali Sherman, 21 November 2005
 *    Content Objects, VP R&D.
 *    
 ******************************************************************************
 * Author : Omry Yadan
 ******************************************************************************/


#include "Logger.h"

#include <stdio.h>
#include <windows.h>
#include "Lock.h"

namespace {

Lock _logLoc;
FILE *logFile = 0;

const bool DEBUG = false;
const bool LOG = true;

char sbuf[1024];

void _log()
{
    if (logFile == 0) return;
    fputs(sbuf, logFile);
    fputc('\n', logFile);
    fflush(logFile);
}

}

// TODO: rotate log file
void initLog(const char *path)
{
    if (logFile != 0) {
        fclose(logFile);
    }
    logFile = fopen(path, "a");
}

void debug(const char *format, ...)
{
    if (!DEBUG) return;

    _logLoc.lock();

    va_list args;
    va_start(args, format);
    _vsnprintf(sbuf, 1024, format, args);
    va_end(args);

    _log();

    _logLoc.unlock();
}

void log(const char *format, ...)
{
    if (!LOG) return;

    _logLoc.lock();

    va_list args;
    va_start(args, format);
    _vsnprintf(sbuf, 1024, format, args);
    va_end(args);

    _log();

    _logLoc.unlock();
}
