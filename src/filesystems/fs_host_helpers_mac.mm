// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: macOS-specific trash implementation

#import <Foundation/Foundation.h>
#include <string>

int utf8_trash(const std::string& path) {
    @autoreleasepool {
        NSString *nsPath = [NSString stringWithUTF8String:path.c_str()];
        NSURL *fileURL = [NSURL fileURLWithPath:nsPath];

        NSFileManager *fileManager = [NSFileManager defaultManager];
        NSError *error = nil;

        BOOL success = [fileManager trashItemAtURL:fileURL
                                  resultingItemURL:nil
                                             error:&error];

        return success ? 0 : -1;
    }
}