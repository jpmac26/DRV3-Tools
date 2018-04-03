#pragma once

#include "utils_global.h"
#include <QByteArray>
#include <QString>

UTILSSHARED_EXPORT QString bytes_to_str(const QByteArray &data, int &pos, const int len = -1, const bool utf16 = false);
UTILSSHARED_EXPORT QByteArray str_to_bytes(const QString &string, const bool utf16 = false);
UTILSSHARED_EXPORT QByteArray get_bytes(const QByteArray &data, int &pos, const int len = -1);

template <typename T> UTILSSHARED_EXPORT inline T bytes_to_num(const QByteArray &data, int &pos, const bool big_endian = false)
{
    const int num_size = sizeof(T);
    T result = 0;

    if (big_endian)
    {
        for (int i = 0; i != num_size; i++)
        {
            result |= (data.at(pos + i) & 0xFF) << ((num_size - i - 1) * 8);
        }
    }
    else
    {
        for (int i = 0; i != num_size; i++)
        {
            result |= (data.at(pos + i) & 0xFF) << (i * 8);
        }
    }

    pos += num_size;
    return result;
}

template <typename T> UTILSSHARED_EXPORT inline QByteArray num_to_bytes(T num, const bool big_endian = false)
{
    const int num_size = sizeof(T);

    QByteArray result;
    result.reserve(num_size);

    if (big_endian)
    {
        for (int i = 0; i != num_size; i++)
        {
            result.prepend((char)(num >> (i * 8)));
        }
    }
    else
    {
        for (int i = 0; i != num_size; i++)
        {
            result.append((char)(num >> (i * 8)));
        }
    }

    return result;
}
