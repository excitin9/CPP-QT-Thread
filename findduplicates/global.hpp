#ifndef GLOBAL_HPP
#define GLOBAL_HPP
/*
    Copyright (c) 2009-10 Qtrac Ltd. All rights reserved.

    This program or module is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version. It is provided
    for educational purposes and is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
    the GNU General Public License for more details.
*/

#include "threadsafehash.hpp"

//前者存储MD5值，后者保存文件大小
typedef ThreadSafeHash<QPair<QByteArray, qint64>,
                       QString> FilesForMD5;

#endif // GLOBAL_HPP
