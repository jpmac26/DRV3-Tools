#ifndef BINARYDATA_H
#define BINARYDATA_H

#include <QDataStream>
#include <QtEndian>
#include <QString>

QByteArray from_u16(ushort n);
QByteArray from_u32(uint n);

struct BinaryData
{
    QByteArray Bytes;
    int Position;

    BinaryData();
    BinaryData(int reserve_size);
    BinaryData(QByteArray data);
    QByteArray get(int len);
    QString get_str(int len = -1, bool utf16 = false);
    char get_u8();
    ushort get_u16();
    ushort get_u16be();
    uint get_u32();
    uint get_u32be();
    int size();
    QByteArray& append(char c);
    QByteArray& append(QByteArray a);
    QByteArray& insert(int i, char c);
    int lastIndexOf(QByteArray a, int start = 0, int end = 0) const;
    char at(int i) const;
};

#endif // BINARYDATA_H
