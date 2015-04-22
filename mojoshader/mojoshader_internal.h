#ifndef _INCLUDE_MOJOSHADER_INTERNAL_H_
#define _INCLUDE_MOJOSHADER_INTERNAL_H_

#ifndef __MOJOSHADER_INTERNAL__
#error Do not include this header from your applications.
#endif

// Shader bytecode format is described at MSDN:
//  http://msdn.microsoft.com/en-us/library/ff569705.aspx

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "mojoshader.h"

#define DEBUG_LEXER 0
#define DEBUG_PREPROCESSOR 0
#define DEBUG_ASSEMBLER_PARSER 0
#define DEBUG_COMPILER_PARSER 0
#define DEBUG_TOKENIZER \
    (DEBUG_PREPROCESSOR || DEBUG_ASSEMBLER_PARSER || DEBUG_LEXER)

#if (defined(__APPLE__) && defined(__MACH__))
#define PLATFORM_MACOSX 1
#endif

// This is the highest shader version we currently support.

#define MAX_SHADER_MAJOR 3
#define MAX_SHADER_MINOR 255  // vs_3_sw


// If SUPPORT_PROFILE_* isn't defined, we assume an implicit desire to support.
//  You get all the profiles unless you go out of your way to disable them.

#define SUPPORT_PROFILE_GLSL 1
#define SUPPORT_PROFILE_GLSL120 1

// Get basic wankery out of the way here...

typedef unsigned int uint;  // this is a printf() helper. don't use for code.

#ifdef _MSC_VER
#include <malloc.h>
#define va_copy(a, b) a = b
#define snprintf _snprintf  // !!! FIXME: not a safe replacement!
#define vsnprintf _vsnprintf  // !!! FIXME: not a safe replacement!
#define strcasecmp stricmp
#define strncasecmp strnicmp
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
typedef __int32 int32;
typedef __int64 int64;
#ifdef _WIN64
typedef __int64 ssize_t;
#elif defined _WIN32
typedef __int32 ssize_t;
#else
#error Please define your platform.
#endif
// Warning Level 4 considered harmful.  :)
#pragma warning(disable: 4100)  // "unreferenced formal parameter"
#pragma warning(disable: 4389)  // "signed/unsigned mismatch"
#else
#include <stdint.h>
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef int64_t int64;
typedef uint64_t uint64;
#endif

#ifdef sun
#include <alloca.h>
#endif

#ifdef __GNUC__
#define ISPRINTF(x,y) __attribute__((format (printf, x, y)))
#else
#define ISPRINTF(x,y)
#endif

#define STATICARRAYLEN(x) ( (sizeof ((x))) / (sizeof ((x)[0])) )


static inline int Min(const int a, const int b)
{
    return ((a < b) ? a : b);
}

static inline int Max(const int a, const int b)
{
    return ((a > b) ? a : b);
}


// Error lists...

typedef struct ErrorList ErrorList;
ErrorList *errorlist_create(MOJOSHADER_malloc m, MOJOSHADER_free f, void *d);
int errorlist_add(ErrorList *list, const char *fname, const int errpos, const char *str);
int errorlist_add_fmt(ErrorList *list, const char *fname, const int errpos, const char *fmt, ...) ISPRINTF(4,5);
int errorlist_add_va(ErrorList *list, const char *_fname, const int errpos, const char *fmt, va_list va);
int errorlist_count(ErrorList *list);
MOJOSHADER_error *errorlist_flatten(ErrorList *list); // resets the list!
void errorlist_destroy(ErrorList *list);


// Dynamic buffers...

typedef struct Buffer Buffer;
Buffer *buffer_create(size_t blksz, MOJOSHADER_malloc m, MOJOSHADER_free f, void *d);
char *buffer_reserve(Buffer *buffer, const size_t len);
int buffer_append(Buffer *buffer, const void *_data, size_t len);
int buffer_append_fmt(Buffer *buffer, const char *fmt, ...) ISPRINTF(2,3);
int buffer_append_va(Buffer *buffer, const char *fmt, va_list va);
size_t buffer_size(Buffer *buffer);
void buffer_empty(Buffer *buffer);
char *buffer_flatten(Buffer *buffer);
char *buffer_merge(Buffer **buffers, const size_t n, size_t *_len);
void buffer_destroy(Buffer *buffer);
ssize_t buffer_find(Buffer *buffer, const size_t start,
                    const void *data, const size_t len);


