#ifndef WRD_H
#define WRD_H

#include "utils_global.h"
#include "binarydata.h"
#include "stx.h"

struct UTILSSHARED_EXPORT WrdCmd
{
    uchar opcode;
    QString name;
    QList<ushort> args;
    QList<uchar> arg_types;  // 0 = flag, 1 = raw number, 2 = string
};

// Official command names found in game_resident/command_label.dat
static QList<WrdCmd> known_commands = {
    {0x00, "FLG", {}, {0, 0}},              // Set Flag
    {0x01, "IFF", {}, {0, 0, 0}},           // If Flag
    {0x02, "WAK", {}, {0, 0, 0}},           // Wake? Work? (Seems to be used to configure game engine parameters)
    {0x03, "IFW", {}, {0, 0, 1}},           // If WAK?
    {0x04, "SWI", {}, {0}},                 // Begin switch statement
    {0x05, "CAS", {}, {1}},                 // Switch Case
    {0x06, "MPF", {}, {}},                  // Map Flag?
    {0x07, "SPW", {}, {}},                  //
    {0x08, "MOD", {}, {0, 0, 0, 0}},        // Set Modifier (Also used to configure game engine parameters)
    {0x09, "HUM", {}, {}},                  // Human? Seems to be used to initialize "interactable" objects in a map?
    {0x0A, "CHK", {}, {0}},                 // Check?
    {0x0B, "KTD", {}, {0, 0}},              // Kotodama?
    {0x0C, "CLR", {}, {}},                  // Clear?
    {0x0D, "RET", {}, {}},                  // Return? There's definitely a command later which as also definitely return, though...
    {0x0E, "KNM", {}, {0, 0, 0, 0, 0}},     // Kinematics (camera movement)
    {0x0F, "CAP", {}, {}},                  // Camera Parameters?
    {0x10, "FIL", {}, {0, 0}},              // Load Script File & jump to label
    {0x11, "END", {}, {}},                  // End of script or switch case
    {0x12, "SUB", {}, {0, 0}},              // Jump to subroutine
    {0x13, "RTN", {}, {}},                  // Return (called inside subroutine)
    {0x14, "LAB", {}, {1}},                 // Label number
    {0x15, "JMP", {}, {0}},                 // Jump to label
    {0x16, "MOV", {}, {}},                  // Movie
    {0x17, "FLS", {}, {0, 0, 0, 0}},        // Flash
    {0x18, "FLM", {}, {}},                  // Flash Modifier?
    {0x19, "VOI", {}, {0, 0}},              // Play voice clip
    {0x1A, "BGM", {}, {0, 0, 0}},           // Play BGM
    {0x1B, "SE_", {}, {0, 0}},              // Play sound effect
    {0x1C, "JIN", {}, {0, 0}},              // Play jingle
    {0x1D, "CHN", {}, {0}},                 // Set active character ID (current person speaking)
    {0x1E, "VIB", {}, {0, 0, 0}},           // Camera Vibration
    {0x1F, "FDS", {}, {0, 0, 0}},           // Character Vibration
    {0x20, "FLA", {}, {}},
    {0x21, "LIG", {}, {0, 0, 0}},           // Lighting Parameters
    {0x22, "CHR", {}, {0, 0, 0, 0, 0}},     // Character Parameters
    {0x23, "BGD", {}, {}},                  // Background Object Parameters
    {0x24, "CUT", {}, {}},
    {0x25, "ADF", {}, {0, 0, 0, 0 ,0}},
    {0x26, "PAL", {}, {}},
    {0x27, "MAP", {}, {0, 0, 0}},           // Load Map
    {0x28, "OBJ", {}, {0, 0, 0}},           // Load Object
    {0x29, "BUL", {}, {}},
    {0x2A, "CRF", {}, {}},
    {0x2B, "CAM", {}, {0, 0, 0, 0, 0}},     // Camera Parameters
    {0x2C, "KWM", {}, {}},                  // Window Parameters?
    {0x2D, "ARE", {}, {}},
    {0x2E, "KEY", {}, {}},
    {0x2F, "WIN", {}, {}},
    {0x30, "MSC", {}, {}},
    {0x31, "CSM", {}, {}},
    {0x32, "PST", {}, {}},
    {0x33, "KNS", {}, {0, 0, 0, 0}},
    {0x34, "FON", {}, {}},                  // Set Font?
    {0x35, "BGO", {}, {0, 0, 0, 0, 0}},     // Load Background Object
    {0x36, "LOG", {}, {}},                  // Edit Text Backlog?
    {0x37, "SPT", {}, {}},
    {0x38, "CDV", {}, {}},
    {0x39, "SZM", {}, {}},
    {0x3A, "PVI", {}, {}},
    {0x3B, "EXP", {}, {}},                  // Give Experience?
    {0x3C, "MTA", {}, {}},
    {0x3D, "MVP", {}, {}},
    {0x3E, "POS", {}, {}},
    {0x3F, "ICO", {}, {}},
    {0x40, "EAI", {}, {}},
    {0x41, "COL", {}, {}},
    {0x42, "CFP", {}, {}},
    {0x43, "CLT=", {}, {}},                 // Text modifier command
    {0x44, "R=", {}, {}},
    {0x45, "PAD=", {}, {}},                 // Gamepad button symbol
    {0x46, "LOC", {}, {2}},                 // Display text string
    {0x47, "BTN", {}, {}},                  // Wait for button press
    {0x48, "ENT", {}, {}},
    {0x49, "CED", {}, {}},                  // Check End? (Used after IFF commands?)
    {0x4A, "LBN", {}, {1}},                 // Local Branch Number (for branching case statements)
    {0x4B, "JMN", {}, {1}}                  // Jump to Local Branch (for branching case statements)
};

struct UTILSSHARED_EXPORT WrdFile
{
    QString filename;
    QStringList labels;
    QStringList flags;
    QStringList strings;
    QList<QList<WrdCmd>> code;
    QList<uint> unk_data;
    bool external_strings;
};

UTILSSHARED_EXPORT WrdFile wrd_from_bytes(const QByteArray &bytes, QString filename);
UTILSSHARED_EXPORT QByteArray wrd_to_bytes(const WrdFile &wrd);
UTILSSHARED_EXPORT QList<QList<WrdCmd>> wrd_code_to_cmds(const QByteArray &bytes);
UTILSSHARED_EXPORT QByteArray wrd_cmds_to_code(const QByteArray &wrd);

#endif // WRD_H
