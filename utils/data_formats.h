#ifndef DRV3_DEC_H
#define DRV3_DEC_H

#include <cmath>
#include <QDir>
#include <QException>
#include <QFileInfo>
#include <QHash>
#include <QTextCodec>
#include <QTextStream>
#include <QVector>
#include "binarydata.h"

const QString SPC_MAGIC = "CPS.";
const QString SPC_TABLE_MAGIC = "Root";
const QString STX_MAGIC = "STXT";

struct SpcSubfile
{
    QString filename;
    QByteArray data;
    ushort cmp_flag;
    ushort unk_flag;
    int cmp_size;
    int dec_size;
    int name_len;
};

struct SpcFile
{
    QString filename;
    QByteArray unk1;
    uint unk2;
    QList<SpcSubfile> subfiles;
};

struct WrdCmd
{
    ushort opcode;
    QList<ushort> args;
};

struct WrdFile
{
    QString filename;
    QStringList labels;
    QStringList flags;
    QStringList strings;
    QList<WrdCmd> cmds;
};

inline uchar bit_reverse(uchar b);
SpcFile spc_from_data(const QByteArray &data);
QByteArray spc_to_data(const SpcFile &spc);
QByteArray spc_dec(const QByteArray &data, int dec_size = -1);
QByteArray spc_cmp(const QByteArray &data);
QByteArray srd_dec(const QByteArray &data);
QByteArray srd_dec_chunk(const QByteArray &chunk, QString cmp_mode);
QStringList get_stx_strings(const QByteArray &data);
QByteArray repack_stx_strings(QStringList strings);
WrdFile wrd_from_data(const QByteArray &data, QString filename);
QByteArray wrd_to_data(const WrdFile &wrd);

#endif // DRV3_DEC_H