// we need to reference these by explicit value occasionally...
#define OPCODE_RET 28
#define OPCODE_IF 40
#define OPCODE_IFC 41
#define OPCODE_BREAK 44
#define OPCODE_BREAKC 45
#define OPCODE_TEXLD 66
#define OPCODE_SETP 94

// TEXLD becomes a different instruction with these instruction controls.
#define CONTROL_TEXLD  0
#define CONTROL_TEXLDP 1
#define CONTROL_TEXLDB 2

// result modifiers.
// !!! FIXME: why isn't this an enum?
#define MOD_SATURATE 0x01
#define MOD_PP 0x02
#define MOD_CENTROID 0x04

typedef enum
{
    REG_TYPE_TEMP = 0,
    REG_TYPE_INPUT = 1,
    REG_TYPE_CONST = 2,
    REG_TYPE_ADDRESS = 3,
    REG_TYPE_TEXTURE = 3,  // ALSO 3!
    REG_TYPE_RASTOUT = 4,
    REG_TYPE_ATTROUT = 5,
    REG_TYPE_TEXCRDOUT = 6,
    REG_TYPE_OUTPUT = 6,  // ALSO 6!
    REG_TYPE_CONSTINT = 7,
    REG_TYPE_COLOROUT = 8,
    REG_TYPE_DEPTHOUT = 9,
    REG_TYPE_SAMPLER = 10,
    REG_TYPE_CONST2 = 11,
    REG_TYPE_CONST3 = 12,
    REG_TYPE_CONST4 = 13,
    REG_TYPE_CONSTBOOL = 14,
    REG_TYPE_LOOP = 15,
    REG_TYPE_TEMPFLOAT16 = 16,
    REG_TYPE_MISCTYPE = 17,
    REG_TYPE_LABEL = 18,
    REG_TYPE_PREDICATE = 19,
    REG_TYPE_MAX = 19
} RegisterType;

typedef enum
{
    TEXTURE_TYPE_2D = 2,
    TEXTURE_TYPE_CUBE = 3,
    TEXTURE_TYPE_VOLUME = 4,
} TextureType;

typedef enum
{
    RASTOUT_TYPE_POSITION = 0,
    RASTOUT_TYPE_FOG = 1,
    RASTOUT_TYPE_POINT_SIZE = 2,
    RASTOUT_TYPE_MAX = 2
} RastOutType;

typedef enum
{
    MISCTYPE_TYPE_POSITION = 0,
    MISCTYPE_TYPE_FACE = 1,
    MISCTYPE_TYPE_MAX = 1
} MiscTypeType;

// source modifiers.
typedef enum
{
    SRCMOD_NONE,
    SRCMOD_NEGATE,
    SRCMOD_BIAS,
    SRCMOD_BIASNEGATE,
    SRCMOD_SIGN,
    SRCMOD_SIGNNEGATE,
    SRCMOD_COMPLEMENT,
    SRCMOD_X2,
    SRCMOD_X2NEGATE,
    SRCMOD_DZ,
    SRCMOD_DW,
    SRCMOD_ABS,
    SRCMOD_ABSNEGATE,
    SRCMOD_NOT,
    SRCMOD_TOTAL
} SourceMod;


typedef struct
{
    const uint32 *token;   // this is the unmolested token in the stream.
    int regnum;
    int relative;
    int writemask;   // xyzw or rgba (all four, not split out).
    int writemask0;  // x or red
    int writemask1;  // y or green
    int writemask2;  // z or blue
    int writemask3;  // w or alpha
    int orig_writemask;   // writemask before mojoshader tweaks it.
    int result_mod;
    int result_shift;
    RegisterType regtype;
} DestArgInfo;

// NOTE: This will NOT know a dcl_psize or dcl_fog output register should be
//        scalar! This function doesn't have access to that information.
static inline int scalar_register(const MOJOSHADER_shaderType shader_type,
                                  const RegisterType regtype, const int regnum)
{
    switch (regtype)
    {
        case REG_TYPE_RASTOUT:
            if (((const RastOutType) regnum) == RASTOUT_TYPE_FOG)
                return 1;
            else if (((const RastOutType) regnum) == RASTOUT_TYPE_POINT_SIZE)
                return 1;
            return 0;

        case REG_TYPE_DEPTHOUT:
        case REG_TYPE_CONSTBOOL:
        case REG_TYPE_LOOP:
            return 1;

        case REG_TYPE_MISCTYPE:
            if ( ((const MiscTypeType) regnum) == MISCTYPE_TYPE_FACE )
                return 1;
            return 0;

        case REG_TYPE_PREDICATE:
            return (shader_type == MOJOSHADER_TYPE_PIXEL) ? 1 : 0;

        default: break;
    }

    return 0;
}


// preprocessor stuff.

typedef enum
{
    TOKEN_UNKNOWN = 256,  // start past ASCII character values.

    // These are all C-like constructs. Tokens < 256 may be single
    //  chars (like '+' or whatever). These are just multi-char sequences
    //  (like "+=" or whatever).
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_RSHIFTASSIGN,
    TOKEN_LSHIFTASSIGN,
    TOKEN_ADDASSIGN,
    TOKEN_SUBASSIGN,
    TOKEN_MULTASSIGN,
    TOKEN_DIVASSIGN,
    TOKEN_MODASSIGN,
    TOKEN_XORASSIGN,
    TOKEN_ANDASSIGN,
    TOKEN_ORASSIGN,
    TOKEN_INCREMENT,
    TOKEN_DECREMENT,
    TOKEN_RSHIFT,
    TOKEN_LSHIFT,
    TOKEN_ANDAND,
    TOKEN_OROR,
    TOKEN_LEQ,
    TOKEN_GEQ,
    TOKEN_EQL,
    TOKEN_NEQ,
    TOKEN_HASH,
    TOKEN_HASHHASH,

    // This is returned if the preprocessor isn't stripping comments. Note
    //  that in asm files, the ';' counts as a single-line comment, same as
    //  "//". Note that both eat newline tokens: all of the ones inside a
    //  multiline comment, and the ending newline on a single-line comment.
    TOKEN_MULTI_COMMENT,
    TOKEN_SINGLE_COMMENT,

    // This is returned at the end of input...no more to process.
    TOKEN_EOI,

    // This is returned for char sequences we think are bogus. You'll have
    //  to judge for yourself. In most cases, you'll probably just fail with
    //  bogus syntax without explicitly checking for this token.
    TOKEN_BAD_CHARS,

    // This is returned if there's an error condition (the error is returned
    //  as a NULL-terminated string from preprocessor_nexttoken(), instead
    //  of actual token data). You can continue getting tokens after this
    //  is reported. It happens for things like missing #includes, etc.
    TOKEN_PREPROCESSING_ERROR,

    // These are all caught by the preprocessor. Caller won't ever see them,
    //  except TOKEN_PP_PRAGMA.
    //  They control the preprocessor (#includes new files, etc).
    TOKEN_PP_INCLUDE,
    TOKEN_PP_LINE,
    TOKEN_PP_DEFINE,
    TOKEN_PP_UNDEF,
    TOKEN_PP_IF,
    TOKEN_PP_IFDEF,
    TOKEN_PP_IFNDEF,
    TOKEN_PP_ELSE,
    TOKEN_PP_ELIF,
    TOKEN_PP_ENDIF,
    TOKEN_PP_ERROR,  // caught, becomes TOKEN_PREPROCESSING_ERROR
    TOKEN_PP_PRAGMA,
    TOKEN_INCOMPLETE_COMMENT,  // caught, becomes TOKEN_PREPROCESSING_ERROR
    TOKEN_PP_UNARY_MINUS,  // used internally, never returned.
    TOKEN_PP_UNARY_PLUS,   // used internally, never returned.
} Token;

void MOJOSHADER_print_debug_token(const char *subsystem, const char *token,
                                  const unsigned int tokenlen,
                                  const Token tokenval);

#endif  // _INCLUDE_MOJOSHADER_INTERNAL_H_


#if MOJOSHADER_DO_INSTRUCTION_TABLE
// These have to be in the right order! Arrays are indexed by the value
//  of the instruction token.

// INSTRUCTION_STATE means this opcode has to update the state machine
//  (we're entering an ELSE block, etc). INSTRUCTION means there's no
//  state, just go straight to the emitters.

// !!! FIXME: Some of these MOJOSHADER_TYPE_ANYs need to have their scope
// !!! FIXME:  reduced to just PIXEL or VERTEX.

INSTRUCTION(NOP, "NOP", 1, NULL, MOJOSHADER_TYPE_ANY)
INSTRUCTION(MOV, "MOV", 1, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(ADD, "ADD", 1, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(SUB, "SUB", 1, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(MAD, "MAD", 1, DSSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(MUL, "MUL", 1, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(RCP, "RCP", 1, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(RSQ, "RSQ", 1, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(DP3, "DP3", 1, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(DP4, "DP4", 1, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(MIN, "MIN", 1, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(MAX, "MAX", 1, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(SLT, "SLT", 1, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(SGE, "SGE", 1, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(EXP, "EXP", 1, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(LOG, "LOG", 1, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(LIT, "LIT", 3, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(DST, "DST", 1, DSS, MOJOSHADER_TYPE_VERTEX)
INSTRUCTION(LRP, "LRP", 2, DSSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(FRC, "FRC", 1, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(M4X4, "M4X4", 4, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(M4X3, "M4X3", 3, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(M3X4, "M3X4", 4, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(M3X3, "M3X3", 3, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(M3X2, "M3X2", 2, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(CALL, "CALL", 2, S, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(CALLNZ, "CALLNZ", 3, SS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(LOOP, "LOOP", 3, SS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(RET, "RET", 1, NULL, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(ENDLOOP, "ENDLOOP", 2, NULL, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(LABEL, "LABEL", 0, S, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(DCL, "DCL", 0, DCL, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(POW, "POW", 3, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(CRS, "CRS", 2, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(SGN, "SGN", 3, DSSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(ABS, "ABS", 1, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(NRM, "NRM", 3, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(SINCOS, "SINCOS", 8, SINCOS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(REP, "REP", 3, S, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(ENDREP, "ENDREP", 2, NULL, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(IF, "IF", 3, S, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(IFC, "IF", 3, SS, MOJOSHADER_TYPE_ANY)
INSTRUCTION(ELSE, "ELSE", 1, NULL, MOJOSHADER_TYPE_ANY)  // !!! FIXME: state!
INSTRUCTION(ENDIF, "ENDIF", 1, NULL, MOJOSHADER_TYPE_ANY) // !!! FIXME: state!
INSTRUCTION_STATE(BREAK, "BREAK", 1, NULL, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(BREAKC, "BREAK", 3, SS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(MOVA, "MOVA", 1, DS, MOJOSHADER_TYPE_VERTEX)
INSTRUCTION_STATE(DEFB, "DEFB", 0, DEFB, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(DEFI, "DEFI", 0, DEFI, MOJOSHADER_TYPE_ANY)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION_STATE(TEXCRD, "TEXCRD", 1, TEXCRD, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(TEXKILL, "TEXKILL", 2, D, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(TEXLD, "TEXLD", 1, TEXLD, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(TEXBEM, "TEXBEM", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(TEXBEML, "TEXBEML", 2, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(TEXREG2AR, "TEXREG2AR", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(TEXREG2GB, "TEXREG2GB", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(TEXM3X2PAD, "TEXM3X2PAD", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(TEXM3X2TEX, "TEXM3X2TEX", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(TEXM3X3PAD, "TEXM3X3PAD", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(TEXM3X3TEX, "TEXM3X3TEX", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(RESERVED, 0, 0, NULL, MOJOSHADER_TYPE_UNKNOWN)
INSTRUCTION_STATE(TEXM3X3SPEC, "TEXM3X3SPEC", 1, DSS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(TEXM3X3VSPEC, "TEXM3X3VSPEC", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(EXPP, "EXPP", 1, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(LOGP, "LOGP", 1, DS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(CND, "CND", 1, DSSS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(DEF, "DEF", 0, DEF, MOJOSHADER_TYPE_ANY)
INSTRUCTION(TEXREG2RGB, "TEXREG2RGB", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(TEXDP3TEX, "TEXDP3TEX", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(TEXM3X2DEPTH, "TEXM3X2DEPTH", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(TEXDP3, "TEXDP3", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(TEXM3X3, "TEXM3X3", 1, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(TEXDEPTH, "TEXDEPTH", 1, D, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(CMP, "CMP", 1, DSSS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(BEM, "BEM", 2, DSS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(DP2ADD, "DP2ADD", 2, DSSS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(DSX, "DSX", 2, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(DSY, "DSY", 2, DS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION(TEXLDD, "TEXLDD", 3, DSSSS, MOJOSHADER_TYPE_PIXEL)
INSTRUCTION_STATE(SETP, "SETP", 1, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(TEXLDL, "TEXLDL", 2, DSS, MOJOSHADER_TYPE_ANY)
INSTRUCTION_STATE(BREAKP, "BREAKP", 3, S, MOJOSHADER_TYPE_ANY)
#endif

// end of mojoshader_internal.h ...
