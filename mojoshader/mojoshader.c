/**
 * MojoShader; generate shader programs from bytecode of compiled
 *  Direct3D shaders.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

// !!! FIXME: this file really needs to be split up.
// !!! FIXME: I keep changing coding styles for symbols and typedefs.

// !!! FIXME: rules from MSDN about temp registers we probably don't check.
// - There are limited temporaries: vs_1_1 has 12 (ps_1_1 has _2_!).
// - SM2 apparently was variable, between 12 and 32. Shader Model 3 has 32.
// - A maximum of three temp registers can be used in a single instruction.

#define __MOJOSHADER_INTERNAL__ 1
#include "mojoshader_internal.h"

#include <math.h>

typedef struct ConstantsList {
    MOJOSHADER_constant constant;
    struct ConstantsList *next;
} ConstantsList;

typedef struct RegisterList {
    RegisterType regtype;
    int regnum;
    MOJOSHADER_usage usage;
    unsigned int index;
    int writemask;
    int misc;
    int written;
    struct RegisterList *next;
} RegisterList;

typedef struct {
    const uint32 *token;   // this is the unmolested token in the stream.
    int regnum;
    int swizzle;  // xyzw (all four, not split out).
    int swizzle_x;
    int swizzle_y;
    int swizzle_z;
    int swizzle_w;
    SourceMod src_mod;
    RegisterType regtype;
    int relative;
    RegisterType relative_regtype;
    int relative_regnum;
    int relative_component;
} SourceArgInfo;

struct Profile;  // predeclare.

// Context...this is state that changes as we parse through a shader...
typedef struct Context {
    int isfail;
    int current_position;
    const uint32 *orig_tokens;
    const uint32 *tokens;
    uint32 tokencount;
    int know_shader_size;
    const MOJOSHADER_samplerMap *samplermap;
    unsigned int samplermap_count;
    unsigned int shadow_samplers;
    Buffer *output;
    Buffer *preflight;
    Buffer *globals;
    Buffer *helpers;
    Buffer *subroutines;
    Buffer *mainline_intro;
    Buffer *mainline;
    Buffer *ignore;
    Buffer *output_stack[2];
    int indent_stack[2];
    int output_stack_len;
    int indent;
    const char *shader_type_str;
    int profileid;
    const struct Profile *profile;
    MOJOSHADER_shaderType shader_type;
    uint8 major_ver;
    uint8 minor_ver;
    DestArgInfo dest_arg;
    SourceArgInfo source_args[5];
    SourceArgInfo predicate_arg;  // for predicated instructions.
    uint32 dwords[4];
    uint32 version_token;
    int instruction_count;
    uint32 instruction_controls;
    uint32 previous_opcode;
    int loops;
    int reps;
    int max_reps;
    int cmps;
    RegisterList used_registers;
    RegisterList defined_registers;
    ErrorList *errors;
    int constant_count;
    ConstantsList *constants;
    int uniform_float4_count;
    int uniform_int4_count;
    int uniform_bool_count;
    int uniform_count;
    RegisterList uniforms;
    int attribute_count;
    RegisterList attributes;
    int sampler_count;
    RegisterList samplers;
    int centroid_allowed;
    int have_relative_input_registers;
    int have_relative_const_registers;
    int determined_constants_arrays;
    int predicated;
    int uses_pointsize;
    int uses_fog;
    int glsl_generated_lit_helper;
    int glsl_generated_texldd_setup;
    int glsl_generated_texm3x3spec_helper;
    int reset_texmpad;
    int texm3x2pad_dst0;
    int texm3x2pad_src0;
    int texm3x3pad_dst0;
    int texm3x3pad_src0;
    int texm3x3pad_dst1;
    int texm3x3pad_src1;
} Context;

// Profile entry points...

// one emit function for each opcode in each profile.
typedef void (*emit_function)(Context *ctx);

// one emit function for starting output in each profile.
typedef void (*emit_start)(Context *ctx, const char *profilestr);

// one emit function for ending output in each profile.
typedef void (*emit_end)(Context *ctx);

// one emit function for phase opcode output in each profile.
typedef void (*emit_phase)(Context *ctx);

// one emit function for finalizing output in each profile.
typedef void (*emit_finalize)(Context *ctx);

// one emit function for global definitions in each profile.
typedef void (*emit_global)(Context *ctx, RegisterType regtype, int regnum);

// one emit function for uniforms in each profile.
typedef void (*emit_uniform)(Context *ctx, RegisterType regtype, int regnum);

// one emit function for samplers in each profile.
typedef void (*emit_sampler)(Context *ctx, int stage, TextureType ttype,
                             int texbem);

// one emit function for attributes in each profile.
typedef void (*emit_attribute)(Context *ctx, RegisterType regtype, int regnum,
                               MOJOSHADER_usage usage, int index, int wmask,
                               int flags);

// one args function for each possible sequence of opcode arguments.
typedef int (*args_function)(Context *ctx);

// one state function for each opcode where we have state machine updates.
typedef void (*state_function)(Context *ctx);

// one function for varnames in each profile.
typedef const char *(*varname_function)(Context *c, RegisterType t, int num);

typedef struct Profile {
    const char *name;
    emit_start start_emitter;
    emit_end end_emitter;
    emit_phase phase_emitter;
    emit_global global_emitter;
    emit_uniform uniform_emitter;
    emit_sampler sampler_emitter;
    emit_attribute attribute_emitter;
    emit_finalize finalize_emitter;
    varname_function get_varname;
} Profile;


// jump between output sections in the context...

static int set_output(Context *ctx, Buffer **section)
{
    // only create output sections on first use.
    if(*section == NULL)
    {
        *section = buffer_create(256);
        if(*section == NULL) return 0;
    }

    ctx->output = *section;
    return 1;
}

static void push_output(Context *ctx, Buffer **section)
{
    assert(ctx->output_stack_len < (int)STATICARRAYLEN(ctx->output_stack));
    ctx->output_stack[ctx->output_stack_len] = ctx->output;
    ctx->indent_stack[ctx->output_stack_len] = ctx->indent;
    ctx->output_stack_len++;
    if(!set_output(ctx, section))
        return;
    ctx->indent = 0;
}

static inline void pop_output(Context *ctx)
{
    assert(ctx->output_stack_len > 0);
    ctx->output_stack_len--;
    ctx->output = ctx->output_stack[ctx->output_stack_len];
    ctx->indent = ctx->indent_stack[ctx->output_stack_len];
}


// Shader model version magic...

static inline uint32 ver_ui32(const uint8 major, const uint8 minor)
{ return (major << 16) | ((minor == 0xFF) ? 1 : minor); }

static inline int shader_version_supported(const uint8 maj, const uint8 min)
{ return ver_ui32(maj,min) <= ver_ui32(MAX_SHADER_MAJOR,MAX_SHADER_MINOR); }

static inline int shader_version_atleast(const Context *ctx, const uint8 maj, const uint8 min)
{ return ver_ui32(ctx->major_ver, ctx->minor_ver) >= ver_ui32(maj, min); }

static inline int shader_version_exactly(const Context *ctx, const uint8 maj, const uint8 min)
{ return (ctx->major_ver == maj) && (ctx->minor_ver == min); }

static inline int shader_is_pixel(const Context *ctx)
{ return ctx->shader_type == MOJOSHADER_TYPE_PIXEL; }

static inline int shader_is_vertex(const Context *ctx)
{ return (ctx->shader_type == MOJOSHADER_TYPE_VERTEX); }


static inline int isfail(const Context *ctx)
{ return ctx->isfail; }

static void failf(Context *ctx, const char *fmt, ...) ISPRINTF(2,3);
static void failf(Context *ctx, const char *fmt, ...)
{
    ctx->isfail = 1;

    // no filename at this level (we pass a NULL to errorlist_add_va()...)
    va_list ap;
    va_start(ap, fmt);
    errorlist_add_va(ctx->errors, NULL, ctx->current_position, fmt, ap);
    va_end(ap);
}

static inline void fail(Context *ctx, const char *reason)
{ failf(ctx, "%s", reason); }


static void output_line(Context *ctx, const char *fmt, ...) ISPRINTF(2,3);
static void output_line(Context *ctx, const char *fmt, ...)
{
    assert(ctx->output != NULL);
    if (isfail(ctx))
        return;  // we failed previously, don't go on...

    if(*fmt != '\0')
    {
        const int indent = ctx->indent;
        if (indent > 0)
        {
            char *indentbuf = (char*)alloca(indent);
            memset(indentbuf, '\t', indent);
            buffer_append(ctx->output, indentbuf, indent);
        }

        va_list ap;
        va_start(ap, fmt);
        buffer_append_va(ctx->output, fmt, ap);
        va_end(ap);
    }
    buffer_append(ctx->output, "\n", 1);
}

static inline void output_blank_line(Context *ctx)
{ if(!isfail(ctx)) buffer_append(ctx->output, "\n", 1); }


static void floatstr(char *buf, size_t bufsize, float f)
{
    int idx = 1;
    if(copysignf(1.0f, f) < 0.0f)
        ++idx;

    snprintf(buf, bufsize, "%.8e", f);
    if(isfinite(f)) buf[idx] = '.';
}

static inline TextureType cvtMojoToD3DSamplerType(const MOJOSHADER_samplerType type)
{ return (TextureType) (((int) type) + 2); }

static inline MOJOSHADER_samplerType cvtD3DToMojoSamplerType(const TextureType type)
{ return (MOJOSHADER_samplerType) (((int) type) - 2); }


// Deal with register lists...  !!! FIXME: I sort of hate this.

static void free_reglist(RegisterList *item)
{
    while (item != NULL)
    {
        RegisterList *next = item->next;
        free(item);
        item = next;
    }
}

static inline uint32 reg_to_ui32(const RegisterType regtype, const int regnum)
{ return ((uint32)regtype) | (((uint32)regnum)<<16); }

// !!! FIXME: ditch this for a hash table.
static RegisterList *reglist_insert(RegisterList *prev,
                                    const RegisterType regtype,
                                    const int regnum)
{
    const uint32 newval = reg_to_ui32(regtype, regnum);
    RegisterList *item = prev->next;

    while(item != NULL)
    {
        const uint32 val = reg_to_ui32(item->regtype, item->regnum);
        if(newval == val) return item;  // already set, so we're done.
        if(newval < val) break; // insert it here.

        // keep going, we're not to the insertion point yet.
        prev = item;
        item = item->next;
    }

    // we need to insert an entry after (prev).
    item = malloc(sizeof(RegisterList));
    item->regtype = regtype;
    item->regnum = regnum;
    item->usage = MOJOSHADER_USAGE_UNKNOWN;
    item->index = 0;
    item->writemask = 0;
    item->misc = 0;
    item->next = prev->next;
    prev->next = item;

    return item;
}

static RegisterList *reglist_find(const RegisterList *prev,
                                  const RegisterType rtype, const int regnum)
{
    const uint32 newval = reg_to_ui32(rtype, regnum);
    RegisterList *item = prev->next;
    while(item != NULL)
    {
        const uint32 val = reg_to_ui32(item->regtype, item->regnum);
        if(newval == val) return item;  // here it is.
        if(newval < val) return NULL;  // should have been here if it existed.
        item = item->next;
    }

    return NULL;
}

static inline const RegisterList *reglist_exists(RegisterList *prev,
                                                 const RegisterType regtype,
                                                 const int regnum)
{
    return reglist_find(prev, regtype, regnum);
}

static inline int register_was_written(Context *ctx, const RegisterType rtype,
                                       const int regnum)
{
    RegisterList *reg = reglist_find(&ctx->used_registers, rtype, regnum);
    return reg && reg->written;
}

static inline RegisterList *set_used_register(Context *ctx, const RegisterType regtype,
                                              const int regnum, const int written)
{
    RegisterList *reg = NULL;
    reg = reglist_insert(&ctx->used_registers, regtype, regnum);
    if(reg && written) reg->written = 1;
    return reg;
}

static inline int get_used_register(Context *ctx, const RegisterType regtype,
                                    const int regnum)
{
    return reglist_exists(&ctx->used_registers, regtype, regnum) != NULL;
}

static inline void set_defined_register(Context *ctx, const RegisterType rtype,
                                        const int regnum)
{
    reglist_insert(&ctx->defined_registers, rtype, regnum);
}

static inline int get_defined_register(Context *ctx, const RegisterType rtype,
                                       const int regnum)
{
    return reglist_exists(&ctx->defined_registers, rtype, regnum) != NULL;
}

static void add_attribute_register(Context *ctx, const RegisterType rtype,
                                   const int regnum, const MOJOSHADER_usage usage,
                                   const int index, const int writemask, int flags)
{
    RegisterList *item = reglist_insert(&ctx->attributes, rtype, regnum);
    item->usage = usage;
    item->index = index;
    item->writemask = writemask;
    item->misc = flags;

    if(rtype == REG_TYPE_OUTPUT && usage == MOJOSHADER_USAGE_POINTSIZE)
        ctx->uses_pointsize = 1;  // note that we have to check this later.
    else if(rtype == REG_TYPE_OUTPUT && usage == MOJOSHADER_USAGE_FOG)
        ctx->uses_fog = 1;  // note that we have to check this later.
}

static inline void add_sampler(Context *ctx, const int regnum,
                               TextureType ttype, const int texbem)
{
    const RegisterType rtype = REG_TYPE_SAMPLER;

    // !!! FIXME: make sure it doesn't exist?
    // !!! FIXME:  (ps_1_1 assume we can add it multiple times...)
    RegisterList *item = reglist_insert(&ctx->samplers, rtype, regnum);

    if (ctx->samplermap != NULL)
    {
        unsigned int i;
        for (i = 0; i < ctx->samplermap_count; i++)
        {
            if (ctx->samplermap[i].index == regnum)
            {
                ttype = cvtMojoToD3DSamplerType(ctx->samplermap[i].type);
                break;
            }
        }
    }

    item->index = (int)ttype;
    item->misc |= texbem;
}


static inline int writemask_xyzw(const int writemask)
{
    return (writemask == 0xF);  // 0xF == 1111. No explicit mask (full!).
}
static inline int writemask_xyz(const int writemask)
{
    return (writemask == 0x7);  // 0x7 == 0111. (that is: xyz)
}
static inline int writemask_xy(const int writemask)
{
    return (writemask == 0x3);  // 0x3 == 0011. (that is: xy)
}
static inline int writemask_x(const int writemask)
{
    return (writemask == 0x1);  // 0x1 == 0001. (that is: x)
}
static inline int writemask_y(const int writemask)
{
    return (writemask == 0x2);  // 0x1 == 0010. (that is: y)
}

static inline int replicate_swizzle(const int swizzle)
{
    return (((swizzle >> 0) & 0x3) == ((swizzle >> 2) & 0x3)) &&
           (((swizzle >> 2) & 0x3) == ((swizzle >> 4) & 0x3)) &&
           (((swizzle >> 4) & 0x3) == ((swizzle >> 6) & 0x3));
}
static inline int no_swizzle(const int swizzle)
{
    return (swizzle == 0xE4);  // 0xE4 == 11100100 ... 0 1 2 3. No swizzle.
}

static inline int vecsize_from_writemask(const int m)
{
    return (m&1) + ((m>>1)&1) + ((m>>2)&1) + ((m>>3)&1);
}


static inline void set_dstarg_writemask(DestArgInfo *dst, const int mask)
{
    dst->writemask = mask;
    dst->writemask0 = ((mask >> 0) & 1);
    dst->writemask1 = ((mask >> 1) & 1);
    dst->writemask2 = ((mask >> 2) & 1);
    dst->writemask3 = ((mask >> 3) & 1);
}


static inline void adjust_token_position(Context *ctx, const int incr)
{
    ctx->tokens += incr;
    ctx->tokencount -= incr;
    ctx->current_position += incr * sizeof(uint32);
}


// D3D stuff that's used in more than just the d3d profile...

static int isscalar(Context *ctx, const MOJOSHADER_shaderType shader_type,
                    const RegisterType rtype, const int rnum)
{
    const int uses_psize = ctx->uses_pointsize;
    const int uses_fog = ctx->uses_fog;
    if(rtype == REG_TYPE_OUTPUT && (uses_psize || uses_fog))
    {
        const RegisterList *reg = reglist_find(&ctx->attributes, rtype, rnum);
        if(reg != NULL)
        {
            const MOJOSHADER_usage usage = reg->usage;
            return (uses_psize && (usage == MOJOSHADER_USAGE_POINTSIZE)) ||
                   (uses_fog && (usage == MOJOSHADER_USAGE_FOG));
        }
    }

    return scalar_register(shader_type, rtype, rnum);
}

static const char swizzle_channels[] = { 'x', 'y', 'z', 'w' };


static const char *get_D3D_register_string(Context *ctx, RegisterType regtype,
                                           int regnum, char *regnum_str,
                                           size_t regnum_size)
{
    const char *retval = NULL;
    int has_number = 1;

    switch(regtype)
    {
        case REG_TYPE_TEMP:
            retval = "r";
            break;

        case REG_TYPE_INPUT:
            retval = "v";
            break;

        case REG_TYPE_CONST:
            retval = "c";
            break;

        case REG_TYPE_ADDRESS:  // (or REG_TYPE_TEXTURE, same value.)
            retval = shader_is_vertex(ctx) ? "a" : "t";
            break;

        case REG_TYPE_RASTOUT:
            switch((RastOutType)regnum)
            {
                case RASTOUT_TYPE_POSITION: retval = "oPos"; break;
                case RASTOUT_TYPE_FOG: retval = "oFog"; break;
                case RASTOUT_TYPE_POINT_SIZE: retval = "oPts"; break;
            }
            has_number = 0;
            break;

        case REG_TYPE_ATTROUT:
            retval = "oD";
            break;

        case REG_TYPE_OUTPUT: // (or REG_TYPE_TEXCRDOUT, same value.)
            if(shader_is_vertex(ctx) && shader_version_atleast(ctx, 3, 0))
                retval = "o";
            else
                retval = "oT";
            break;

        case REG_TYPE_CONSTINT:
            retval = "i";
            break;

        case REG_TYPE_COLOROUT:
            retval = "oC";
            break;

        case REG_TYPE_DEPTHOUT:
            retval = "oDepth";
            has_number = 0;
            break;

        case REG_TYPE_SAMPLER:
            retval = "s";
            break;

        case REG_TYPE_CONSTBOOL:
            retval = "b";
            break;

        case REG_TYPE_LOOP:
            retval = "aL";
            has_number = 0;
            break;

        case REG_TYPE_MISCTYPE:
            switch((const MiscTypeType)regnum)
            {
                case MISCTYPE_TYPE_POSITION: retval = "vPos"; break;
                case MISCTYPE_TYPE_FACE: retval = "vFace"; break;
            }
            has_number = 0;
            break;

        case REG_TYPE_LABEL:
            retval = "l";
            break;

        case REG_TYPE_PREDICATE:
            retval = "p";
            break;

        //case REG_TYPE_TEMPFLOAT16:  // !!! FIXME: don't know this asm string
        default:
            fail(ctx, "unknown register type");
            retval = "???";
            has_number = 0;
            break;
    }

    if (has_number)
        snprintf(regnum_str, regnum_size, "%u", (uint) regnum);
    else
        regnum_str[0] = '\0';

    return retval;
}


// !!! FIXME: can we split the profile code out to separate source files?

#define AT_LEAST_ONE_PROFILE 0

#if SUPPORT_PROFILE_GLSL
#undef AT_LEAST_ONE_PROFILE
#define AT_LEAST_ONE_PROFILE 1
#define PROFILE_EMITTER_GLSL(op) emit_GLSL_##op,

#define EMIT_GLSL_OPCODE_UNIMPLEMENTED_FUNC(op) \
    static void emit_GLSL_##op(Context *ctx) { \
        fail(ctx, #op " unimplemented in glsl profile"); \
    }

static inline const char *get_GLSL_register_string(Context *ctx,
                        const RegisterType regtype, const int regnum,
                        char *regnum_str, const size_t regnum_size)
{
    // turns out these are identical at the moment.
    return get_D3D_register_string(ctx,regtype,regnum,regnum_str,regnum_size);
} // get_GLSL_register_string

static const char *get_GLSL_uniform_type(Context *ctx, const RegisterType rtype)
{
    switch(rtype)
    {
        case REG_TYPE_CONST: return "vec4";
        case REG_TYPE_CONSTINT: return "ivec4";
        case REG_TYPE_CONSTBOOL: return "bool";
        default: break;
    }

    fail(ctx, "BUG: used a uniform we don't know how to define.");
    return NULL;
} // get_GLSL_uniform_type

static const char *get_GLSL_varname_in_buf(Context *ctx, RegisterType rt,
                                           int regnum, char *buf,
                                           const size_t len)
{
    char regnum_str[16];
    const char *regtype_str = get_GLSL_register_string(ctx, rt, regnum, regnum_str, sizeof(regnum_str));
    snprintf(buf,len,"%s_%s%s", ctx->shader_type_str, regtype_str, regnum_str);
    return buf;
} // get_GLSL_varname_in_buf


static const char *get_GLSL_varname(Context *ctx, RegisterType rt, int regnum)
{
    char buf[64];
    get_GLSL_varname_in_buf(ctx, rt, regnum, buf, sizeof(buf));
    return strdup(buf);
}


static inline const char *get_GLSL_input_array_varname(char *buf, const size_t buflen)
{
    snprintf(buf, buflen, "%s", "vertex_input_array");
    return buf;
} // get_GLSL_input_array_varname


static const char *get_GLSL_uniform_array_varname(Context *ctx,
                                                  const RegisterType regtype,
                                                  char *buf, const size_t len)
{
    const char *shadertype = ctx->shader_type_str;
    const char *type = get_GLSL_uniform_type(ctx, regtype);
    snprintf(buf, len, "%s_uniforms_%s", shadertype, type);
    return buf;
} // get_GLSL_uniform_array_varname

static const char *get_GLSL_destarg_varname(Context *ctx, char *buf, size_t len)
{
    const DestArgInfo *arg = &ctx->dest_arg;
    return get_GLSL_varname_in_buf(ctx, arg->regtype, arg->regnum, buf, len);
} // get_GLSL_destarg_varname

static const char *get_GLSL_srcarg_varname(Context *ctx, const size_t idx,
                                           char *buf, size_t len)
{
    if (idx >= STATICARRAYLEN(ctx->source_args))
    {
        fail(ctx, "Too many source args");
        *buf = '\0';
        return buf;
    } // if

    const SourceArgInfo *arg = &ctx->source_args[idx];
    return get_GLSL_varname_in_buf(ctx, arg->regtype, arg->regnum, buf, len);
} // get_GLSL_srcarg_varname


static const char *make_GLSL_destarg_assign(Context *, char *, const size_t,
                                            const char *, ...) ISPRINTF(4,5);

static const char *make_GLSL_destarg_assign(Context *ctx, char *buf,
                                            const size_t buflen,
                                            const char *fmt, ...)
{
    int need_parens = 0;
    const DestArgInfo *arg = &ctx->dest_arg;

    if (arg->writemask == 0)
    {
        *buf = '\0';
        return buf;  // no writemask? It's a no-op.
    }

    char clampbuf[32] = { '\0' };
    const char *clampleft = "";
    const char *clampright = "";
    if (arg->result_mod & MOD_SATURATE)
    {
        const int vecsize = vecsize_from_writemask(arg->writemask);
        clampleft = "clamp(";
        if (vecsize == 1)
            clampright = ", 0.0, 1.0)";
        else
        {
            snprintf(clampbuf, sizeof (clampbuf),
                     ", vec%d(0.0), vec%d(1.0))", vecsize, vecsize);
            clampright = clampbuf;
        }
    }

    // MSDN says MOD_PP is a hint and many implementations ignore it. So do we.

    // CENTROID only allowed in DCL opcodes, which shouldn't come through here.
    assert((arg->result_mod & MOD_CENTROID) == 0);

    if (ctx->predicated)
    {
        fail(ctx, "predicated destinations unsupported");  // !!! FIXME
        *buf = '\0';
        return buf;
    }

    char operation[256];
    va_list ap;
    va_start(ap, fmt);
    const size_t len = vsnprintf(operation, sizeof (operation), fmt, ap);
    va_end(ap);
    if (len >= sizeof (operation))
    {
        fail(ctx, "operation string too large");  // I'm lazy.  :P
        *buf = '\0';
        return buf;
    }

    const char *result_shift_str = "";
    switch (arg->result_shift)
    {
        case 0x1: result_shift_str = " * 2.0"; break;
        case 0x2: result_shift_str = " * 4.0"; break;
        case 0x3: result_shift_str = " * 8.0"; break;
        case 0xD: result_shift_str = " / 8.0"; break;
        case 0xE: result_shift_str = " / 4.0"; break;
        case 0xF: result_shift_str = " / 2.0"; break;
    }
    need_parens |= (result_shift_str[0] != '\0');

    char regnum_str[16];
    const char *regtype_str = get_GLSL_register_string(ctx, arg->regtype,
                                                       arg->regnum, regnum_str,
                                                       sizeof (regnum_str));
    char writemask_str[6];
    size_t i = 0;
    const int scalar = isscalar(ctx, ctx->shader_type, arg->regtype, arg->regnum);
    if (!scalar && !writemask_xyzw(arg->writemask))
    {
        writemask_str[i++] = '.';
        if (arg->writemask0) writemask_str[i++] = 'x';
        if (arg->writemask1) writemask_str[i++] = 'y';
        if (arg->writemask2) writemask_str[i++] = 'z';
        if (arg->writemask3) writemask_str[i++] = 'w';
    }
    writemask_str[i] = '\0';
    assert(i < sizeof (writemask_str));

    const char *leftparen = (need_parens) ? "(" : "";
    const char *rightparen = (need_parens) ? ")" : "";

    snprintf(buf, buflen, "%s_%s%s%s = %s%s%s%s%s%s;",
             ctx->shader_type_str, regtype_str, regnum_str, writemask_str,
             clampleft, leftparen, operation, rightparen, result_shift_str,
             clampright);
    // !!! FIXME: make sure the scratch buffer was large enough.
    return buf;
}


static char *make_GLSL_swizzle_string(char *swiz_str, const int swizzle, const int writemask)
{
    size_t i = 0;
    if(!no_swizzle(swizzle) || !writemask_xyzw(writemask))
    {
        const int writemask0 = (writemask >> 0) & 0x1;
        const int writemask1 = (writemask >> 1) & 0x1;
        const int writemask2 = (writemask >> 2) & 0x1;
        const int writemask3 = (writemask >> 3) & 0x1;

        const int swizzle_x = (swizzle >> 0) & 0x3;
        const int swizzle_y = (swizzle >> 2) & 0x3;
        const int swizzle_z = (swizzle >> 4) & 0x3;
        const int swizzle_w = (swizzle >> 6) & 0x3;

        swiz_str[i++] = '.';
        if (writemask0) swiz_str[i++] = swizzle_channels[swizzle_x];
        if (writemask1) swiz_str[i++] = swizzle_channels[swizzle_y];
        if (writemask2) swiz_str[i++] = swizzle_channels[swizzle_z];
        if (writemask3) swiz_str[i++] = swizzle_channels[swizzle_w];
    }
    swiz_str[i] = '\0';
    return swiz_str;
}


static const char *make_GLSL_srcarg_string(Context *ctx, const size_t idx,
                                           const int writemask, char *buf,
                                           const size_t buflen)
{
    *buf = '\0';

    if(idx >= STATICARRAYLEN(ctx->source_args))
    {
        fail(ctx, "Too many source args");
        return buf;
    }

    const SourceArgInfo *arg = &ctx->source_args[idx];

    const char *premod_str = "";
    const char *postmod_str = "";
    switch (arg->src_mod)
    {
        case SRCMOD_NEGATE:
            premod_str = "-";
            break;

        case SRCMOD_BIASNEGATE:
            premod_str = "-(";
            postmod_str = " - 0.5)";
            break;

        case SRCMOD_BIAS:
            premod_str = "(";
            postmod_str = " - 0.5)";
            break;

        case SRCMOD_SIGNNEGATE:
            premod_str = "-((";
            postmod_str = " - 0.5) * 2.0)";
            break;

        case SRCMOD_SIGN:
            premod_str = "((";
            postmod_str = " - 0.5) * 2.0)";
            break;

        case SRCMOD_COMPLEMENT:
            premod_str = "(1.0 - ";
            postmod_str = ")";
            break;

        case SRCMOD_X2NEGATE:
            premod_str = "-(";
            postmod_str = " * 2.0)";
            break;

        case SRCMOD_X2:
            premod_str = "(";
            postmod_str = " * 2.0)";
            break;

        case SRCMOD_DZ:
            fail(ctx, "SRCMOD_DZ unsupported"); return buf; // !!! FIXME
            postmod_str = "_dz";
            break;

        case SRCMOD_DW:
            fail(ctx, "SRCMOD_DW unsupported"); return buf; // !!! FIXME
            postmod_str = "_dw";
            break;

        case SRCMOD_ABSNEGATE:
            premod_str = "-abs(";
            postmod_str = ")";
            break;

        case SRCMOD_ABS:
            premod_str = "abs(";
            postmod_str = ")";
            break;

        case SRCMOD_NOT:
            premod_str = "!";
            break;

        case SRCMOD_NONE:
        case SRCMOD_TOTAL:
             break;
    }

    char regtype_str[64] = { '\0' };
    const char *rel_lbracket = "";
    char rel_offset[32] = { '\0' };
    const char *rel_rbracket = "";
    char rel_swizzle[4] = { '\0' };
    char rel_regtype_str[64] = { '\0' };
    if(!arg->relative)
    {
        get_GLSL_varname_in_buf(ctx, arg->regtype, arg->regnum,
                                regtype_str, sizeof(regtype_str));
    }
    else
    {
        if (arg->regtype == REG_TYPE_INPUT)
            get_GLSL_input_array_varname(regtype_str, sizeof(regtype_str));
        else
        {
            assert(arg->regtype == REG_TYPE_CONST);

            const int offset = arg->regnum;
            assert(offset >= 0);

            get_GLSL_uniform_array_varname(ctx, arg->regtype, regtype_str, sizeof(regtype_str));
            if(offset == 0)
                rel_offset[0] = '\0';
            else
                snprintf(rel_offset, sizeof(rel_offset), "%d + ", offset);
        }

        rel_lbracket = "[";
        get_GLSL_varname_in_buf(ctx, arg->relative_regtype, arg->relative_regnum,
                                rel_regtype_str, sizeof(rel_regtype_str));
        rel_swizzle[0] = '.';
        rel_swizzle[1] = swizzle_channels[arg->relative_component];
        rel_swizzle[2] = '\0';
        rel_rbracket = "]";
    }

    char swiz_str[6] = { '\0' };
    if (!isscalar(ctx, ctx->shader_type, arg->regtype, arg->regnum))
        make_GLSL_swizzle_string(swiz_str, arg->swizzle, writemask);

    snprintf(buf, buflen, "%s%s%s%s%s%s%s%s%s",
             premod_str, regtype_str, rel_lbracket, rel_offset,
             rel_regtype_str, rel_swizzle, rel_rbracket, swiz_str,
             postmod_str);
    // !!! FIXME: make sure the scratch buffer was large enough.
    return buf;
}

// generate some convenience functions.
#define MAKE_GLSL_SRCARG_STRING_(mask, bitmask) \
    static inline const char *make_GLSL_srcarg_string_##mask(Context *ctx, \
                                                const size_t idx, char *buf, \
                                                const size_t buflen) { \
        return make_GLSL_srcarg_string(ctx, idx, bitmask, buf, buflen); \
    }
MAKE_GLSL_SRCARG_STRING_(x, (1 << 0))
MAKE_GLSL_SRCARG_STRING_(y, (1 << 1))
MAKE_GLSL_SRCARG_STRING_(z, (1 << 2))
MAKE_GLSL_SRCARG_STRING_(w, (1 << 3))
MAKE_GLSL_SRCARG_STRING_(scalar, (1 << 0))
MAKE_GLSL_SRCARG_STRING_(full, 0xF)
MAKE_GLSL_SRCARG_STRING_(masked, ctx->dest_arg.writemask)
MAKE_GLSL_SRCARG_STRING_(vec3, 0x7)
MAKE_GLSL_SRCARG_STRING_(vec2, 0x3)
#undef MAKE_GLSL_SRCARG_STRING_

// special cases for comparison opcodes...

static const char *get_GLSL_comparison_string_scalar(Context *ctx)
{
    static const char *comps[] = { "", ">", "==", ">=", "<", "!=", "<=" };
    if (ctx->instruction_controls >= STATICARRAYLEN(comps))
    {
        fail(ctx, "unknown comparison control");
        return "";
    } // if

    return comps[ctx->instruction_controls];
} // get_GLSL_comparison_string_scalar

static const char *get_GLSL_comparison_string_vector(Context *ctx)
{
    static const char *comps[] = {
        "", "greaterThan", "equal", "greaterThanEqual", "lessThan",
        "notEqual", "lessThanEqual"
    };

    if (ctx->instruction_controls >= STATICARRAYLEN(comps))
    {
        fail(ctx, "unknown comparison control");
        return "";
    } // if

    return comps[ctx->instruction_controls];
} // get_GLSL_comparison_string_vector


static void emit_GLSL_start(Context *ctx, const char *profilestr)
{
    if(!shader_is_vertex(ctx) && !shader_is_pixel(ctx))
    {
        failf(ctx, "Shader type %u unsupported in this profile.",
              (uint)ctx->shader_type);
        return;
    }

    if(strcmp(profilestr, MOJOSHADER_PROFILE_GLSL330) != 0)
    {
        failf(ctx, "Profile '%s' unsupported or unknown.", profilestr);
        return;
    }

    push_output(ctx, &ctx->preflight);
    output_line(ctx, "#version 330");
    output_line(ctx, "#extension GL_ARB_separate_shader_objects : enable");
    pop_output(ctx);

    push_output(ctx, &ctx->globals);
    if(shader_is_vertex(ctx))
    {
        // NOTE: POS_FIXUP to fix D3D/GL projection differences
        output_line(ctx, "layout(std140) uniform pos_fixup { vec4 POS_FIXUP; };");
        output_line(ctx, "layout(std140) uniform vertex_state {\n"
                         "\tvec4 ClipPlane[8];\n"
                         "};");
        output_line(ctx, "out gl_PerVertex {\n"
                         "\tvec4 gl_Position;\n"
                         "\tfloat gl_PointSize;\n"
                         "\tfloat gl_ClipDistance[8];\n"
                         "};");
        output_line(ctx, "layout(location=0) out vec4 vs_o[10];");
    }
    else if(shader_is_pixel(ctx))
        output_line(ctx, "layout(location=0) in vec4 ps_v[10];");
    pop_output(ctx);

    push_output(ctx, &ctx->mainline_intro);
    output_line(ctx, "void main()");
    output_line(ctx, "{");
    pop_output(ctx);

    set_output(ctx, &ctx->mainline);
    ctx->indent++;
}

static void emit_GLSL_RET(Context *ctx);
static void emit_GLSL_end(Context *ctx)
{
    // ps_1_* writes color to r0 instead oC0. We move it to the right place.
    // We don't have to worry about a RET opcode messing this up, since
    //  RET isn't available before ps_2_0.
    if(shader_is_pixel(ctx) && !shader_version_atleast(ctx, 2, 0))
    {
        const char *shstr = ctx->shader_type_str;
        set_used_register(ctx, REG_TYPE_COLOROUT, 0, 1);
        output_line(ctx, "%s_oC0 = %s_r0;", shstr, shstr);
    }
    else if(shader_is_vertex(ctx))
    {
        output_blank_line(ctx);
        output_line(ctx, "for(int i = 0;i < ClipPlane.length();++i)");
        ctx->indent++;
            output_line(ctx, "gl_ClipDistance[i] = dot(gl_Position, ClipPlane[i]);");
        ctx->indent--;
        // NOTE: Fixup the vertex position (offset X, flip+offset Y, scale+offset Z)
        output_line(ctx, "gl_Position.xy = gl_Position.xy*vec2(1.0,-1.0) + POS_FIXUP.xy*gl_Position.ww;");
        output_line(ctx, "gl_Position.z = gl_Position.z*2.0 - gl_Position.w;");
        output_line(ctx, "gl_Position.w = gl_Position.w;");
    }
    // force a RET opcode if we're at the end of the stream without one.
    if(ctx->previous_opcode != OPCODE_RET)
        emit_GLSL_RET(ctx);
}

static void emit_GLSL_phase(Context *ctx)
{
    // no-op in GLSL.
    (void)ctx;
} // emit_GLSL_phase

static void output_GLSL_uniform_array(Context *ctx, const RegisterType regtype,
                                      const int size)
{
    if (size > 0)
    {
        const char *pre="uniform ", *post="";
        char buf[64];
        get_GLSL_uniform_array_varname(ctx, regtype, buf, sizeof (buf));
        if(shader_is_pixel(ctx))
        {
            switch (regtype)
            {
                case REG_TYPE_CONST:
                    pre = "layout(std140) uniform ps_vec4 { ";
                    post = " };"; break;
                case REG_TYPE_CONSTINT:
                    pre = "layout(std140) uniform ps_ivec4 { ";
                    post = " };"; break;
                case REG_TYPE_CONSTBOOL:
                    pre = "layout(std140) uniform ps_bool { ";
                    post = " };"; break;
                default: fail(ctx, "BUG: used a uniform we don't know how to define.");
            }
        }
        else if(shader_is_vertex(ctx))
        {
            switch (regtype)
            {
                case REG_TYPE_CONST:
                    pre = "layout(std140) uniform vs_vec4 { ";
                    post = " };"; break;
                case REG_TYPE_CONSTINT:
                    pre = "layout(std140) uniform vs_ivec4 { ";
                    post = " };"; break;
                case REG_TYPE_CONSTBOOL:
                    pre = "layout(std140) uniform vs_bool { ";
                    post = " };"; break;
                default: fail(ctx, "BUG: used a uniform we don't know how to define.");
            }
        }
        const char *type = get_GLSL_uniform_type(ctx, regtype);
        output_line(ctx, "%s%s %s[%d];%s", pre, type, buf, size, post);
    }
} // output_GLSL_uniform_array

static void emit_GLSL_finalize(Context *ctx)
{
    // throw some blank lines around to make source more readable.
    push_output(ctx, &ctx->globals);
    output_blank_line(ctx);
    pop_output(ctx);

    // If we had a relative addressing of REG_TYPE_INPUT, we need to build
    //  an array for it at the start of main(). GLSL doesn't let you specify
    //  arrays of attributes.
    //vec4 blah_array[BIGGEST_ARRAY];
    if (ctx->have_relative_input_registers) // !!! FIXME
        fail(ctx, "Relative addressing of input registers not supported.");

    push_output(ctx, &ctx->preflight);
    output_blank_line(ctx);
    output_GLSL_uniform_array(ctx, REG_TYPE_CONST, ctx->uniform_float4_count);
    output_GLSL_uniform_array(ctx, REG_TYPE_CONSTINT, ctx->uniform_int4_count);
    output_GLSL_uniform_array(ctx, REG_TYPE_CONSTBOOL, ctx->uniform_bool_count);
    pop_output(ctx);
} // emit_GLSL_finalize

static void emit_GLSL_global(Context *ctx, RegisterType regtype, int regnum)
{
    char varname[64];
    get_GLSL_varname_in_buf(ctx, regtype, regnum, varname, sizeof (varname));

    push_output(ctx, &ctx->globals);
    switch (regtype)
    {
        case REG_TYPE_ADDRESS:
            if (shader_is_vertex(ctx))
                output_line(ctx, "ivec4 %s;", varname);
            else if (shader_is_pixel(ctx))  // actually REG_TYPE_TEXTURE.
            {
                // We have to map texture registers to temps for ps_1_1, since
                //  they work like temps, initialize with tex coords, and the
                //  ps_1_1 TEX opcode expects to overwrite it.
                if (!shader_version_atleast(ctx, 1, 4))
                    output_line(ctx, "vec4 %s = ps_v[%d];", varname, regnum);
            }
            break;
        case REG_TYPE_PREDICATE:
            output_line(ctx, "bvec4 %s;", varname);
            break;
        case REG_TYPE_TEMP:
            output_line(ctx, "vec4 %s;", varname);
            break;
        case REG_TYPE_LOOP:
            break; // no-op. We declare these in for loops at the moment.
        case REG_TYPE_LABEL:
            break; // no-op. If we see it here, it means we optimized it out.
        default:
            fail(ctx, "BUG: we used a register we don't know how to define.");
            break;
    }
    pop_output(ctx);
} // emit_GLSL_global

static void emit_GLSL_uniform(Context *ctx, RegisterType regtype, int regnum)
{
    char varname[64];
    char name[64];

    get_GLSL_varname_in_buf(ctx, regtype, regnum, varname, sizeof (varname));

    push_output(ctx, &ctx->globals);
    get_GLSL_uniform_array_varname(ctx, regtype, name, sizeof(name));
    output_line(ctx, "#define %s %s[%d]", varname, name, regnum);
    pop_output(ctx);
}

static void emit_GLSL_sampler(Context *ctx,int stage,TextureType ttype,int tb)
{
    const char *type = "";
    if(!(ctx->shadow_samplers&(1<<stage)))
    {
        switch (ttype)
        {
            case TEXTURE_TYPE_2D: type = "sampler2D"; break;
            case TEXTURE_TYPE_CUBE: type = "samplerCube"; break;
            case TEXTURE_TYPE_VOLUME: type = "sampler3D"; break;
            default: fail(ctx, "BUG: used a sampler we don't know how to define.");
        }
    }
    else
    {
        switch (ttype)
        {
            case TEXTURE_TYPE_2D: type = "sampler2DShadow"; break;
            case TEXTURE_TYPE_CUBE: type = "samplerCubeShadow"; break;
            case TEXTURE_TYPE_VOLUME: type = "sampler3DShadow"; break;
            default: fail(ctx, "BUG: used a sampler we don't know how to define.");
        }
    }

    char var[64];
    get_GLSL_varname_in_buf(ctx, REG_TYPE_SAMPLER, stage, var, sizeof (var));

    push_output(ctx, &ctx->globals);
    output_line(ctx, "uniform %s %s;", type, var);
    if (tb)  // This sampler used a ps_1_1 TEXBEM opcode?
    {
        char name[64];
        const int index = ctx->uniform_float4_count;
        ctx->uniform_float4_count += 2;
        get_GLSL_uniform_array_varname(ctx, REG_TYPE_CONST, name, sizeof (name));
        output_line(ctx, "#define %s_texbem %s[%d]", var, name, index);
        output_line(ctx, "#define %s_texbeml %s[%d]", var, name, index+1);
    } // if
    pop_output(ctx);
} // emit_GLSL_sampler

static void emit_GLSL_attribute(Context *ctx, RegisterType regtype, int regnum,
                                MOJOSHADER_usage usage, int index, int wmask,
                                int flags)
{
    // !!! FIXME: this function doesn't deal with write masks at all yet!
    char var[64];
    (void)wmask;

    //assert((flags & MOD_PP) == 0);  // !!! FIXME: is PP allowed?

    get_GLSL_varname_in_buf(ctx, regtype, regnum, var, sizeof(var));

    if (shader_is_vertex(ctx))
    {
        // pre-vs3 output registers.
        // these don't ever happen in DCL opcodes, I think. Map to vs_3_*
        //  output registers.
        if (!shader_version_atleast(ctx, 3, 0))
        {
            if (regtype == REG_TYPE_RASTOUT)
            {
                regtype = REG_TYPE_OUTPUT;
                index = regnum;
                switch ((const RastOutType) regnum)
                {
                    case RASTOUT_TYPE_POSITION:
                        usage = MOJOSHADER_USAGE_POSITION;
                        break;
                    case RASTOUT_TYPE_FOG:
                        usage = MOJOSHADER_USAGE_FOG;
                        break;
                    case RASTOUT_TYPE_POINT_SIZE:
                        usage = MOJOSHADER_USAGE_POINTSIZE;
                        break;
                }
            }
            else if (regtype == REG_TYPE_ATTROUT)
            {
                regtype = REG_TYPE_OUTPUT;
                usage = MOJOSHADER_USAGE_COLOR;
                index = regnum;
            }
            else if (regtype == REG_TYPE_TEXCRDOUT)
            {
                regtype = REG_TYPE_OUTPUT;
                usage = MOJOSHADER_USAGE_TEXCOORD;
                index = regnum;
            }
        }

        // to avoid limitations of various GL entry points for input
        // attributes (glSecondaryColorPointer() can only take 3 component
        // items, glVertexPointer() can't do GL_UNSIGNED_BYTE, many other
        // issues), we set up all inputs as generic vertex attributes, so we
        // can pass data in just about any form, and ignore the built-in GLSL
        // attributes like gl_SecondaryColor.

        push_output(ctx, &ctx->globals);
        if (regtype == REG_TYPE_INPUT)
            output_line(ctx, "in vec4 %s; /* usage %s, index %i */", var, MOJOSHADER_usage_to_string(usage), index);
        else if (regtype == REG_TYPE_OUTPUT)
        {
            switch (usage)
            {
                case MOJOSHADER_USAGE_POSITION:
                    output_line(ctx, "#define %s gl_Position", var);
                    break;
                case MOJOSHADER_USAGE_POINTSIZE:
                    output_line(ctx, "#define %s gl_PointSize", var);
                    break;
                case MOJOSHADER_USAGE_FOG:
                    output_line(ctx, "float FogFragCoord; /* FIXME: unused */");
                    output_line(ctx, "#define %s FogFragCoord", var);
                    break;
                // For these, we can declare outputs from vertex and inputs to
                // pixel shaders using a layout location index corresponding to
                // the register number. For vs<3.0, TEXCOORD 0...7 and COLOR 8+9
                case MOJOSHADER_USAGE_TEXCOORD:
                    output_line(ctx, "#define %s vs_o[%u]", var, (uint)index);
                    break;
                case MOJOSHADER_USAGE_COLOR:
                    if(index < 0 || index > 1)
                        failf(ctx, "Unhandled vertex output color index: %d\n", index);
                    output_line(ctx, "#define %s vs_o[%u]", var, 8u+index);
                    break;
                default:
                    // !!! FIXME: we need to deal with some more built-in varyings here.
                    failf(ctx, "Unhandled vertex output register usage: 0x%x\n", usage);
                    break;
            }
        }
        else
        {
            fail(ctx, "unknown vertex shader attribute register");
        }
        pop_output(ctx);
    }
    else if (shader_is_pixel(ctx))
    {
        // samplers DCLs get handled in emit_GLSL_sampler().

        if (flags & MOD_CENTROID)  // !!! FIXME
        {
            failf(ctx, "centroid unsupported in %s profile", ctx->profile->name);
            return;
        }

        push_output(ctx, &ctx->globals);
        if (regtype == REG_TYPE_COLOROUT)
            output_line(ctx, "layout(location=%u) out vec4 %s;", (uint)regnum, var);
        else if (regtype == REG_TYPE_DEPTHOUT)
            output_line(ctx, "#define %s gl_FragDepth", var);
        else if(regtype == REG_TYPE_TEXTURE || regtype == REG_TYPE_INPUT)
        {
            // !!! FIXME: can you actualy have a texture register with COLOR usage?
            if (usage == MOJOSHADER_USAGE_TEXCOORD)
            {
                // ps_1_1 does a different hack for this attribute.
                //  Refer to emit_GLSL_global()'s REG_TYPE_TEXTURE code.
                if (shader_version_atleast(ctx, 1, 4))
                    output_line(ctx, "#define %s ps_v[%u]", var, (uint)index);
            }
            else if (usage == MOJOSHADER_USAGE_COLOR)
            {
                if(index < 0 || index > 1)
                    fail(ctx, "unsupported color index");
                output_line(ctx, "#define %s ps_v[%u]", var, 8u+index);
            }
            else
            {
                fail(ctx, "unhandled texture/input usage");
            }
        }
        else if (regtype == REG_TYPE_MISCTYPE)
        {
            const MiscTypeType mt = (MiscTypeType) regnum;
            if (mt == MISCTYPE_TYPE_FACE)
                output_line(ctx, "float %s = gl_FrontFacing ? 1.0 : -1.0;", var);
            else if (mt == MISCTYPE_TYPE_POSITION)
            {
                output_line(ctx, "layout(pixel_center_integer) in vec4 gl_FragCoord;");
                output_line(ctx, "#define %s gl_FragCoord", var);
            }
            else
            {
                fail(ctx, "BUG: unhandled misc register");
            }
        }
        else
        {
            fail(ctx, "unknown pixel shader attribute register");
        }
        pop_output(ctx);
    }
    else
    {
        fail(ctx, "Unknown shader type");  // state machine should catch this.
    }
}

static void emit_GLSL_NOP(Context *ctx)
{
    // no-op is a no-op.  :)
    (void)ctx;
} // emit_GLSL_NOP

static void emit_GLSL_MOV(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "%s", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_MOV

static void emit_GLSL_ADD(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "%s + %s", src0, src1);
    output_line(ctx, "%s", code);
} // emit_GLSL_ADD

static void emit_GLSL_SUB(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "%s - %s", src0, src1);
    output_line(ctx, "%s", code);
} // emit_GLSL_SUB

static void emit_GLSL_MAD(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char src2[64]; make_GLSL_srcarg_string_masked(ctx, 2, src2, sizeof (src2));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "(%s * %s) + %s", src0, src1, src2);
    output_line(ctx, "%s", code);
} // emit_GLSL_MAD

static void emit_GLSL_MUL(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "%s * %s", src0, src1);
    output_line(ctx, "%s", code);
} // emit_GLSL_MUL

static void emit_GLSL_RCP(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "1.0 / %s", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_RCP

static void emit_GLSL_RSQ(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "inversesqrt(%s)", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_RSQ

static void emit_GLSL_dotprod(Context *ctx, const char *src0, const char *src1,
                              const char *extra)
{
    const int vecsize = vecsize_from_writemask(ctx->dest_arg.writemask);
    char castleft[16] = { '\0' };
    const char *castright = "";
    if (vecsize != 1)
    {
        snprintf(castleft, sizeof (castleft), "vec%d(", vecsize);
        castright = ")";
    } // if

    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "%sdot(%s, %s)%s%s",
                             castleft, src0, src1, extra, castright);
    output_line(ctx, "%s", code);
} // emit_GLSL_dotprod

static void emit_GLSL_DP3(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_vec3(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_vec3(ctx, 1, src1, sizeof (src1));
    emit_GLSL_dotprod(ctx, src0, src1, "");
} // emit_GLSL_DP3

static void emit_GLSL_DP4(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_full(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_full(ctx, 1, src1, sizeof (src1));
    emit_GLSL_dotprod(ctx, src0, src1, "");
} // emit_GLSL_DP4

static void emit_GLSL_MIN(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "min(%s, %s)", src0, src1);
    output_line(ctx, "%s", code);
} // emit_GLSL_MIN

static void emit_GLSL_MAX(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "max(%s, %s)", src0, src1);
    output_line(ctx, "%s", code);
} // emit_GLSL_MAX

static void emit_GLSL_SLT(Context *ctx)
{
    const int vecsize = vecsize_from_writemask(ctx->dest_arg.writemask);
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char code[128];

    // float(bool) or vec(bvec) results in 0.0 or 1.0, like SLT wants.
    if (vecsize == 1)
        make_GLSL_destarg_assign(ctx, code, sizeof (code), "float(%s < %s)", src0, src1);
    else
    {
        make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                 "vec%d(lessThan(%s, %s))",
                                 vecsize, src0, src1);
    }
    output_line(ctx, "%s", code);
} // emit_GLSL_SLT

static void emit_GLSL_SGE(Context *ctx)
{
    const int vecsize = vecsize_from_writemask(ctx->dest_arg.writemask);
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char code[128];

    // float(bool) or vec(bvec) results in 0.0 or 1.0, like SGE wants.
    if (vecsize == 1)
    {
        make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                 "float(%s >= %s)", src0, src1);
    }
    else
    {
        make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                 "vec%d(greaterThanEqual(%s, %s))",
                                 vecsize, src0, src1);
    }
    output_line(ctx, "%s", code);
} // emit_GLSL_SGE

static void emit_GLSL_EXP(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "exp2(%s)", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_EXP

static void emit_GLSL_LOG(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "log2(%s)", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_LOG

static void emit_GLSL_LIT_helper(Context *ctx)
{
    const char *maxp = "127.9961"; // value from the dx9 reference.

    if (ctx->glsl_generated_lit_helper)
        return;

    ctx->glsl_generated_lit_helper = 1;

    push_output(ctx, &ctx->helpers);
    output_line(ctx, "vec4 LIT(const vec4 src)");
    output_line(ctx, "{"); ctx->indent++;
    output_line(ctx,   "float power = clamp(src.w, -%s, %s);",maxp,maxp);
    output_line(ctx,   "vec4 retval = vec4(1.0, 0.0, 0.0, 1.0);");
    output_line(ctx,   "if (src.x > 0.0) {"); ctx->indent++;
    output_line(ctx,     "retval.y = src.x;");
    output_line(ctx,     "if (src.y > 0.0) {"); ctx->indent++;
    output_line(ctx,       "retval.z = pow(src.y, power);"); ctx->indent--;
    output_line(ctx,     "}"); ctx->indent--;
    output_line(ctx,   "}");
    output_line(ctx,   "return retval;"); ctx->indent--;
    output_line(ctx, "}");
    output_blank_line(ctx);
    pop_output(ctx);
} // emit_GLSL_LIT_helper

static void emit_GLSL_LIT(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_full(ctx, 0, src0, sizeof (src0));
    char code[128];
    emit_GLSL_LIT_helper(ctx);
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "LIT(%s)", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_LIT

static void emit_GLSL_DST(Context *ctx)
{
    // !!! FIXME: needs to take ctx->dst_arg.writemask into account.
    char src0_y[64]; make_GLSL_srcarg_string_y(ctx, 0, src0_y, sizeof (src0_y));
    char src1_y[64]; make_GLSL_srcarg_string_y(ctx, 1, src1_y, sizeof (src1_y));
    char src0_z[64]; make_GLSL_srcarg_string_z(ctx, 0, src0_z, sizeof (src0_z));
    char src1_w[64]; make_GLSL_srcarg_string_w(ctx, 1, src1_w, sizeof (src1_w));

    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code),
                             "vec4(1.0, %s * %s, %s, %s)",
                             src0_y, src1_y, src0_z, src1_w);
    output_line(ctx, "%s", code);
} // emit_GLSL_DST

static void emit_GLSL_LRP(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char src2[64]; make_GLSL_srcarg_string_masked(ctx, 2, src2, sizeof (src2));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "mix(%s, %s, %s)",
                             src2, src1, src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_LRP

static void emit_GLSL_FRC(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "fract(%s)", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_FRC

static void emit_GLSL_M4X4(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_full(ctx, 0, src0, sizeof (src0));
    char row0[64]; make_GLSL_srcarg_string_full(ctx, 1, row0, sizeof (row0));
    char row1[64]; make_GLSL_srcarg_string_full(ctx, 2, row1, sizeof (row1));
    char row2[64]; make_GLSL_srcarg_string_full(ctx, 3, row2, sizeof (row2));
    char row3[64]; make_GLSL_srcarg_string_full(ctx, 4, row3, sizeof (row3));
    char code[256];
    make_GLSL_destarg_assign(ctx, code, sizeof (code),
                    "vec4(dot(%s, %s), dot(%s, %s), dot(%s, %s), dot(%s, %s))",
                    src0, row0, src0, row1, src0, row2, src0, row3);
    output_line(ctx, "%s", code);
} // emit_GLSL_M4X4

static void emit_GLSL_M4X3(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_full(ctx, 0, src0, sizeof (src0));
    char row0[64]; make_GLSL_srcarg_string_full(ctx, 1, row0, sizeof (row0));
    char row1[64]; make_GLSL_srcarg_string_full(ctx, 2, row1, sizeof (row1));
    char row2[64]; make_GLSL_srcarg_string_full(ctx, 3, row2, sizeof (row2));
    char code[256];
    make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                "vec3(dot(%s, %s), dot(%s, %s), dot(%s, %s))",
                                src0, row0, src0, row1, src0, row2);
    output_line(ctx, "%s", code);
} // emit_GLSL_M4X3

static void emit_GLSL_M3X4(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_vec3(ctx, 0, src0, sizeof (src0));
    char row0[64]; make_GLSL_srcarg_string_vec3(ctx, 1, row0, sizeof (row0));
    char row1[64]; make_GLSL_srcarg_string_vec3(ctx, 2, row1, sizeof (row1));
    char row2[64]; make_GLSL_srcarg_string_vec3(ctx, 3, row2, sizeof (row2));
    char row3[64]; make_GLSL_srcarg_string_vec3(ctx, 4, row3, sizeof (row3));

    char code[256];
    make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                "vec4(dot(%s, %s), dot(%s, %s), "
                                     "dot(%s, %s), dot(%s, %s))",
                                src0, row0, src0, row1,
                                src0, row2, src0, row3);
    output_line(ctx, "%s", code);
} // emit_GLSL_M3X4

static void emit_GLSL_M3X3(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_vec3(ctx, 0, src0, sizeof (src0));
    char row0[64]; make_GLSL_srcarg_string_vec3(ctx, 1, row0, sizeof (row0));
    char row1[64]; make_GLSL_srcarg_string_vec3(ctx, 2, row1, sizeof (row1));
    char row2[64]; make_GLSL_srcarg_string_vec3(ctx, 3, row2, sizeof (row2));
    char code[256];
    make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                "vec3(dot(%s, %s), dot(%s, %s), dot(%s, %s))",
                                src0, row0, src0, row1, src0, row2);
    output_line(ctx, "%s", code);
} // emit_GLSL_M3X3

static void emit_GLSL_M3X2(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_vec3(ctx, 0, src0, sizeof (src0));
    char row0[64]; make_GLSL_srcarg_string_vec3(ctx, 1, row0, sizeof (row0));
    char row1[64]; make_GLSL_srcarg_string_vec3(ctx, 2, row1, sizeof (row1));

    char code[256];
    make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                "vec2(dot(%s, %s), dot(%s, %s))",
                                src0, row0, src0, row1);
    output_line(ctx, "%s", code);
} // emit_GLSL_M3X2

static void emit_GLSL_CALL(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    if (ctx->loops > 0)
        output_line(ctx, "%s(aL);", src0);
    else
        output_line(ctx, "%s();", src0);
} // emit_GLSL_CALL

static void emit_GLSL_CALLNZ(Context *ctx)
{
    // !!! FIXME: if src1 is a constbool that's true, we can remove the
    // !!! FIXME:  if. If it's false, we can make this a no-op.
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));

    if (ctx->loops > 0)
        output_line(ctx, "if (%s) { %s(aL); }", src1, src0);
    else
        output_line(ctx, "if (%s) { %s(); }", src1, src0);
} // emit_GLSL_CALLNZ

static void emit_GLSL_LOOP(Context *ctx)
{
    // !!! FIXME: swizzle?
    char var[64]; get_GLSL_srcarg_varname(ctx, 1, var, sizeof (var));
    assert(ctx->source_args[0].regnum == 0);  // in case they add aL1 someday.
    output_line(ctx, "{");
    ctx->indent++;
    output_line(ctx, "const int aLend = %s.x + %s.y;", var, var);
    output_line(ctx, "for (int aL = %s.y; aL < aLend; aL += %s.z) {", var, var);
    ctx->indent++;
} // emit_GLSL_LOOP

static void emit_GLSL_RET(Context *ctx)
{
    // thankfully, the MSDN specs say a RET _has_ to end a function...no
    //  early returns. So if you hit one, you know you can safely close
    //  a high-level function.
    ctx->indent--;
    output_line(ctx, "}");
    output_blank_line(ctx);
    set_output(ctx, &ctx->subroutines);
} // emit_GLSL_RET

static void emit_GLSL_ENDLOOP(Context *ctx)
{
    ctx->indent--;
    output_line(ctx, "}");
    ctx->indent--;
    output_line(ctx, "}");
} // emit_GLSL_ENDLOOP

static void emit_GLSL_LABEL(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    const int label = ctx->source_args[0].regnum;
    RegisterList *reg = reglist_find(&ctx->used_registers, REG_TYPE_LABEL, label);
    assert(ctx->output == ctx->subroutines);  // not mainline, etc.
    assert(ctx->indent == 0);  // we shouldn't be in the middle of a function.

    // MSDN specs say CALL* has to come before the LABEL, so we know if we
    //  can ditch the entire function here as unused.
    if (reg == NULL)
        set_output(ctx, &ctx->ignore);  // Func not used. Parse, but don't output.

    // !!! FIXME: it would be nice if we could determine if a function is
    // !!! FIXME:  only called once and, if so, forcibly inline it.

    const char *uses_loopreg = ((reg) && (reg->misc == 1)) ? "int aL" : "";
    output_line(ctx, "void %s(%s)", src0, uses_loopreg);
    output_line(ctx, "{");
    ctx->indent++;
} // emit_GLSL_LABEL

static void emit_GLSL_DCL(Context *ctx)
{
    // no-op. We do this in our emit_attribute() and emit_uniform().
    (void)ctx;
} // emit_GLSL_DCL

static void emit_GLSL_POW(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code),
                             "pow(abs(%s), %s)", src0, src1);
    output_line(ctx, "%s", code);
} // emit_GLSL_POW

static void emit_GLSL_CRS(Context *ctx)
{
    // !!! FIXME: needs to take ctx->dst_arg.writemask into account.
    char src0[64]; make_GLSL_srcarg_string_vec3(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_vec3(ctx, 1, src1, sizeof (src1));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code),
                             "cross(%s, %s)", src0, src1);
    output_line(ctx, "%s", code);
} // emit_GLSL_CRS

static void emit_GLSL_SGN(Context *ctx)
{
    // (we don't need the temporary registers specified for the D3D opcode.)
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "sign(%s)", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_SGN

static void emit_GLSL_ABS(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "abs(%s)", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_ABS

static void emit_GLSL_NRM(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "normalize(%s)", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_NRM

static void emit_GLSL_SINCOS(Context *ctx)
{
    // we don't care about the temp registers that <= sm2 demands; ignore them.
    //  sm2 also talks about what components are left untouched vs. undefined,
    //  but we just leave those all untouched with GLSL write masks (which
    //  would fulfill the "undefined" requirement, too).
    const int mask = ctx->dest_arg.writemask;
    char src0[64]; make_GLSL_srcarg_string_scalar(ctx, 0, src0, sizeof (src0));
    char code[128] = { '\0' };

    if (writemask_x(mask))
        make_GLSL_destarg_assign(ctx, code, sizeof (code), "cos(%s)", src0);
    else if (writemask_y(mask))
        make_GLSL_destarg_assign(ctx, code, sizeof (code), "sin(%s)", src0);
    else if (writemask_xy(mask))
    {
        make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                 "vec2(cos(%s), sin(%s))", src0, src0);
    } // else if

    output_line(ctx, "%s", code);
} // emit_GLSL_SINCOS

static void emit_GLSL_REP(Context *ctx)
{
    // !!! FIXME:
    // msdn docs say legal loop values are 0 to 255. We can check DEFI values
    //  at parse time, but if they are pulling a value from a uniform, do
    //  we clamp here?
    // !!! FIXME: swizzle is legal here, right?
    char src0[64]; make_GLSL_srcarg_string_x(ctx, 0, src0, sizeof(src0));
    const uint rep = (uint) ctx->reps;
    output_line(ctx, "for (int rep%u = 0; rep%u < %s; rep%u++) {",
                rep, rep, src0, rep);
    ctx->indent++;
} // emit_GLSL_REP

static void emit_GLSL_ENDREP(Context *ctx)
{
    ctx->indent--;
    output_line(ctx, "}");
} // emit_GLSL_ENDREP

static void emit_GLSL_IF(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_scalar(ctx, 0, src0, sizeof (src0));
    output_line(ctx, "if (%s) {", src0);
    ctx->indent++;
} // emit_GLSL_IF

static void emit_GLSL_IFC(Context *ctx)
{
    const char *comp = get_GLSL_comparison_string_scalar(ctx);
    char src0[64]; make_GLSL_srcarg_string_scalar(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_scalar(ctx, 1, src1, sizeof (src1));
    output_line(ctx, "if (%s %s %s) {", src0, comp, src1);
    ctx->indent++;
} // emit_GLSL_IFC

static void emit_GLSL_ELSE(Context *ctx)
{
    ctx->indent--;
    output_line(ctx, "} else {");
    ctx->indent++;
} // emit_GLSL_ELSE

static void emit_GLSL_ENDIF(Context *ctx)
{
    ctx->indent--;
    output_line(ctx, "}");
} // emit_GLSL_ENDIF

static void emit_GLSL_BREAK(Context *ctx)
{
    output_line(ctx, "break;");
} // emit_GLSL_BREAK

static void emit_GLSL_BREAKC(Context *ctx)
{
    const char *comp = get_GLSL_comparison_string_scalar(ctx);
    char src0[64]; make_GLSL_srcarg_string_scalar(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_scalar(ctx, 1, src1, sizeof (src1));
    output_line(ctx, "if (%s %s %s) { break; }", src0, comp, src1);
} // emit_GLSL_BREAKC

static void emit_GLSL_MOVA(Context *ctx)
{
    const int vecsize = vecsize_from_writemask(ctx->dest_arg.writemask);
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof(src0));
    char code[128];

    if(vecsize == 1)
    {
        make_GLSL_destarg_assign(ctx, code, sizeof(code),
                                 "int(round(%s))", src0);
    }
    else
    {
        make_GLSL_destarg_assign(ctx, code, sizeof(code),
                                 "ivec%d(round(%s))",
                                 vecsize, src0);
    }

    output_line(ctx, "%s", code);
} // emit_GLSL_MOVA

static void emit_GLSL_DEFB(Context *ctx)
{
    char varname[64]; get_GLSL_destarg_varname(ctx, varname, sizeof (varname));
    push_output(ctx, &ctx->globals);
    output_line(ctx, "const bool %s = %s;",
                varname, ctx->dwords[0] ? "true" : "false");
    pop_output(ctx);
} // emit_GLSL_DEFB

static void emit_GLSL_DEFI(Context *ctx)
{
    char varname[64]; get_GLSL_destarg_varname(ctx, varname, sizeof (varname));
    const int32 *x = (const int32 *) ctx->dwords;
    push_output(ctx, &ctx->globals);
    output_line(ctx, "const ivec4 %s = ivec4(%d, %d, %d, %d);",
                varname, (int) x[0], (int) x[1], (int) x[2], (int) x[3]);
    pop_output(ctx);
} // emit_GLSL_DEFI

EMIT_GLSL_OPCODE_UNIMPLEMENTED_FUNC(TEXCRD)

static void emit_GLSL_TEXKILL(Context *ctx)
{
    char dst[64]; get_GLSL_destarg_varname(ctx, dst, sizeof (dst));
    output_line(ctx, "if (any(lessThan(%s.xyz, vec3(0.0)))) discard;", dst);
} // emit_GLSL_TEXKILL

static void glsl_texld(Context *ctx, const int texldd, const int texldl)
{
    if(!shader_version_atleast(ctx, 1, 4))
    {
        DestArgInfo *info = &ctx->dest_arg;
        char dst[64];
        char sampler[64];
        char code[128] = {0};
        const char *coords = "";
        const char *cast_left = "";
        const char *cast_right = "";

        assert(!texldd && !texldl);

        RegisterList *sreg;
        sreg = reglist_find(&ctx->samplers, REG_TYPE_SAMPLER, info->regnum);
        const TextureType ttype = (TextureType) (sreg ? sreg->index : 0);

        // !!! FIXME: this code counts on the register not having swizzles, etc.
        get_GLSL_destarg_varname(ctx, dst, sizeof (dst));
        get_GLSL_varname_in_buf(ctx, REG_TYPE_SAMPLER, info->regnum,
                                sampler, sizeof (sampler));

        if(!(ctx->shadow_samplers&(1<<info->regnum)))
        {
            if(ttype == TEXTURE_TYPE_2D)
                coords = ".xy";
            else if(ttype == TEXTURE_TYPE_VOLUME || ttype == TEXTURE_TYPE_CUBE)
                coords = ".xyz";
            else
                fail(ctx, "unexpected texture type");
        }
        else
        {
            const int vecsize = vecsize_from_writemask(info->writemask);
            if(vecsize == 1)
                cast_left = "float(";
            else if(vecsize == 2)
                cast_left = "vec2(";
            else if(vecsize == 3)
                cast_left = "vec3(";
            else if(vecsize == 4)
                cast_left = "vec4(";
            else
            {
                fail(ctx, "Unexpected writemask size");
                cast_left = "(";
            }
            cast_right = ")";

            if(ttype == TEXTURE_TYPE_2D)
                coords = ".xyz";
            else if(ttype == TEXTURE_TYPE_VOLUME || ttype == TEXTURE_TYPE_CUBE)
                coords = ".xyzw";
            else
                fail(ctx, "unexpected texture type");
        }

        make_GLSL_destarg_assign(ctx, code, sizeof(code),
                                 "%stexture(%s, %s%s)%s",
                                 cast_left, sampler, dst, coords, cast_right);
        output_line(ctx, "%s", code);
    }
    else if(!shader_version_atleast(ctx, 2, 0))
    {
        // ps_1_4 is different, too!
        fail(ctx, "TEXLD == Shader Model 1.4 unimplemented.");  // !!! FIXME
        return;
    }
    else
    {
        const SourceArgInfo *samp_arg = &ctx->source_args[1];
        RegisterList *sreg = reglist_find(&ctx->samplers, REG_TYPE_SAMPLER, samp_arg->regnum);
        char funcname[32] = { '\0' };
        char src0[64] = { '\0' };
        char src1[64]; get_GLSL_srcarg_varname(ctx, 1, src1, sizeof(src1)); // !!! FIXME: SRC_MOD?
        char src2[64] = { '\0' };
        char src3[64] = { '\0' };

        if(sreg == NULL)
        {
            fail(ctx, "TEXLD using undeclared sampler");
            return;
        }
        assert(!isscalar(ctx, ctx->shader_type, samp_arg->regtype, samp_arg->regnum));

        if(texldd)
        {
            make_GLSL_srcarg_string_vec2(ctx, 2, src2, sizeof(src2));
            make_GLSL_srcarg_string_vec2(ctx, 3, src3, sizeof(src3));
        }

        // !!! FIXME: can TEXLDD or TEXLDL set instruction_controls?
        // !!! FIXME: does the d3d bias value map directly to GLSL?
        const char *biassep = "";
        char bias[64] = { '\0' };
        if(ctx->instruction_controls == CONTROL_TEXLDB || texldl)
        {
            biassep = ", ";
            make_GLSL_srcarg_string_w(ctx, 0, bias, sizeof(bias));
        }

        int mask = 0;
        if(ctx->instruction_controls == CONTROL_TEXLDP)
        {
            if(sreg->index == TEXTURE_TYPE_CUBE)
                fail(ctx, "TEXLDP on a cubemap");  // !!! FIXME: is this legal?
            if(!(ctx->shadow_samplers&(1<<samp_arg->regnum)))
                snprintf(funcname, sizeof(funcname), "textureProj");
            else
            {
                const int vecsize = vecsize_from_writemask(ctx->dest_arg.writemask);
                if(vecsize == 1)
                    snprintf(funcname, sizeof(funcname), "float(textureProj");
                else
                    snprintf(funcname, sizeof(funcname), "vec%d(textureProj", vecsize);
            }
            // PS2.0+ always uses the 4th component for projection
            mask = (1<<3);
        }
        else
        {
            // texld/texldb/texldl
            if(!(ctx->shadow_samplers&(1<<samp_arg->regnum)))
                snprintf(funcname, sizeof(funcname), "texture");
            else
            {
                const int vecsize = vecsize_from_writemask(ctx->dest_arg.writemask);
                if(vecsize == 1)
                    snprintf(funcname, sizeof(funcname), "float(texture");
                else
                    snprintf(funcname, sizeof(funcname), "vec%d(texture", vecsize);
            }
        }

        switch((const TextureType)sreg->index)
        {
        case TEXTURE_TYPE_2D:
            mask |= (1<<0) | (1<<1);
            if((ctx->shadow_samplers&(1<<samp_arg->regnum)))
                mask |= (1<<2);
            break;
        case TEXTURE_TYPE_VOLUME:
        case TEXTURE_TYPE_CUBE:
            mask |= (1<<0) | (1<<1) | (1<<2);
            if((ctx->shadow_samplers&(1<<samp_arg->regnum)))
                fail(ctx, "3D/Cube shadow samplers unsupported");
            break;
        default:
            fail(ctx, "unknown texture type");
            break;
        }
        make_GLSL_srcarg_string(ctx, 0, mask, src0, sizeof(src0));

        char swiz_str[7] = { '\0', '\0' };
        if(!(ctx->shadow_samplers&(1<<samp_arg->regnum)))
            make_GLSL_swizzle_string(swiz_str, samp_arg->swizzle, ctx->dest_arg.writemask);
        else
        {
            swiz_str[0] = ')';
            make_GLSL_swizzle_string(swiz_str+1, samp_arg->swizzle, ctx->dest_arg.writemask);
        }

        char code[128];
        if(texldd)
            make_GLSL_destarg_assign(ctx, code, sizeof(code),
                                     "%sGrad(%s, %s, %s, %s)%s", funcname,
                                     src1, src0, src2, src3, swiz_str);
        else if(texldl)
            make_GLSL_destarg_assign(ctx, code, sizeof(code),
                                     "%sLod(%s, %s%s%s)%s", funcname,
                                     src1, src0, biassep, bias, swiz_str);
        else
            make_GLSL_destarg_assign(ctx, code, sizeof(code),
                                     "%s(%s, %s%s%s)%s", funcname,
                                     src1, src0, biassep, bias, swiz_str);

        output_line(ctx, "%s", code);
    }
}

static void emit_GLSL_TEXLD(Context *ctx)
{
    glsl_texld(ctx, 0, 0);
}


static void emit_GLSL_TEXBEM(Context *ctx)
{
    DestArgInfo *info = &ctx->dest_arg;
    char dst[64]; get_GLSL_destarg_varname(ctx, dst, sizeof(dst));
    char src[64]; get_GLSL_srcarg_varname(ctx, 0, src, sizeof(src));
    char sampler[64];
    char code[512];

    // !!! FIXME: this code counts on the register not having swizzles, etc.
    get_GLSL_varname_in_buf(ctx, REG_TYPE_SAMPLER, info->regnum,
                            sampler, sizeof(sampler));

    make_GLSL_destarg_assign(ctx, code, sizeof(code),
        "texture(%s, vec2(%s.x + (%s_texbem.x * %s.x) + (%s_texbem.z * %s.y),"
        " %s.y + (%s_texbem.y * %s.x) + (%s_texbem.w * %s.y)))",
        sampler,
        dst, sampler, src, sampler, src,
        dst, sampler, src, sampler, src);

    output_line(ctx, "%s", code);
} // emit_GLSL_TEXBEM


static void emit_GLSL_TEXBEML(Context *ctx)
{
    // !!! FIXME: this code counts on the register not having swizzles, etc.
    DestArgInfo *info = &ctx->dest_arg;
    char dst[64]; get_GLSL_destarg_varname(ctx, dst, sizeof(dst));
    char src[64]; get_GLSL_srcarg_varname(ctx, 0, src, sizeof(src));
    char sampler[64];
    char code[512];

    get_GLSL_varname_in_buf(ctx, REG_TYPE_SAMPLER, info->regnum,
                            sampler, sizeof(sampler));

    make_GLSL_destarg_assign(ctx, code, sizeof (code),
        "(texture(%s, vec2(%s.x + (%s_texbem.x * %s.x) + (%s_texbem.z * %s.y),"
        " %s.y + (%s_texbem.y * %s.x) + (%s_texbem.w * %s.y)))) *"
        " ((%s.z * %s_texbeml.x) + %s_texbem.y)",
        sampler,
        dst, sampler, src, sampler, src,
        dst, sampler, src, sampler, src,
        src, sampler, sampler
    );

    output_line(ctx, "%s", code);
} // emit_GLSL_TEXBEML

EMIT_GLSL_OPCODE_UNIMPLEMENTED_FUNC(TEXREG2AR) // !!! FIXME
EMIT_GLSL_OPCODE_UNIMPLEMENTED_FUNC(TEXREG2GB) // !!! FIXME


static void emit_GLSL_TEXM3X2PAD(Context *ctx)
{
    // no-op ... work happens in emit_GLSL_TEXM3X2TEX().
    (void)ctx;
} // emit_GLSL_TEXM3X2PAD

static void emit_GLSL_TEXM3X2TEX(Context *ctx)
{
    if (ctx->texm3x2pad_src0 == -1)
        return;

    DestArgInfo *info = &ctx->dest_arg;
    char dst[64];
    char src0[64];
    char src1[64];
    char src2[64];
    char sampler[64];
    char code[512];

    // !!! FIXME: this code counts on the register not having swizzles, etc.
    get_GLSL_varname_in_buf(ctx, REG_TYPE_SAMPLER, info->regnum, sampler, sizeof(sampler));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x2pad_src0, src0, sizeof(src0));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x2pad_dst0, src1, sizeof(src1));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->source_args[0].regnum,  src2, sizeof(src2));
    get_GLSL_destarg_varname(ctx, dst, sizeof(dst));

    make_GLSL_destarg_assign(ctx, code, sizeof(code),
        "texture(%s, vec2(dot(%s.xyz, %s.xyz), dot(%s.xyz, %s.xyz)))",
        sampler, src0, src1, src2, dst
    );

    output_line(ctx, "%s", code);
} // emit_GLSL_TEXM3X2TEX

static void emit_GLSL_TEXM3X3PAD(Context *ctx)
{
    // no-op ... work happens in emit_GLSL_TEXM3X3*().
    (void)ctx;
} // emit_GLSL_TEXM3X3PAD

static void emit_GLSL_TEXM3X3TEX(Context *ctx)
{
    if (ctx->texm3x3pad_src1 == -1)
        return;

    DestArgInfo *info = &ctx->dest_arg;
    char dst[64];
    char src0[64];
    char src1[64];
    char src2[64];
    char src3[64];
    char src4[64];
    char sampler[64];
    char code[512];

    // !!! FIXME: this code counts on the register not having swizzles, etc.
    get_GLSL_varname_in_buf(ctx, REG_TYPE_SAMPLER, info->regnum, sampler, sizeof(sampler));

    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_dst0, src0, sizeof(src0));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_src0, src1, sizeof(src1));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_dst1, src2, sizeof(src2));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_src1, src3, sizeof(src3));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->source_args[0].regnum, src4, sizeof(src4));
    get_GLSL_destarg_varname(ctx, dst, sizeof(dst));

    make_GLSL_destarg_assign(ctx, code, sizeof(code),
        "texture(%s,"
            " vec3(dot(%s.xyz, %s.xyz),"
            " dot(%s.xyz, %s.xyz),"
            " dot(%s.xyz, %s.xyz)))",
        sampler, src0, src1, src2, src3, dst, src4);

    output_line(ctx, "%s", code);
} // emit_GLSL_TEXM3X3TEX

static void emit_GLSL_TEXM3X3SPEC_helper(Context *ctx)
{
    if (ctx->glsl_generated_texm3x3spec_helper)
        return;
    ctx->glsl_generated_texm3x3spec_helper = 1;

    push_output(ctx, &ctx->helpers);
    output_line(ctx, "vec3 TEXM3X3SPEC_reflection(const vec3 normal, const vec3 eyeray)"
                     "{"
                     "\treturn (2.0 * ((normal * eyeray) / (normal * normal)) * normal) - eyeray;"
                     "}");
    output_blank_line(ctx);
    pop_output(ctx);
} // emit_GLSL_TEXM3X3SPEC_helper

static void emit_GLSL_TEXM3X3SPEC(Context *ctx)
{
    if (ctx->texm3x3pad_src1 == -1)
        return;

    DestArgInfo *info = &ctx->dest_arg;
    char dst[64];
    char src0[64];
    char src1[64];
    char src2[64];
    char src3[64];
    char src4[64];
    char src5[64];
    char sampler[64];
    char code[512];

    emit_GLSL_TEXM3X3SPEC_helper(ctx);

    // !!! FIXME: this code counts on the register not having swizzles, etc.
    get_GLSL_varname_in_buf(ctx, REG_TYPE_SAMPLER, info->regnum, sampler, sizeof(sampler));

    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_dst0, src0, sizeof(src0));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_src0, src1, sizeof(src1));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_dst1, src2, sizeof(src2));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_src1, src3, sizeof(src3));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->source_args[0].regnum, src4, sizeof(src4));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->source_args[1].regnum, src5, sizeof(src5));
    get_GLSL_destarg_varname(ctx, dst, sizeof(dst));

    make_GLSL_destarg_assign(ctx, code, sizeof (code),
        "texture(%s, "
            "TEXM3X3SPEC_reflection("
                "vec3("
                    "dot(%s.xyz, %s.xyz), "
                    "dot(%s.xyz, %s.xyz), "
                    "dot(%s.xyz, %s.xyz)"
                "),"
                "%s.xyz,"
            ")"
        ")",
        sampler, src0, src1, src2, src3, dst, src4, src5);

    output_line(ctx, "%s", code);
} // emit_GLSL_TEXM3X3SPEC

static void emit_GLSL_TEXM3X3VSPEC(Context *ctx)
{
    if (ctx->texm3x3pad_src1 == -1)
        return;

    DestArgInfo *info = &ctx->dest_arg;
    char dst[64];
    char src0[64];
    char src1[64];
    char src2[64];
    char src3[64];
    char src4[64];
    char sampler[64];
    char code[512];

    emit_GLSL_TEXM3X3SPEC_helper(ctx);

    // !!! FIXME: this code counts on the register not having swizzles, etc.
    get_GLSL_varname_in_buf(ctx, REG_TYPE_SAMPLER, info->regnum, sampler, sizeof(sampler));

    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_dst0, src0, sizeof(src0));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_src0, src1, sizeof(src1));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_dst1, src2, sizeof(src2));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_src1, src3, sizeof(src3));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->source_args[0].regnum, src4, sizeof(src4));
    get_GLSL_destarg_varname(ctx, dst, sizeof(dst));

    make_GLSL_destarg_assign(ctx, code, sizeof(code),
        "texture(%s, "
            "TEXM3X3SPEC_reflection("
                "vec3("
                    "dot(%s.xyz, %s.xyz), "
                    "dot(%s.xyz, %s.xyz), "
                    "dot(%s.xyz, %s.xyz)"
                "), "
                "vec3(%s.w, %s.w, %s.w)"
            ")"
        ")",
        sampler, src0, src1, src2, src3, dst, src4, src0, src2, dst);

    output_line(ctx, "%s", code);
} // emit_GLSL_TEXM3X3VSPEC

static void emit_GLSL_EXPP(Context *ctx)
{
    // !!! FIXME: msdn's asm docs don't list this opcode, I'll have to check the driver documentation.
    emit_GLSL_EXP(ctx);  // I guess this is just partial precision EXP?
} // emit_GLSL_EXPP

static void emit_GLSL_LOGP(Context *ctx)
{
    // LOGP is just low-precision LOG, but we'll take the higher precision.
    emit_GLSL_LOG(ctx);
} // emit_GLSL_LOGP

// common code between CMP and CND.
static void emit_GLSL_comparison_operations(Context *ctx, const char *cmp)
{
    int i, j;
    DestArgInfo *dst = &ctx->dest_arg;
    const SourceArgInfo *srcarg0 = &ctx->source_args[0];
    const int origmask = dst->writemask;
    int used_swiz[4] = { 0, 0, 0, 0 };
    const int writemask[4] = { dst->writemask0, dst->writemask1,
                               dst->writemask2, dst->writemask3 };
    const int src0swiz[4] = { srcarg0->swizzle_x, srcarg0->swizzle_y,
                              srcarg0->swizzle_z, srcarg0->swizzle_w };

    for (i = 0; i < 4; i++)
    {
        int mask = (1 << i);

        if(!writemask[i]) continue;
        if(used_swiz[i]) continue;

        // This is a swizzle we haven't checked yet.
        used_swiz[i] = 1;

        // see if there are any other elements swizzled to match (.yyyy)
        for(j = i + 1; j < 4; j++)
        {
            if (!writemask[j]) continue;
            if (src0swiz[i] != src0swiz[j]) continue;
            mask |= (1 << j);
            used_swiz[j] = 1;
        }

        // okay, (mask) should be the writemask of swizzles we like.

        //return make_GLSL_srcarg_string(ctx, idx, (1 << 0));

        char src0[64];
        char src1[64];
        char src2[64];
        make_GLSL_srcarg_string(ctx, 0, (1 << i), src0, sizeof (src0));
        make_GLSL_srcarg_string(ctx, 1, mask, src1, sizeof (src1));
        make_GLSL_srcarg_string(ctx, 2, mask, src2, sizeof (src2));

        set_dstarg_writemask(dst, mask);

        char code[128];
        make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                 "((%s %s) ? %s : %s)",
                                 src0, cmp, src1, src2);
        output_line(ctx, "%s", code);
    }

    set_dstarg_writemask(dst, origmask);
} // emit_GLSL_comparison_operations

static void emit_GLSL_CND(Context *ctx)
{
    emit_GLSL_comparison_operations(ctx, "> 0.5");
} // emit_GLSL_CND

static void emit_GLSL_DEF(Context *ctx)
{
    const float *val = (const float*)ctx->dwords; // !!! FIXME: could be int?
    char varname[64]; get_GLSL_destarg_varname(ctx, varname, sizeof(varname));
    char val0[32]; floatstr(val0, sizeof(val0), val[0]);
    char val1[32]; floatstr(val1, sizeof(val1), val[1]);
    char val2[32]; floatstr(val2, sizeof(val2), val[2]);
    char val3[32]; floatstr(val3, sizeof(val3), val[3]);

    push_output(ctx, &ctx->globals);
    output_line(ctx, "const vec4 %s = vec4(%s, %s, %s, %s);",
                varname, val0, val1, val2, val3);
    pop_output(ctx);
} // emit_GLSL_DEF

EMIT_GLSL_OPCODE_UNIMPLEMENTED_FUNC(TEXREG2RGB) // !!! FIXME
EMIT_GLSL_OPCODE_UNIMPLEMENTED_FUNC(TEXDP3TEX) // !!! FIXME
EMIT_GLSL_OPCODE_UNIMPLEMENTED_FUNC(TEXM3X2DEPTH) // !!! FIXME
EMIT_GLSL_OPCODE_UNIMPLEMENTED_FUNC(TEXDP3) // !!! FIXME

static void emit_GLSL_TEXM3X3(Context *ctx)
{
    if (ctx->texm3x3pad_src1 == -1)
        return;

    char dst[64];
    char src0[64];
    char src1[64];
    char src2[64];
    char src3[64];
    char src4[64];
    char code[512];

    // !!! FIXME: this code counts on the register not having swizzles, etc.
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_dst0, src0, sizeof(src0));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_src0, src1, sizeof(src1));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_dst1, src2, sizeof(src2));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->texm3x3pad_src1, src3, sizeof(src3));
    get_GLSL_varname_in_buf(ctx, REG_TYPE_TEXTURE, ctx->source_args[0].regnum, src4, sizeof(src4));
    get_GLSL_destarg_varname(ctx, dst, sizeof (dst));

    make_GLSL_destarg_assign(ctx, code, sizeof (code),
        "vec4(dot(%s.xyz, %s.xyz), dot(%s.xyz, %s.xyz), dot(%s.xyz, %s.xyz), 1.0)",
        src0, src1, src2, src3, dst, src4);

    output_line(ctx, "%s", code);
} // emit_GLSL_TEXM3X3

EMIT_GLSL_OPCODE_UNIMPLEMENTED_FUNC(TEXDEPTH) // !!! FIXME

static void emit_GLSL_CMP(Context *ctx)
{
    emit_GLSL_comparison_operations(ctx, ">= 0.0");
} // emit_GLSL_CMP

EMIT_GLSL_OPCODE_UNIMPLEMENTED_FUNC(BEM) // !!! FIXME

static void emit_GLSL_DP2ADD(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_vec2(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_vec2(ctx, 1, src1, sizeof (src1));
    char src2[64]; make_GLSL_srcarg_string_scalar(ctx, 2, src2, sizeof (src2));
    char extra[64]; snprintf(extra, sizeof (extra), " + %s", src2);
    emit_GLSL_dotprod(ctx, src0, src1, extra);
} // emit_GLSL_DP2ADD

static void emit_GLSL_DSX(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "dFdx(%s)", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_DSX

static void emit_GLSL_DSY(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char code[128];
    make_GLSL_destarg_assign(ctx, code, sizeof (code), "dFdy(%s)", src0);
    output_line(ctx, "%s", code);
} // emit_GLSL_DSY

static void emit_GLSL_TEXLDD(Context *ctx)
{
    glsl_texld(ctx, 1, 0);
} // emit_GLSL_TEXLDD

static void emit_GLSL_SETP(Context *ctx)
{
    const int vecsize = vecsize_from_writemask(ctx->dest_arg.writemask);
    char src0[64]; make_GLSL_srcarg_string_masked(ctx, 0, src0, sizeof (src0));
    char src1[64]; make_GLSL_srcarg_string_masked(ctx, 1, src1, sizeof (src1));
    char code[128];

    // destination is always predicate register (which is type bvec4).
    if(vecsize == 1)
    {
        const char *comp = get_GLSL_comparison_string_scalar(ctx);
        make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                 "(%s %s %s)", src0, comp, src1);
    }
    else
    {
        const char *comp = get_GLSL_comparison_string_vector(ctx);
        make_GLSL_destarg_assign(ctx, code, sizeof (code),
                                 "%s(%s, %s)", comp, src0, src1);
    }

    output_line(ctx, "%s", code);
} // emit_GLSL_SETP

static void emit_GLSL_TEXLDL(Context *ctx)
{
    glsl_texld(ctx, 0, 1);
} // emit_GLSL_TEXLDL

static void emit_GLSL_BREAKP(Context *ctx)
{
    char src0[64]; make_GLSL_srcarg_string_scalar(ctx, 0, src0, sizeof (src0));
    output_line(ctx, "if (%s) { break; }", src0);
} // emit_GLSL_BREAKP

static void emit_GLSL_RESERVED(Context *ctx)
{
    // do nothing; fails in the state machine.
    (void)ctx;
} // emit_GLSL_RESERVED

#endif  // SUPPORT_PROFILE_GLSL


#if !AT_LEAST_ONE_PROFILE
#error No profiles are supported. Fix your build.
#endif

#define DEFINE_PROFILE(prof) { \
    MOJOSHADER_PROFILE_##prof, \
    emit_##prof##_start, \
    emit_##prof##_end, \
    emit_##prof##_phase, \
    emit_##prof##_global, \
    emit_##prof##_uniform, \
    emit_##prof##_sampler, \
    emit_##prof##_attribute, \
    emit_##prof##_finalize, \
    get_##prof##_varname, \
},

static const Profile profiles[] =
{
    DEFINE_PROFILE(GLSL)
};

#undef DEFINE_PROFILE

// This is for profiles that extend other profiles...
static const struct { const char *from; const char *to; } profileMap[] =
{
    { MOJOSHADER_PROFILE_GLSL330, MOJOSHADER_PROFILE_GLSL },
    { MOJOSHADER_PROFILE_GLSL120, MOJOSHADER_PROFILE_GLSL },
};


// The PROFILE_EMITTER_* items MUST be in the same order as profiles[]!
#define PROFILE_EMITTERS(op) { \
     PROFILE_EMITTER_GLSL(op) \
}

static int parse_destination_token(Context *ctx, DestArgInfo *info)
{
    // !!! FIXME: recheck against the spec for ranges (like RASTOUT values, etc).
    if (ctx->tokencount == 0)
    {
        fail(ctx, "Out of tokens in destination parameter");
        return 0;
    } // if

    const uint32 token = *(ctx->tokens);
    const int reserved1 = (int)((token>>14) & 0x3); // bits 14 through 15
    const int reserved2 = (int)((token>>31) & 0x1); // bit 31

    info->token = ctx->tokens;
    info->regnum = (int)(token & 0x7ff);  // bits 0 through 10
    info->relative = (int)((token>>13) & 0x1); // bit 13
    info->orig_writemask = (int)((token>>16) & 0xF); // bits 16 through 19
    info->result_mod = (int)((token>>20) & 0xF); // bits 20 through 23
    info->result_shift = (int)((token>>24) & 0xF); // bits 24 through 27      abc
    info->regtype = (RegisterType)(((token>>28) & 0x7) | ((token>>8) & 0x18));  // bits 28-30, 11-12

    int writemask;
    if (isscalar(ctx, ctx->shader_type, info->regtype, info->regnum))
        writemask = 0x1;  // just x.
    else
        writemask = info->orig_writemask;

    set_dstarg_writemask(info, writemask);  // bits 16 through 19.

    // all the REG_TYPE_CONSTx types are the same register type, it's just
    //  split up so its regnum can be > 2047 in the bytecode. Clean it up.
    if (info->regtype == REG_TYPE_CONST2)
    {
        info->regtype = REG_TYPE_CONST;
        info->regnum += 2048;
    }
    else if (info->regtype == REG_TYPE_CONST3)
    {
        info->regtype = REG_TYPE_CONST;
        info->regnum += 4096;
    }
    else if (info->regtype == REG_TYPE_CONST4)
    {
        info->regtype = REG_TYPE_CONST;
        info->regnum += 6144;
    }

    // swallow token for now, for multiple calls in a row.
    adjust_token_position(ctx, 1);

    if (reserved1 != 0x0)
        fail(ctx, "Reserved bit #1 in destination token must be zero");

    if (reserved2 != 0x1)
        fail(ctx, "Reserved bit #2 in destination token must be one");

    if (info->relative)
    {
        if (!shader_is_vertex(ctx))
            fail(ctx, "Relative addressing in non-vertex shader");
        if (!shader_version_atleast(ctx, 3, 0))
            fail(ctx, "Relative addressing in vertex shader version < 3.0");

        // !!! FIXME: I don't have a shader that has a relative dest currently.
        fail(ctx, "Relative addressing of dest tokens is unsupported");
        return 2;
    }

    const int s = info->result_shift;
    if (s != 0)
    {
        if (!shader_is_pixel(ctx))
            fail(ctx, "Result shift scale in non-pixel shader");
        if (shader_version_atleast(ctx, 2, 0))
            fail(ctx, "Result shift scale in pixel shader version >= 2.0");
        if ( ! (((s >= 1) && (s <= 3)) || ((s >= 0xD) && (s <= 0xF))) )
            fail(ctx, "Result shift scale isn't 1 to 3, or 13 to 15.");
    }

    if (info->result_mod & MOD_PP)  // Partial precision (pixel shaders only)
    {
        if (!shader_is_pixel(ctx))
            fail(ctx, "Partial precision result mod in non-pixel shader");
    }

    if (info->result_mod & MOD_CENTROID)  // Centroid (pixel shaders only)
    {
        if (!shader_is_pixel(ctx))
            fail(ctx, "Centroid result mod in non-pixel shader");
        else if (!ctx->centroid_allowed)  // only on DCL opcodes!
            fail(ctx, "Centroid modifier not allowed here");
    }

    if ((info->regtype < 0) || (info->regtype > REG_TYPE_MAX))
        fail(ctx, "Register type is out of range");

    if (!isfail(ctx))
        set_used_register(ctx, info->regtype, info->regnum, 1);

    return 1;
} // parse_destination_token


static void determine_constants_arrays(Context *ctx)
{
    // Only process this stuff once. This is called after all DEF* opcodes
    //  could have been parsed.
    if(ctx->determined_constants_arrays)
        return;
    ctx->determined_constants_arrays = 1;

    if (ctx->constant_count <= 1)
        return;  // nothing to sort or group.

    // Sort the linked list into an array for easier tapdancing...
    ConstantsList **array = alloca(sizeof(ConstantsList*) * (ctx->constant_count+1));
    ConstantsList *item = ctx->constants;
    int i;

    for(i = 0;i < ctx->constant_count;i++)
    {
        if(item == NULL)
        {
            fail(ctx, "BUG: mismatched constant list and count");
            return;
        }

        array[i] = item;
        item = item->next;
    }

    array[ctx->constant_count] = NULL;

    // bubble sort ftw.
    int sorted;
    do {
        sorted = 1;
        for(i = 0; i < ctx->constant_count-1; i++)
        {
            if(array[i]->constant.index > array[i+1]->constant.index)
            {
                ConstantsList *tmp = array[i];
                array[i] = array[i+1];
                array[i+1] = tmp;
                sorted = 0;
            }
        }
    } while(!sorted);

    // okay, sorted. While we're here, let's redo the linked list in order...
    for(i = 0; i < ctx->constant_count; i++)
        array[i]->next = array[i+1];
    ctx->constants = array[0];
}


static int parse_source_token(Context *ctx, SourceArgInfo *info)
{
    int retval = 1;

    if (ctx->tokencount == 0)
    {
        fail(ctx, "Out of tokens in source parameter");
        return 0;
    } // if

    const uint32 token = *(ctx->tokens);
    const int reserved1 = (int)((token>>14) & 0x3); // bits 14 through 15
    const int reserved2 = (int)((token>>31) & 0x1); // bit 31

    info->token = ctx->tokens;
    info->regnum = (int)(token & 0x7ff);  // bits 0 through 10
    info->relative = (int)((token>>13) & 0x1); // bit 13
    const int swizzle = (int)((token>>16) & 0xFF); // bits 16 through 23
    info->src_mod = (SourceMod)((token>>24) & 0xF); // bits 24 through 27
    info->regtype = (RegisterType)(((token>>28) & 0x7) | ((token>>8) & 0x18));  // bits 28-30, 11-12

    // all the REG_TYPE_CONSTx types are the same register type, it's just
    //  split up so its regnum can be > 2047 in the bytecode. Clean it up.
    if(info->regtype == REG_TYPE_CONST2)
    {
        info->regtype = REG_TYPE_CONST;
        info->regnum += 2048;
    }
    else if(info->regtype == REG_TYPE_CONST3)
    {
        info->regtype = REG_TYPE_CONST;
        info->regnum += 4096;
    }
    else if(info->regtype == REG_TYPE_CONST4)
    {
        info->regtype = REG_TYPE_CONST;
        info->regnum += 6144;
    }

    info->swizzle = swizzle;
    info->swizzle_x = ((info->swizzle >> 0) & 0x3);
    info->swizzle_y = ((info->swizzle >> 2) & 0x3);
    info->swizzle_z = ((info->swizzle >> 4) & 0x3);
    info->swizzle_w = ((info->swizzle >> 6) & 0x3);

    // swallow token for now, for multiple calls in a row.
    adjust_token_position(ctx, 1);

    if (reserved1 != 0x0)
        fail(ctx, "Reserved bits #1 in source token must be zero");

    if (reserved2 != 0x1)
        fail(ctx, "Reserved bit #2 in source token must be one");

    if ((info->relative) && (ctx->tokencount == 0))
    {
        fail(ctx, "Out of tokens in relative source parameter");
        info->relative = 0;  // don't try to process it.
    } // if

    if (info->relative)
    {
        if(shader_is_pixel(ctx) && !shader_version_atleast(ctx, 3, 0))
            fail(ctx, "Relative addressing in pixel shader version < 3.0");

        // Shader Model 1 doesn't have an extra token to specify the
        //  relative register: it's always a0.x.
        if (!shader_version_atleast(ctx, 2, 0))
        {
            info->relative_regnum = 0;
            info->relative_regtype = REG_TYPE_ADDRESS;
            info->relative_component = 0;
        }
        else  // Shader Model 2 and later...
        {
            const uint32 reltoken = *(ctx->tokens);
            // swallow token for now, for multiple calls in a row.
            adjust_token_position(ctx, 1);

            const int relswiz = (int)((reltoken>>16) & 0xFF);
            info->relative_regnum = (int) (reltoken & 0x7ff);
            info->relative_regtype = (RegisterType)
                                        (((reltoken>>28) & 0x7) |
                                        ((reltoken>>8) & 0x18));

            if (((reltoken>>31) & 0x1) == 0)
                fail(ctx, "bit #31 in relative address must be set");

            if ((reltoken & 0xF00E000) != 0)  // usused bits.
                fail(ctx, "relative address reserved bit must be zero");

            switch (info->relative_regtype)
            {
                case REG_TYPE_LOOP:
                case REG_TYPE_ADDRESS:
                    break;
                default:
                    fail(ctx, "invalid register for relative address");
                    break;
            }

            if (info->relative_regnum != 0)  // true for now.
                fail(ctx, "invalid register for relative address");

            if (!replicate_swizzle(relswiz))
                fail(ctx, "relative address needs replicate swizzle");

            info->relative_component = (relswiz & 0x3);

            retval++;
        }

        if (info->regtype == REG_TYPE_INPUT)
        {
            if(shader_is_pixel(ctx) || !shader_version_atleast(ctx, 3, 0))
                fail(ctx, "relative addressing of input registers not supported in this shader model");
            ctx->have_relative_input_registers = 1;
        } // if
        else if (info->regtype == REG_TYPE_CONST)
        {
            // figure out what array we're in...
            determine_constants_arrays(ctx);

            set_used_register(ctx, info->relative_regtype, info->relative_regnum, 0);
            ctx->have_relative_const_registers = 1;
        }
        else
        {
            fail(ctx, "relative addressing of invalid register");
        }
    }

    switch (info->src_mod)
    {
        case SRCMOD_NONE:
        case SRCMOD_ABSNEGATE:
        case SRCMOD_ABS:
        case SRCMOD_NEGATE:
            break; // okay in any shader model.

        // apparently these are only legal in Shader Model 1.x ...
        case SRCMOD_BIASNEGATE:
        case SRCMOD_BIAS:
        case SRCMOD_SIGNNEGATE:
        case SRCMOD_SIGN:
        case SRCMOD_COMPLEMENT:
        case SRCMOD_X2NEGATE:
        case SRCMOD_X2:
        case SRCMOD_DZ:
        case SRCMOD_DW:
            if (shader_version_atleast(ctx, 2, 0))
                fail(ctx, "illegal source mod for this Shader Model.");
            break;

        case SRCMOD_NOT:  // !!! FIXME: I _think_ this is right...
            if (shader_version_atleast(ctx, 2, 0))
            {
                if (info->regtype != REG_TYPE_PREDICATE)
                    fail(ctx, "NOT only allowed on predicate register.");
            } // if
            break;

        default:
            fail(ctx, "Unknown source modifier");
    } // switch

    // !!! FIXME: docs say this for sm3 ... check these!
    //  "The negate modifier cannot be used on second source register of these
    //   instructions: m3x2 - ps, m3x3 - ps, m3x4 - ps, m4x3 - ps, and
    //   m4x4 - ps."
    //  "If any version 3 shader reads from one or more constant float
    //   registers (c#), one of the following must be true.
    //    All of the constant floating-point registers must use the abs modifier.
    //    None of the constant floating-point registers can use the abs modifier.

    if (!isfail(ctx))
    {
        RegisterList *reg;
        reg = set_used_register(ctx, info->regtype, info->regnum, 0);
        // !!! FIXME: this test passes if you write to the register
        // !!! FIXME:  in this same instruction, because we parse the
        // !!! FIXME:  destination token first.
        // !!! FIXME: Microsoft's shader validation explicitly checks temp
        // !!! FIXME:  registers for this...do they check other writable ones?
        if ((info->regtype == REG_TYPE_TEMP) && (reg) && (!reg->written))
            failf(ctx, "Temp register r%d used uninitialized", info->regnum);
    } // if

    return retval;
} // parse_source_token


static int parse_predicated_token(Context *ctx)
{
    SourceArgInfo *arg = &ctx->predicate_arg;
    parse_source_token(ctx, arg);
    if (arg->regtype != REG_TYPE_PREDICATE)
        fail(ctx, "Predicated instruction but not predicate register!");
    if ((arg->src_mod != SRCMOD_NONE) && (arg->src_mod != SRCMOD_NOT))
        fail(ctx, "Predicated instruction register is not NONE or NOT");
    if ( !no_swizzle(arg->swizzle) && !replicate_swizzle(arg->swizzle) )
        fail(ctx, "Predicated instruction register has wrong swizzle");
    if (arg->relative)  // I'm pretty sure this is illegal...?
        fail(ctx, "relative addressing in predicated token");

    return 1;
} // parse_predicated_token


static int parse_args_NULL(Context *ctx)
{
    (void)ctx;
    return 1;
}


static int parse_args_DEF(Context *ctx)
{
    parse_destination_token(ctx, &ctx->dest_arg);
    if (ctx->dest_arg.regtype != REG_TYPE_CONST)
        fail(ctx, "DEF using non-CONST register");
    if (ctx->dest_arg.relative)  // I'm pretty sure this is illegal...?
        fail(ctx, "relative addressing in DEF");

    ctx->dwords[0] = ctx->tokens[0];
    ctx->dwords[1] = ctx->tokens[1];
    ctx->dwords[2] = ctx->tokens[2];
    ctx->dwords[3] = ctx->tokens[3];

    return 6;
} // parse_args_DEF


static int parse_args_DEFI(Context *ctx)
{
    parse_destination_token(ctx, &ctx->dest_arg);
    if (ctx->dest_arg.regtype != REG_TYPE_CONSTINT)
        fail(ctx, "DEFI using non-CONSTING register");
    if (ctx->dest_arg.relative)  // I'm pretty sure this is illegal...?
        fail(ctx, "relative addressing in DEFI");

    ctx->dwords[0] = ctx->tokens[0];
    ctx->dwords[1] = ctx->tokens[1];
    ctx->dwords[2] = ctx->tokens[2];
    ctx->dwords[3] = ctx->tokens[3];

    return 6;
} // parse_args_DEFI


static int parse_args_DEFB(Context *ctx)
{
    parse_destination_token(ctx, &ctx->dest_arg);
    if (ctx->dest_arg.regtype != REG_TYPE_CONSTBOOL)
        fail(ctx, "DEFB using non-CONSTBOOL register");
    if (ctx->dest_arg.relative)  // I'm pretty sure this is illegal...?
        fail(ctx, "relative addressing in DEFB");

    ctx->dwords[0] = *(ctx->tokens) ? 1 : 0;

    return 3;
} // parse_args_DEFB


static int valid_texture_type(const uint32 ttype)
{
    switch ((const TextureType) ttype)
    {
        case TEXTURE_TYPE_2D:
        case TEXTURE_TYPE_CUBE:
        case TEXTURE_TYPE_VOLUME:
            return 1;  // it's okay.
    } // switch

    return 0;
} // valid_texture_type


// !!! FIXME: this function is kind of a mess.
static int parse_args_DCL(Context *ctx)
{
    const uint32 token = *(ctx->tokens);
    const int reserved1 = (int) ((token>>31) & 0x1); // bit 31
    uint32 reserved_mask = 0x00000000;

    if (reserved1 != 0x1)
        fail(ctx, "Bit #31 in DCL token must be one");

    ctx->centroid_allowed = 1;
    adjust_token_position(ctx, 1);
    parse_destination_token(ctx, &ctx->dest_arg);
    ctx->centroid_allowed = 0;

    if (ctx->dest_arg.result_shift != 0)  // I'm pretty sure this is illegal...?
        fail(ctx, "shift scale in DCL");
    if (ctx->dest_arg.relative)  // I'm pretty sure this is illegal...?
        fail(ctx, "relative addressing in DCL");

    const RegisterType regtype = ctx->dest_arg.regtype;
    const int regnum = ctx->dest_arg.regnum;
    if(shader_is_pixel(ctx) && shader_version_atleast(ctx, 3, 0))
    {
        if (regtype == REG_TYPE_INPUT)
        {
            const uint32 usage = (token & 0xF);
            const uint32 index = ((token >> 16) & 0xF);
            reserved_mask = 0x7FF0FFE0;
            ctx->dwords[0] = usage;
            ctx->dwords[1] = index;
        }
        else if(regtype == REG_TYPE_MISCTYPE)
        {
            const MiscTypeType mt = (MiscTypeType) regnum;
            if (mt == MISCTYPE_TYPE_POSITION)
                reserved_mask = 0x7FFFFFFF;
            else if (mt == MISCTYPE_TYPE_FACE)
            {
                reserved_mask = 0x7FFFFFFF;
                if (!writemask_xyzw(ctx->dest_arg.orig_writemask))
                    fail(ctx, "DCL face writemask must be full");
                if (ctx->dest_arg.result_mod != 0)
                    fail(ctx, "DCL face result modifier must be zero");
                if (ctx->dest_arg.result_shift != 0)
                    fail(ctx, "DCL face shift scale must be zero");
            }
            else
            {
                failf(ctx, "ps3.x DCL unexpected misc register type: 0x%x", mt);
            }

            ctx->dwords[0] = (uint32) MOJOSHADER_USAGE_UNKNOWN;
            ctx->dwords[1] = 0;
        }
        else if (regtype == REG_TYPE_TEXTURE)
        {
            const uint32 usage = (token & 0xF);
            const uint32 index = ((token >> 16) & 0xF);
            if (usage == MOJOSHADER_USAGE_TEXCOORD)
            {
                if (index > 7)
                    fail(ctx, "DCL texcoord usage must have 0-7 index");
            }
            else if (usage == MOJOSHADER_USAGE_COLOR)
            {
                if (index != 0)
                    fail(ctx, "DCL color usage must have 0 index");
            }
            else
            {
                fail(ctx, "Invalid DCL texture usage");
            }

            reserved_mask = 0x7FF0FFE0;
            ctx->dwords[0] = usage;
            ctx->dwords[1] = index;
        }
        else if (regtype == REG_TYPE_SAMPLER)
        {
            const uint32 ttype = ((token >> 27) & 0xF);
            if (!valid_texture_type(ttype))
                fail(ctx, "unknown sampler texture type");
            reserved_mask = 0x7FFFFFF;
            ctx->dwords[0] = ttype;
        }
        else
        {
            failf(ctx, "ps3.x DCL unexpected register type: 0x%x", regtype);
        }
    }
    else if(shader_is_pixel(ctx) && shader_version_atleast(ctx, 2, 0))
    {
        if (regtype == REG_TYPE_INPUT)
        {
            ctx->dwords[0] = (uint32) MOJOSHADER_USAGE_COLOR;
            ctx->dwords[1] = regnum;
            reserved_mask = 0x7FFFFFFF;
        }
        else if (regtype == REG_TYPE_TEXTURE)
        {
            ctx->dwords[0] = (uint32) MOJOSHADER_USAGE_TEXCOORD;
            ctx->dwords[1] = regnum;
            reserved_mask = 0x7FFFFFFF;
        }
        else if (regtype == REG_TYPE_SAMPLER)
        {
            const uint32 ttype = ((token >> 27) & 0xF);
            if (!valid_texture_type(ttype))
                fail(ctx, "unknown sampler texture type");
            reserved_mask = 0x7FFFFFF;
            ctx->dwords[0] = ttype;
        }
        else
        {
            failf(ctx, "ps2.x DCL unexpected register type: 0x%x", regtype);
        }
    }
    else if(shader_is_vertex(ctx) && shader_version_atleast(ctx, 3, 0))
    {
        if(regtype == REG_TYPE_INPUT || regtype == REG_TYPE_OUTPUT)
        {
            const uint32 usage = (token & 0xF);
            const uint32 index = ((token >> 16) & 0xF);
            reserved_mask = 0x7FF0FFE0;
            ctx->dwords[0] = usage;
            ctx->dwords[1] = index;
        }
        else if (regtype == REG_TYPE_SAMPLER)
        {
            const uint32 ttype = ((token >> 27) & 0xF);
            if (!valid_texture_type(ttype))
                fail(ctx, "unknown sampler texture type");
            reserved_mask = 0x7FFFFFF;
            ctx->dwords[0] = ttype;
        }
        else
        {
            failf(ctx, "vs3.x DCL unexpected register type: 0x%x", regtype);
        }
    }
    else if(shader_is_vertex(ctx) && shader_version_atleast(ctx, 1, 1))
    {
        if (regtype == REG_TYPE_INPUT)
        {
            const uint32 usage = (token & 0xF);
            const uint32 index = ((token >> 16) & 0xF);
            reserved_mask = 0x7FF0FFE0;
            ctx->dwords[0] = usage;
            ctx->dwords[1] = index;
        }
        else
        {
            failf(ctx, "vs1.1 DCL unexpected register type: 0x%x", regtype);
        }
    }
    else
    {
        failf(ctx, "Unknown shader type in DCL");
    }

    if ((token & reserved_mask) != 0)
        fail(ctx, "reserved bits in DCL dword aren't zero");

    return 3;
} // parse_args_DCL


static int parse_args_D(Context *ctx)
{
    int retval = 1;
    retval += parse_destination_token(ctx, &ctx->dest_arg);
    return retval;
} // parse_args_D


static int parse_args_S(Context *ctx)
{
    int retval = 1;
    retval += parse_source_token(ctx, &ctx->source_args[0]);
    return retval;
} // parse_args_S


static int parse_args_SS(Context *ctx)
{
    int retval = 1;
    retval += parse_source_token(ctx, &ctx->source_args[0]);
    retval += parse_source_token(ctx, &ctx->source_args[1]);
    return retval;
} // parse_args_SS


static int parse_args_DS(Context *ctx)
{
    int retval = 1;
    retval += parse_destination_token(ctx, &ctx->dest_arg);
    retval += parse_source_token(ctx, &ctx->source_args[0]);
    return retval;
} // parse_args_DS


static int parse_args_DSS(Context *ctx)
{
    int retval = 1;
    retval += parse_destination_token(ctx, &ctx->dest_arg);
    retval += parse_source_token(ctx, &ctx->source_args[0]);
    retval += parse_source_token(ctx, &ctx->source_args[1]);
    return retval;
} // parse_args_DSS


static int parse_args_DSSS(Context *ctx)
{
    int retval = 1;
    retval += parse_destination_token(ctx, &ctx->dest_arg);
    retval += parse_source_token(ctx, &ctx->source_args[0]);
    retval += parse_source_token(ctx, &ctx->source_args[1]);
    retval += parse_source_token(ctx, &ctx->source_args[2]);
    return retval;
} // parse_args_DSSS


static int parse_args_DSSSS(Context *ctx)
{
    int retval = 1;
    retval += parse_destination_token(ctx, &ctx->dest_arg);
    retval += parse_source_token(ctx, &ctx->source_args[0]);
    retval += parse_source_token(ctx, &ctx->source_args[1]);
    retval += parse_source_token(ctx, &ctx->source_args[2]);
    retval += parse_source_token(ctx, &ctx->source_args[3]);
    return retval;
} // parse_args_DSSSS


static int parse_args_SINCOS(Context *ctx)
{
    // this opcode needs extra registers for sm2 and lower.
    if (!shader_version_atleast(ctx, 3, 0))
        return parse_args_DSSS(ctx);
    return parse_args_DS(ctx);
} // parse_args_SINCOS


static int parse_args_TEXCRD(Context *ctx)
{
    // added extra register in ps_1_4.
    if (shader_version_atleast(ctx, 1, 4))
        return parse_args_DS(ctx);
    return parse_args_D(ctx);
} // parse_args_TEXCRD


static int parse_args_TEXLD(Context *ctx)
{
    // different registers in px_1_3, ps_1_4, and ps_2_0!
    if (shader_version_atleast(ctx, 2, 0))
        return parse_args_DSS(ctx);
    else if (shader_version_atleast(ctx, 1, 4))
        return parse_args_DS(ctx);
    return parse_args_D(ctx);
} // parse_args_TEXLD


// State machine functions...

static ConstantsList *alloc_constant_listitem(Context *ctx)
{
    ConstantsList *item = malloc(sizeof(ConstantsList));
    memset(item, 0, sizeof(ConstantsList));
    item->next = ctx->constants;
    ctx->constants = item;
    ctx->constant_count++;
    return item;
}


static void state_DEF(Context *ctx)
{
    const RegisterType regtype = ctx->dest_arg.regtype;
    const int regnum = ctx->dest_arg.regnum;

    // !!! FIXME: fail if same register is defined twice.

    if (ctx->instruction_count != 0)
        fail(ctx, "DEF token must come before any instructions");
    else if (regtype != REG_TYPE_CONST)
        fail(ctx, "DEF token using invalid register");
    else
    {
        ConstantsList *item = alloc_constant_listitem(ctx);
        item->constant.index = regnum;
        item->constant.type = MOJOSHADER_UNIFORM_FLOAT;
        memcpy(item->constant.value.f, ctx->dwords,
               sizeof (item->constant.value.f));
        set_defined_register(ctx, regtype, regnum);
    }
}

static void state_DEFI(Context *ctx)
{
    const RegisterType regtype = ctx->dest_arg.regtype;
    const int regnum = ctx->dest_arg.regnum;

    // !!! FIXME: fail if same register is defined twice.

    if (ctx->instruction_count != 0)
        fail(ctx, "DEFI token must come before any instructions");
    else if (regtype != REG_TYPE_CONSTINT)
        fail(ctx, "DEFI token using invalid register");
    else
    {
        ConstantsList *item = alloc_constant_listitem(ctx);
        item->constant.index = regnum;
        item->constant.type = MOJOSHADER_UNIFORM_INT;
        memcpy(item->constant.value.i, ctx->dwords,
               sizeof (item->constant.value.i));

        set_defined_register(ctx, regtype, regnum);
    }
}

static void state_DEFB(Context *ctx)
{
    const RegisterType regtype = ctx->dest_arg.regtype;
    const int regnum = ctx->dest_arg.regnum;

    // !!! FIXME: fail if same register is defined twice.

    if (ctx->instruction_count != 0)
        fail(ctx, "DEFB token must come before any instructions");
    else if (regtype != REG_TYPE_CONSTBOOL)
        fail(ctx, "DEFB token using invalid register");
    else
    {
        ConstantsList *item = alloc_constant_listitem(ctx);
        item->constant.index = regnum;
        item->constant.type = MOJOSHADER_UNIFORM_BOOL;
        item->constant.value.b = ctx->dwords[0] ? 1 : 0;
        set_defined_register(ctx, regtype, regnum);
    }
}

static void state_DCL(Context *ctx)
{
    const DestArgInfo *arg = &ctx->dest_arg;
    const RegisterType regtype = arg->regtype;
    const int regnum = arg->regnum;
    const int wmask = arg->writemask;
    const int mods = arg->result_mod;

    // parse_args_DCL() does a lot of state checking before we get here.

    if (ctx->instruction_count != 0)
        fail(ctx, "DCL token must come before any instructions");
    else if (shader_is_vertex(ctx))
    {
        if (regtype == REG_TYPE_SAMPLER)
            add_sampler(ctx, regnum, (TextureType) ctx->dwords[0], 0);
        else
        {
            const MOJOSHADER_usage usage = (const MOJOSHADER_usage) ctx->dwords[0];
            const int index = ctx->dwords[1];
            if (usage >= MOJOSHADER_USAGE_TOTAL)
            {
                fail(ctx, "unknown DCL usage");
                return;
            }
            add_attribute_register(ctx, regtype, regnum, usage, index, wmask, mods);
        }
    }
    else if (shader_is_pixel(ctx))
    {
        if (regtype == REG_TYPE_SAMPLER)
            add_sampler(ctx, regnum, (TextureType) ctx->dwords[0], 0);
        else
        {
            const MOJOSHADER_usage usage = (MOJOSHADER_usage) ctx->dwords[0];
            const int index = ctx->dwords[1];
            add_attribute_register(ctx, regtype, regnum, usage, index, wmask, mods);
        }
    }
    else
    {
        fail(ctx, "unsupported shader type."); // should be caught elsewhere.
        return;
    }

    set_defined_register(ctx, regtype, regnum);
}

static void state_TEXCRD(Context *ctx)
{
    if (shader_version_atleast(ctx, 2, 0))
        fail(ctx, "TEXCRD in Shader Model >= 2.0");  // apparently removed.
}

static void state_FRC(Context *ctx)
{
    const DestArgInfo *dst = &ctx->dest_arg;

    if (dst->result_mod & MOD_SATURATE)  // according to msdn...
        fail(ctx, "FRC destination can't use saturate modifier");
    else if (!shader_version_atleast(ctx, 2, 0))
    {
        if (!writemask_y(dst->writemask) && !writemask_xy(dst->writemask))
            fail(ctx, "FRC writemask must be .y or .xy for shader model 1.x");
    }
}

// replicate the matrix registers to source args. The D3D profile will
//  only use the one legitimate argument, but this saves other profiles
//  from having to build this.
static void srcarg_matrix_replicate(Context *ctx, const int idx, const int rows)
{
    int i;
    SourceArgInfo *src = &ctx->source_args[idx];
    SourceArgInfo *dst = &ctx->source_args[idx+1];
    for (i = 0; i < (rows-1); i++, dst++)
    {
        memcpy(dst, src, sizeof (SourceArgInfo));
        dst->regnum += (i + 1);
        set_used_register(ctx, dst->regtype, dst->regnum, 0);
    }
}

static void state_M4X4(Context *ctx)
{
    const DestArgInfo *info = &ctx->dest_arg;
    if (!writemask_xyzw(info->writemask))
        fail(ctx, "M4X4 writemask must be full");

// !!! FIXME: MSDN:
//The xyzw (default) mask is required for the destination register. Negate and swizzle modifiers are allowed for src0, but not for src1.
//Swizzle and negate modifiers are invalid for the src0 register. The dest and src0 registers cannot be the same.

    srcarg_matrix_replicate(ctx, 1, 4);
} // state_M4X4

static void state_M4X3(Context *ctx)
{
    const DestArgInfo *info = &ctx->dest_arg;
    if (!writemask_xyz(info->writemask))
        fail(ctx, "M4X3 writemask must be .xyz");

// !!! FIXME: MSDN stuff

    srcarg_matrix_replicate(ctx, 1, 3);
} // state_M4X3

static void state_M3X4(Context *ctx)
{
    const DestArgInfo *info = &ctx->dest_arg;
    if (!writemask_xyzw(info->writemask))
        fail(ctx, "M3X4 writemask must be .xyzw");

// !!! FIXME: MSDN stuff

    srcarg_matrix_replicate(ctx, 1, 4);
} // state_M3X4

static void state_M3X3(Context *ctx)
{
    const DestArgInfo *info = &ctx->dest_arg;
    if (!writemask_xyz(info->writemask))
        fail(ctx, "M3X3 writemask must be .xyz");

// !!! FIXME: MSDN stuff

    srcarg_matrix_replicate(ctx, 1, 3);
} // state_M3X3

static void state_M3X2(Context *ctx)
{
    const DestArgInfo *info = &ctx->dest_arg;
    if (!writemask_xy(info->writemask))
        fail(ctx, "M3X2 writemask must be .xy");

// !!! FIXME: MSDN stuff

    srcarg_matrix_replicate(ctx, 1, 2);
} // state_M3X2

static void state_RET(Context *ctx)
{
    // MSDN all but says that assembly shaders are more or less serialized
    //  HLSL functions, and a RET means you're at the end of one, unlike how
    //  most CPUs would behave. This is actually really helpful,
    //  since we can use high-level constructs and not a mess of GOTOs,
    //  which is a godsend for GLSL...this also means we can consider things
    //  like a LOOP without a matching ENDLOOP within a label's section as
    //  an error.
    if (ctx->loops > 0)
        fail(ctx, "LOOP without ENDLOOP");
    if (ctx->reps > 0)
        fail(ctx, "REP without ENDREP");
} // state_RET

static void check_label_register(Context *ctx, int arg, const char *opcode)
{
    const SourceArgInfo *info = &ctx->source_args[arg];
    const RegisterType regtype = info->regtype;
    const int regnum = info->regnum;

    if (regtype != REG_TYPE_LABEL)
        failf(ctx, "%s with a non-label register specified", opcode);
    if (!shader_version_atleast(ctx, 2, 0))
        failf(ctx, "%s not supported in Shader Model 1", opcode);
    if ((shader_version_atleast(ctx, 2, 255)) && (regnum > 2047))
        fail(ctx, "label register number must be <= 2047");
    if (regnum > 15)
        fail(ctx, "label register number must be <= 15");
} // check_label_register

static void state_LABEL(Context *ctx)
{
    if (ctx->previous_opcode != OPCODE_RET)
        fail(ctx, "LABEL not followed by a RET");
    check_label_register(ctx, 0, "LABEL");
    set_defined_register(ctx, REG_TYPE_LABEL, ctx->source_args[0].regnum);
} // state_LABEL

static void check_call_loop_wrappage(Context *ctx, const int regnum)
{
    // msdn says subroutines inherit aL register if you're in a loop when
    //  you call, and further more _if you ever call this function in a loop,
    //  it must always be called in a loop_. So we'll just pass our loop
    //  variable as a function parameter in those cases.

    const int current_usage = (ctx->loops > 0) ? 1 : -1;
    RegisterList *reg = reglist_find(&ctx->used_registers, REG_TYPE_LABEL, regnum);
    assert(reg != NULL);

    if (reg->misc == 0)
        reg->misc = current_usage;
    else if (reg->misc != current_usage)
    {
        if (current_usage == 1)
            fail(ctx, "CALL to this label must be wrapped in LOOP/ENDLOOP");
        else
            fail(ctx, "CALL to this label must not be wrapped in LOOP/ENDLOOP");
    } // else if
} // check_call_loop_wrappage

static void state_CALL(Context *ctx)
{
    check_label_register(ctx, 0, "CALL");
    check_call_loop_wrappage(ctx, ctx->source_args[0].regnum);
} // state_CALL

static void state_CALLNZ(Context *ctx)
{
    const RegisterType regtype = ctx->source_args[1].regtype;
    if ((regtype != REG_TYPE_CONSTBOOL) && (regtype != REG_TYPE_PREDICATE))
        fail(ctx, "CALLNZ argument isn't constbool or predicate register");
    check_label_register(ctx, 0, "CALLNZ");
    check_call_loop_wrappage(ctx, ctx->source_args[0].regnum);
} // state_CALLNZ

static void state_MOVA(Context *ctx)
{
    if (ctx->dest_arg.regtype != REG_TYPE_ADDRESS)
        fail(ctx, "MOVA argument isn't address register");
} // state_MOVA

static void state_RCP(Context *ctx)
{
    if (!replicate_swizzle(ctx->source_args[0].swizzle))
        fail(ctx, "RCP without replicate swizzzle");
} // state_RCP

static void state_LOOP(Context *ctx)
{
    if (ctx->source_args[0].regtype != REG_TYPE_LOOP)
        fail(ctx, "LOOP argument isn't loop register");
    else if (ctx->source_args[1].regtype != REG_TYPE_CONSTINT)
        fail(ctx, "LOOP argument isn't constint register");
    else
        ctx->loops++;
} // state_LOOP

static void state_ENDLOOP(Context *ctx)
{
    // !!! FIXME: check that we aren't straddling an IF block.
    if (ctx->loops <= 0)
        fail(ctx, "ENDLOOP without LOOP");
    ctx->loops--;
} // state_ENDLOOP

static void state_BREAKP(Context *ctx)
{
    const RegisterType regtype = ctx->source_args[0].regtype;
    if (regtype != REG_TYPE_PREDICATE)
        fail(ctx, "BREAKP argument isn't predicate register");
    else if (!replicate_swizzle(ctx->source_args[0].swizzle))
        fail(ctx, "BREAKP without replicate swizzzle");
    else if ((ctx->loops == 0) && (ctx->reps == 0))
        fail(ctx, "BREAKP outside LOOP/ENDLOOP or REP/ENDREP");
} // state_BREAKP

static void state_BREAK(Context *ctx)
{
    if ((ctx->loops == 0) && (ctx->reps == 0))
        fail(ctx, "BREAK outside LOOP/ENDLOOP or REP/ENDREP");
} // state_BREAK

static void state_SETP(Context *ctx)
{
    const RegisterType regtype = ctx->dest_arg.regtype;
    if (regtype != REG_TYPE_PREDICATE)
        fail(ctx, "SETP argument isn't predicate register");
} // state_SETP

static void state_REP(Context *ctx)
{
    const RegisterType regtype = ctx->source_args[0].regtype;
    if (regtype != REG_TYPE_CONSTINT)
        fail(ctx, "REP argument isn't constint register");

    ctx->reps++;
    if (ctx->reps > ctx->max_reps)
        ctx->max_reps = ctx->reps;
} // state_REP

static void state_ENDREP(Context *ctx)
{
    // !!! FIXME: check that we aren't straddling an IF block.
    if (ctx->reps <= 0)
        fail(ctx, "ENDREP without REP");
    ctx->reps--;
} // state_ENDREP

static void state_CMP(Context *ctx)
{
    ctx->cmps++;

    // extra limitations for ps <= 1.4 ...
    if (!shader_version_atleast(ctx, 1, 4))
    {
        int i;
        const DestArgInfo *dst = &ctx->dest_arg;
        const RegisterType dregtype = dst->regtype;
        const int dregnum = dst->regnum;

        if (ctx->cmps > 3)
            fail(ctx, "only 3 CMP instructions allowed in this shader model");

        for (i = 0; i < 3; i++)
        {
            const SourceArgInfo *src = &ctx->source_args[i];
            const RegisterType sregtype = src->regtype;
            const int sregnum = src->regnum;
            if ((dregtype == sregtype) && (dregnum == sregnum))
                fail(ctx, "CMP dest can't match sources in this shader model");
        } // for

        ctx->instruction_count++;  // takes an extra slot in ps_1_2 and _3.
    } // if
} // state_CMP

static void state_DP4(Context *ctx)
{
    // extra limitations for ps <= 1.4 ...
    if (!shader_version_atleast(ctx, 1, 4))
        ctx->instruction_count++;  // takes an extra slot in ps_1_2 and _3.
} // state_DP4

static void state_CND(Context *ctx)
{
    // apparently it was removed...it's not in the docs past ps_1_4 ...
    if (shader_version_atleast(ctx, 2, 0))
        fail(ctx, "CND not allowed in this shader model");

    // extra limitations for ps <= 1.4 ...
    else if (!shader_version_atleast(ctx, 1, 4))
    {
        const SourceArgInfo *src = &ctx->source_args[0];
        if ((src->regtype != REG_TYPE_TEMP) || (src->regnum != 0) ||
            (src->swizzle != 0xFF))
        {
            fail(ctx, "CND src must be r0.a in this shader model");
        } // if
    } // if
} // state_CND

static void state_POW(Context *ctx)
{
    if (!replicate_swizzle(ctx->source_args[0].swizzle))
        fail(ctx, "POW src0 must have replicate swizzle");
    else if (!replicate_swizzle(ctx->source_args[1].swizzle))
        fail(ctx, "POW src1 must have replicate swizzle");
} // state_POW

static void state_LOG(Context *ctx)
{
    if (!replicate_swizzle(ctx->source_args[0].swizzle))
        fail(ctx, "LOG src0 must have replicate swizzle");
} // state_LOG

static void state_LOGP(Context *ctx)
{
    if (!replicate_swizzle(ctx->source_args[0].swizzle))
        fail(ctx, "LOGP src0 must have replicate swizzle");
} // state_LOGP

static void state_SINCOS(Context *ctx)
{
    const DestArgInfo *dst = &ctx->dest_arg;
    const int mask = dst->writemask;
    if (!writemask_x(mask) && !writemask_y(mask) && !writemask_xy(mask))
        fail(ctx, "SINCOS write mask must be .x or .y or .xy");

    else if (!replicate_swizzle(ctx->source_args[0].swizzle))
        fail(ctx, "SINCOS src0 must have replicate swizzle");

    else if (dst->result_mod & MOD_SATURATE)  // according to msdn...
        fail(ctx, "SINCOS destination can't use saturate modifier");

    // this opcode needs extra registers, with extra limitations, for <= sm2.
    else if (!shader_version_atleast(ctx, 3, 0))
    {
        int i;
        for (i = 1; i < 3; i++)
        {
            if (ctx->source_args[i].regtype != REG_TYPE_CONST)
            {
                failf(ctx, "SINCOS src%d must be constfloat", i);
                return;
            } // if
        } // for

        if (ctx->source_args[1].regnum == ctx->source_args[2].regnum)
            fail(ctx, "SINCOS src1 and src2 must be different registers");
    } // if
} // state_SINCOS

static void state_IF(Context *ctx)
{
    const RegisterType regtype = ctx->source_args[0].regtype;
    if ((regtype != REG_TYPE_PREDICATE) && (regtype != REG_TYPE_CONSTBOOL))
        fail(ctx, "IF src0 must be CONSTBOOL or PREDICATE");
    else if (!replicate_swizzle(ctx->source_args[0].swizzle))
        fail(ctx, "IF src0 must have replicate swizzle");
    // !!! FIXME: track if nesting depth.
} // state_IF

static void state_IFC(Context *ctx)
{
    if (!replicate_swizzle(ctx->source_args[0].swizzle))
        fail(ctx, "IFC src0 must have replicate swizzle");
    else if (!replicate_swizzle(ctx->source_args[1].swizzle))
        fail(ctx, "IFC src1 must have replicate swizzle");
    // !!! FIXME: track if nesting depth.
} // state_IFC

static void state_BREAKC(Context *ctx)
{
    if (!replicate_swizzle(ctx->source_args[0].swizzle))
        fail(ctx, "BREAKC src1 must have replicate swizzle");
    else if (!replicate_swizzle(ctx->source_args[1].swizzle))
        fail(ctx, "BREAKC src2 must have replicate swizzle");
    else if ((ctx->loops == 0) && (ctx->reps == 0))
        fail(ctx, "BREAKC outside LOOP/ENDLOOP or REP/ENDREP");
} // state_BREAKC

static void state_TEXKILL(Context *ctx)
{
    // The MSDN docs say this should be a source arg, but the driver docs
    //  say it's a dest arg. That's annoying.
    const DestArgInfo *info = &ctx->dest_arg;
    const RegisterType regtype = info->regtype;
    if (!writemask_xyzw(info->writemask))
        fail(ctx, "TEXKILL writemask must be .xyzw");
    else if ((regtype != REG_TYPE_TEMP) && (regtype != REG_TYPE_TEXTURE))
        fail(ctx, "TEXKILL must use a temp or texture register");

    // !!! FIXME: "If a temporary register is used, all components must have been previously written."
    // !!! FIXME: "If a texture register is used, all components that are read must have been declared."
    // !!! FIXME: there are further limitations in ps_1_3 and earlier.
} // state_TEXKILL

// Some rules that apply to some of the fruity ps_1_1 texture opcodes...
static void state_texops(Context *ctx, const char *opcode,
                         const int dims, const int texbem)
{
    const DestArgInfo *dst = &ctx->dest_arg;
    const SourceArgInfo *src = &ctx->source_args[0];
    if (dst->regtype != REG_TYPE_TEXTURE)
        failf(ctx, "%s destination must be a texture register", opcode);
    if (src->regtype != REG_TYPE_TEXTURE)
        failf(ctx, "%s source must be a texture register", opcode);
    if (src->regnum >= dst->regnum)  // so says MSDN.
        failf(ctx, "%s dest must be a higher register than source", opcode);

    if (dims)
    {
        TextureType ttyp = (dims == 2) ? TEXTURE_TYPE_2D : TEXTURE_TYPE_CUBE;
        add_sampler(ctx, dst->regnum, ttyp, texbem);
    } // if

    add_attribute_register(ctx, REG_TYPE_TEXTURE, dst->regnum,
                           MOJOSHADER_USAGE_TEXCOORD, dst->regnum, 0xF, 0);

    // Strictly speaking, there should be a TEX opcode prior to this call that
    //  should fill in this metadata, but I'm not sure that's required for the
    //  shader to assemble in D3D, so we'll do this so we don't fail with a
    //  cryptic error message even if the developer didn't do the TEX.
    add_attribute_register(ctx, REG_TYPE_TEXTURE, src->regnum,
                           MOJOSHADER_USAGE_TEXCOORD, src->regnum, 0xF, 0);
} // state_texops

static void state_texbem(Context *ctx, const char *opcode)
{
    // The TEXBEM equasion, according to MSDN:
    //u' = TextureCoordinates(stage m)u + D3DTSS_BUMPENVMAT00(stage m)*t(n)R
    //         + D3DTSS_BUMPENVMAT10(stage m)*t(n)G
    //v' = TextureCoordinates(stage m)v + D3DTSS_BUMPENVMAT01(stage m)*t(n)R
    //         + D3DTSS_BUMPENVMAT11(stage m)*t(n)G
    //t(m)RGBA = TextureSample(stage m)
    //
    // ...TEXBEML adds this at the end:
    //t(m)RGBA = t(m)RGBA * [(t(n)B * D3DTSS_BUMPENVLSCALE(stage m)) +
    //           D3DTSS_BUMPENVLOFFSET(stage m)]

    if (shader_version_atleast(ctx, 1, 4))
        failf(ctx, "%s opcode not available after Shader Model 1.3", opcode);

    if (!shader_version_atleast(ctx, 1, 2))
    {
        if (ctx->source_args[0].src_mod == SRCMOD_SIGN)
            failf(ctx, "%s forbids _bx2 on source reg before ps_1_2", opcode);
    } // if

    // !!! FIXME: MSDN:
    // !!! FIXME: Register data that has been read by a texbem
    // !!! FIXME:  or texbeml instruction cannot be read later,
    // !!! FIXME:  except by another texbem or texbeml.

    state_texops(ctx, opcode, 2, 1);
} // state_texbem

static void state_TEXBEM(Context *ctx)
{
    state_texbem(ctx, "TEXBEM");
} // state_TEXBEM

static void state_TEXBEML(Context *ctx)
{
    state_texbem(ctx, "TEXBEML");
} // state_TEXBEML

static void state_TEXM3X2PAD(Context *ctx)
{
    if (shader_version_atleast(ctx, 1, 4))
        fail(ctx, "TEXM3X2PAD opcode not available after Shader Model 1.3");
    state_texops(ctx, "TEXM3X2PAD", 0, 0);
    // !!! FIXME: check for correct opcode existance and order more rigorously?
    ctx->texm3x2pad_src0 = ctx->source_args[0].regnum;
    ctx->texm3x2pad_dst0 = ctx->dest_arg.regnum;
} // state_TEXM3X2PAD

static void state_TEXM3X2TEX(Context *ctx)
{
    if (shader_version_atleast(ctx, 1, 4))
        fail(ctx, "TEXM3X2TEX opcode not available after Shader Model 1.3");
    if (ctx->texm3x2pad_dst0 == -1)
        fail(ctx, "TEXM3X2TEX opcode without matching TEXM3X2PAD");
    // !!! FIXME: check for correct opcode existance and order more rigorously?
    state_texops(ctx, "TEXM3X2TEX", 2, 0);
    ctx->reset_texmpad = 1;

    RegisterList *sreg = reglist_find(&ctx->samplers, REG_TYPE_SAMPLER,
                                      ctx->dest_arg.regnum);
    const TextureType ttype = (TextureType) (sreg ? sreg->index : 0);

    // A samplermap might change this to something nonsensical.
    if (ttype != TEXTURE_TYPE_2D)
        fail(ctx, "TEXM3X2TEX needs a 2D sampler");
} // state_TEXM3X2TEX

static void state_TEXM3X3PAD(Context *ctx)
{
    if (shader_version_atleast(ctx, 1, 4))
        fail(ctx, "TEXM3X2TEX opcode not available after Shader Model 1.3");
    state_texops(ctx, "TEXM3X3PAD", 0, 0);

    // !!! FIXME: check for correct opcode existance and order more rigorously?
    if (ctx->texm3x3pad_dst0 == -1)
    {
        ctx->texm3x3pad_src0 = ctx->source_args[0].regnum;
        ctx->texm3x3pad_dst0 = ctx->dest_arg.regnum;
    } // if
    else if (ctx->texm3x3pad_dst1 == -1)
    {
        ctx->texm3x3pad_src1 = ctx->source_args[0].regnum;
        ctx->texm3x3pad_dst1 = ctx->dest_arg.regnum;
    } // else
} // state_TEXM3X3PAD

static void state_texm3x3(Context *ctx, const char *opcode, const int dims)
{
    // !!! FIXME: check for correct opcode existance and order more rigorously?
    if (shader_version_atleast(ctx, 1, 4))
        failf(ctx, "%s opcode not available after Shader Model 1.3", opcode);
    if (ctx->texm3x3pad_dst1 == -1)
        failf(ctx, "%s opcode without matching TEXM3X3PADs", opcode);
    state_texops(ctx, opcode, dims, 0);
    ctx->reset_texmpad = 1;

    RegisterList *sreg = reglist_find(&ctx->samplers, REG_TYPE_SAMPLER,
                                      ctx->dest_arg.regnum);
    const TextureType ttype = (TextureType) (sreg ? sreg->index : 0);

    // A samplermap might change this to something nonsensical.
    if ((ttype != TEXTURE_TYPE_VOLUME) && (ttype != TEXTURE_TYPE_CUBE))
        failf(ctx, "%s needs a 3D or Cubemap sampler", opcode);
} // state_texm3x3

static void state_TEXM3X3(Context *ctx)
{
    if (!shader_version_atleast(ctx, 1, 2))
        fail(ctx, "TEXM3X3 opcode not available in Shader Model 1.1");
    state_texm3x3(ctx, "TEXM3X3", 0);
} // state_TEXM3X3

static void state_TEXM3X3TEX(Context *ctx)
{
    state_texm3x3(ctx, "TEXM3X3TEX", 3);
} // state_TEXM3X3TEX

static void state_TEXM3X3SPEC(Context *ctx)
{
    state_texm3x3(ctx, "TEXM3X3SPEC", 3);
    if (ctx->source_args[1].regtype != REG_TYPE_CONST)
        fail(ctx, "TEXM3X3SPEC final arg must be a constant register");
} // state_TEXM3X3SPEC

static void state_TEXM3X3VSPEC(Context *ctx)
{
    state_texm3x3(ctx, "TEXM3X3VSPEC", 3);
} // state_TEXM3X3VSPEC


static void state_TEXLD(Context *ctx)
{
    if (shader_version_atleast(ctx, 2, 0))
    {
        const SourceArgInfo *src0 = &ctx->source_args[0];
        const SourceArgInfo *src1 = &ctx->source_args[1];

        // !!! FIXME: verify texldp restrictions:
        //http://msdn.microsoft.com/en-us/library/bb206221(VS.85).aspx
        // !!! FIXME: ...and texldb, too.
        //http://msdn.microsoft.com/en-us/library/bb206217(VS.85).aspx

        //const RegisterType rt0 = src0->regtype;

        // !!! FIXME: msdn says it has to be temp, but Microsoft's HLSL
        // !!! FIXME:  compiler is generating code that uses oC0 for a dest.
        //if (ctx->dest_arg.regtype != REG_TYPE_TEMP)
        //    fail(ctx, "TEXLD dest must be a temp register");

        // !!! FIXME: this can be an REG_TYPE_INPUT, DCL'd to TEXCOORD.
        //else if ((rt0 != REG_TYPE_TEXTURE) && (rt0 != REG_TYPE_TEMP))
        //    fail(ctx, "TEXLD src0 must be texture or temp register");
        //else

        if (src0->src_mod != SRCMOD_NONE)
            fail(ctx, "TEXLD src0 must have no modifiers");
        else if (src1->regtype != REG_TYPE_SAMPLER)
            fail(ctx, "TEXLD src1 must be sampler register");
        else if (src1->src_mod != SRCMOD_NONE)
            fail(ctx, "TEXLD src1 must have no modifiers");
        else if ( (ctx->instruction_controls != CONTROL_TEXLD) &&
                  (ctx->instruction_controls != CONTROL_TEXLDP) &&
                  (ctx->instruction_controls != CONTROL_TEXLDB) )
        {
            fail(ctx, "TEXLD has unknown control bits");
        } // else if

        // Shader Model 3 added swizzle support to this opcode.
        if (!shader_version_atleast(ctx, 3, 0))
        {
            if (!no_swizzle(src0->swizzle))
                fail(ctx, "TEXLD src0 must not swizzle");
            else if (!no_swizzle(src1->swizzle))
                fail(ctx, "TEXLD src1 must not swizzle");
        } // if

        if ( ((TextureType) ctx->source_args[1].regnum) == TEXTURE_TYPE_CUBE )
            ctx->instruction_count += 3;
    } // if

    else if (shader_version_atleast(ctx, 1, 4))
    {
        // !!! FIXME: checks for ps_1_4 version here...
    } // else if

    else
    {
        // !!! FIXME: add (other?) checks for ps_1_1 version here...
        const DestArgInfo *info = &ctx->dest_arg;
        const int sampler = info->regnum;
        if (info->regtype != REG_TYPE_TEXTURE)
            fail(ctx, "TEX param must be a texture register");
        add_sampler(ctx, sampler, TEXTURE_TYPE_2D, 0);
        add_attribute_register(ctx, REG_TYPE_TEXTURE, sampler,
                               MOJOSHADER_USAGE_TEXCOORD, sampler, 0xF, 0);
    } // else
} // state_TEXLD

static void state_TEXLDL(Context *ctx)
{
    if (!shader_version_atleast(ctx, 3, 0))
        fail(ctx, "TEXLDL in version < Shader Model 3.0");
    else if (ctx->source_args[1].regtype != REG_TYPE_SAMPLER)
        fail(ctx, "TEXLDL src1 must be sampler register");
    else
    {
        if ( ((TextureType) ctx->source_args[1].regnum) == TEXTURE_TYPE_CUBE )
            ctx->instruction_count += 3;
    } // else
} // state_TEXLDL

static void state_DP2ADD(Context *ctx)
{
    if (!replicate_swizzle(ctx->source_args[2].swizzle))
        fail(ctx, "DP2ADD src2 must have replicate swizzle");
} // state_DP2ADD


// Lookup table for instruction opcodes...
typedef struct
{
    const char *opcode_string;
    int slots;  // number of instruction slots this opcode eats.
    MOJOSHADER_shaderType shader_types;  // mask of types that can use opcode.
    args_function parse_args;
    state_function state;
    emit_function emitter[STATICARRAYLEN(profiles)];
} Instruction;

// These have to be in the right order! This array is indexed by the value
//  of the instruction token.
static const Instruction instructions[] =
{
    #define INSTRUCTION_STATE(op, opstr, slots, a, t) { \
        opstr, slots, t, parse_args_##a, state_##op, PROFILE_EMITTERS(op) \
    },

    #define INSTRUCTION(op, opstr, slots, a, t) { \
        opstr, slots, t, parse_args_##a, 0, PROFILE_EMITTERS(op) \
    },

    #define MOJOSHADER_DO_INSTRUCTION_TABLE 1
    #include "mojoshader_internal.h"
    #undef MOJOSHADER_DO_INSTRUCTION_TABLE

    #undef INSTRUCTION
    #undef INSTRUCTION_STATE
};


// parse various token types...

static int parse_instruction_token(Context *ctx)
{
    int retval = 0;
    const int start_position = ctx->current_position;
    const uint32 *start_tokens = ctx->tokens;
    const uint32 start_tokencount = ctx->tokencount;
    const uint32 token = *(ctx->tokens);
    const uint32 opcode = (token & 0xFFFF);
    const uint32 controls = ((token>>16) & 0xFF);
    const uint32 insttoks = ((token>>24) & 0x0F);
    const int coissue = (token & 0x40000000) ? 1 : 0;
    const int predicated = (token & 0x10000000) ? 1 : 0;

    if ( opcode >= (sizeof (instructions) / sizeof (instructions[0])) )
        return 0;  // not an instruction token, or just not handled here.

    const Instruction *instruction = &instructions[opcode];
    const emit_function emitter = instruction->emitter[ctx->profileid];

    if ((token & 0x80000000) != 0)
        fail(ctx, "instruction token high bit must be zero.");  // so says msdn.

    if (instruction->opcode_string == NULL)
    {
        fail(ctx, "Unknown opcode.");
        return insttoks + 1;  // pray that you resync later.
    } // if

    if (coissue)
    {
        if (!shader_is_pixel(ctx))
            fail(ctx, "coissue instruction on non-pixel shader");
        if (shader_version_atleast(ctx, 2, 0))
            fail(ctx, "coissue instruction in Shader Model >= 2.0");
    } // if

    if ((ctx->shader_type & instruction->shader_types) == 0)
    {
        failf(ctx, "opcode '%s' not available in this shader type.",
                instruction->opcode_string);
    } // if

    memset(ctx->dwords, '\0', sizeof (ctx->dwords));
    ctx->instruction_controls = controls;
    ctx->predicated = predicated;

    // Update the context with instruction's arguments.
    adjust_token_position(ctx, 1);
    retval = instruction->parse_args(ctx);

    if (predicated)
        retval += parse_predicated_token(ctx);

    // parse_args() moves these forward for convenience...reset them.
    ctx->tokens = start_tokens;
    ctx->tokencount = start_tokencount;
    ctx->current_position = start_position;

    if (instruction->state != NULL)
        instruction->state(ctx);

    ctx->instruction_count += instruction->slots;

    if (!isfail(ctx))
        emitter(ctx);  // call the profile's emitter.

    if (ctx->reset_texmpad)
    {
        ctx->texm3x2pad_dst0 = -1;
        ctx->texm3x2pad_src0 = -1;
        ctx->texm3x3pad_dst0 = -1;
        ctx->texm3x3pad_src0 = -1;
        ctx->texm3x3pad_dst1 = -1;
        ctx->texm3x3pad_src1 = -1;
        ctx->reset_texmpad = 0;
    } // if

    ctx->previous_opcode = opcode;

    if (!shader_version_atleast(ctx, 2, 0))
    {
        if (insttoks != 0)  // reserved field in shaders < 2.0 ...
            fail(ctx, "instruction token count must be zero");
    } // if
    else
    {
        if (((uint32)retval) != (insttoks+1))
        {
            failf(ctx, "wrong token count (%u, not %u) for opcode '%s'.",
                    (uint) retval, (uint) (insttoks+1),
                    instruction->opcode_string);
            retval = insttoks + 1;  // try to keep sync.
        } // if
    } // else

    return retval;
} // parse_instruction_token


static int parse_version_token(Context *ctx, const char *profilestr)
{
    if (ctx->tokencount == 0)
    {
        fail(ctx, "Expected version token, got none at all.");
        return 0;
    } // if

    const uint32 token = *(ctx->tokens);
    const uint32 shadertype = ((token>>16) & 0xFFFF);
    const uint8 major = (uint8) ((token>>8) & 0xFF);
    const uint8 minor = (uint8) (token & 0xFF);

    ctx->version_token = token;

    // 0xFFFF == pixel shader, 0xFFFE == vertex shader
    if(shadertype == 0xFFFF)
    {
        ctx->shader_type = MOJOSHADER_TYPE_PIXEL;
        ctx->shader_type_str = "ps";
    }
    else if(shadertype == 0xFFFE)
    {
        ctx->shader_type = MOJOSHADER_TYPE_VERTEX;
        ctx->shader_type_str = "vs";
    }
    else // geometry shader? Bogus data?
    {
        failf(ctx, "Unsupported shader type, 0x%x (%u.%u)", shadertype, major, minor);
        return -1;
    }

    ctx->major_ver = major;
    ctx->minor_ver = minor;

    if(!shader_version_supported(major, minor))
    {
        failf(ctx, "Shader Model %u.%u is currently unsupported.",
              (uint)major, (uint)minor);
    }

    if(!isfail(ctx))
        ctx->profile->start_emitter(ctx, profilestr);

    return 1;  // ate one token.
} // parse_version_token


static int is_comment_token(Context *ctx, const uint32 token, uint32 *tokcount)
{
    if((token&0xFFFF) == 0xFFFE)  // actually a comment token?
    {
        if ((token&0x80000000) != 0)
            fail(ctx, "comment token high bit must be zero.");  // so says msdn.
        *tokcount = ((token>>16) & 0xFFFF);
        return 1;
    }

    return 0;
}


static int parse_comment_token(Context *ctx)
{
    uint32 commenttoks = 0;
    if(is_comment_token(ctx, *ctx->tokens, &commenttoks))
        return commenttoks + 1;  // comment data plus the initial token.
    return 0;
}


static int parse_end_token(Context *ctx)
{
    if(*(ctx->tokens) != 0x0000FFFF)   // end token always 0x0000FFFF.
        return 0;  // not us, eat no tokens.

    if(!ctx->know_shader_size)  // this is the end of stream!
        ctx->tokencount = 1;
    else if(ctx->tokencount != 1)  // we _must_ be last. If not: fail.
        fail(ctx, "end token before end of stream");

    if(!isfail(ctx))
        ctx->profile->end_emitter(ctx);

    return 1;
}


static int parse_phase_token(Context *ctx)
{
    // !!! FIXME: needs state; allow only one phase token per shader, I think?
    if(*(ctx->tokens) != 0x0000FFFD) // phase token always 0x0000FFFD.
        return 0;  // not us, eat no tokens.

    if(!shader_is_pixel(ctx) || !shader_version_exactly(ctx, 1, 4))
        fail(ctx, "phase token only available in 1.4 pixel shaders");

    if(!isfail(ctx))
        ctx->profile->phase_emitter(ctx);

    return 1;
} // parse_phase_token


static int parse_token(Context *ctx)
{
    int rc = 0;

    assert(ctx->output_stack_len == 0);

    if (ctx->tokencount == 0)
        fail(ctx, "unexpected end of shader.");

    else if ((rc = parse_comment_token(ctx)) != 0)
        return rc;

    else if ((rc = parse_end_token(ctx)) != 0)
        return rc;

    else if ((rc = parse_phase_token(ctx)) != 0)
        return rc;

    else if ((rc = parse_instruction_token(ctx)) != 0)
        return rc;

    failf(ctx, "unknown token (0x%x)", (uint) *ctx->tokens);
    return 1;  // good luck!
} // parse_token


static int find_profile_id(const char *profile)
{
    size_t i;
    for (i = 0; i < STATICARRAYLEN(profileMap); i++)
    {
        const char *name = profileMap[i].from;
        if (strcmp(name, profile) == 0)
        {
            profile = profileMap[i].to;
            break;
        } // if
    } // for

    for (i = 0; i < STATICARRAYLEN(profiles); i++)
    {
        const char *name = profiles[i].name;
        if (strcmp(name, profile) == 0)
            return i;
    } // for

    return -1;  // no match.
} // find_profile_id


static Context *build_context(const char *profile,
                              const unsigned char *tokenbuf,
                              const unsigned int bufsize,
                              const MOJOSHADER_samplerMap *smap,
                              const unsigned int smapcount,
                              const unsigned int shadowsamp)
{
    Context *ctx = malloc(sizeof(Context));
    memset(ctx, 0, sizeof (Context));

    ctx->tokens = (const uint32 *) tokenbuf;
    ctx->orig_tokens = (const uint32 *) tokenbuf;
    ctx->know_shader_size = (bufsize != 0);
    ctx->tokencount = ctx->know_shader_size ? (bufsize / sizeof (uint32)) : 0xFFFFFFFF;
    ctx->samplermap = smap;
    ctx->samplermap_count = smapcount;
    ctx->shadow_samplers = shadowsamp;
    ctx->current_position = MOJOSHADER_POSITION_BEFORE;
    ctx->texm3x2pad_dst0 = -1;
    ctx->texm3x2pad_src0 = -1;
    ctx->texm3x3pad_dst0 = -1;
    ctx->texm3x3pad_src0 = -1;
    ctx->texm3x3pad_dst1 = -1;
    ctx->texm3x3pad_src1 = -1;

    ctx->errors = errorlist_create();
    if(ctx->errors == NULL)
    {
        free(ctx);
        return NULL;
    }

    if(!set_output(ctx, &ctx->mainline))
    {
        errorlist_destroy(ctx->errors);
        free(ctx);
        return NULL;
    }

    const int profileid = find_profile_id(profile);
    ctx->profileid = profileid;
    if(profileid >= 0)
        ctx->profile = &profiles[profileid];
    else
        failf(ctx, "Profile '%s' is unknown or unsupported", profile);

    return ctx;
}


static void free_constants_list(ConstantsList *item)
{
    while (item != NULL)
    {
        ConstantsList *next = item->next;
        free(item);
        item = next;
    }
}

static void destroy_context(Context *ctx)
{
    if(!ctx)
        return;

    buffer_destroy(ctx->preflight);
    buffer_destroy(ctx->globals);
    buffer_destroy(ctx->helpers);
    buffer_destroy(ctx->subroutines);
    buffer_destroy(ctx->mainline_intro);
    buffer_destroy(ctx->mainline);
    buffer_destroy(ctx->ignore);
    free_constants_list(ctx->constants);
    free_reglist(ctx->used_registers.next);
    free_reglist(ctx->defined_registers.next);
    free_reglist(ctx->uniforms.next);
    free_reglist(ctx->attributes.next);
    free_reglist(ctx->samplers.next);
    errorlist_destroy(ctx->errors);
    free(ctx);
}


static char *build_output(Context *ctx, size_t *len)
{
    // add a byte for a null terminator.
    Buffer *buffers[] = {
        ctx->preflight, ctx->globals, ctx->helpers,
        ctx->subroutines, ctx->mainline_intro, ctx->mainline
        // don't append ctx->ignore ... that's why it's called "ignore"
    };
    return buffer_merge(buffers, STATICARRAYLEN(buffers), len);
}


static inline const char *alloc_varname(Context *ctx, const RegisterList *reg)
{
    return ctx->profile->get_varname(ctx, reg->regtype, reg->regnum);
}


static MOJOSHADER_constant *build_constants(Context *ctx)
{
    const size_t len = sizeof(MOJOSHADER_constant) * ctx->constant_count;
    MOJOSHADER_constant *retval = malloc(len);

    ConstantsList *item = ctx->constants;
    int i;
    for(i = 0; i < ctx->constant_count; i++)
    {
        if(item == NULL)
        {
            fail(ctx, "BUG: mismatched constant list and count");
            break;
        }

        retval[i] = item->constant;
        item = item->next;
    }

    return retval;
}

static MOJOSHADER_sampler *build_samplers(Context *ctx)
{
    const size_t len = sizeof(MOJOSHADER_sampler) * ctx->sampler_count;
    MOJOSHADER_sampler *retval = malloc(len);

    RegisterList *item = ctx->samplers.next;
    int i;

    memset(retval, 0, len);

    for(i = 0; i < ctx->sampler_count; i++)
    {
        if(item == NULL)
        {
            fail(ctx, "BUG: mismatched sampler list and count");
            break;
        }

        assert(item->regtype == REG_TYPE_SAMPLER);
        retval[i].type = cvtD3DToMojoSamplerType((TextureType) item->index);
        retval[i].index = item->regnum;
        retval[i].name = alloc_varname(ctx, item);
        retval[i].texbem = (item->misc != 0) ? 1 : 0;
        item = item->next;
    }

    return retval;
}


static MOJOSHADER_attribute *build_attributes(Context *ctx, int *_count)
{
    if(ctx->attribute_count == 0)
    {
        *_count = 0;
        return NULL;
    }

    const size_t len = sizeof (MOJOSHADER_attribute) * ctx->attribute_count;
    MOJOSHADER_attribute *retval = malloc(len);
    memset(retval, 0, len);

    RegisterList *item = ctx->attributes.next;
    MOJOSHADER_attribute *wptr = retval;
    int ignore = 0;
    int count = 0;
    int i;

    for(i = 0; i < ctx->attribute_count; i++)
    {
        if(item == NULL)
        {
            fail(ctx, "BUG: mismatched attribute list and count");
            break;
        }

        switch(item->regtype)
        {
            case REG_TYPE_RASTOUT:
            case REG_TYPE_ATTROUT:
            case REG_TYPE_TEXCRDOUT:
            case REG_TYPE_COLOROUT:
            case REG_TYPE_DEPTHOUT:
                ignore = 1;
                break;
            case REG_TYPE_TEXTURE:
            case REG_TYPE_MISCTYPE:
            case REG_TYPE_INPUT:
                ignore = shader_is_pixel(ctx);
                break;
            default:
                ignore = 0;
                break;
        }

        if(!ignore)
        {
            if(shader_is_pixel(ctx))
                fail(ctx, "BUG: pixel shader with vertex attributes");
            else
            {
                wptr->usage = item->usage;
                wptr->index = item->index;
                wptr->name = alloc_varname(ctx, item);
                wptr++;
                count++;
            }
        }

        item = item->next;
    }

    *_count = count;
    return retval;
}

static MOJOSHADER_attribute *build_outputs(Context *ctx, int *_count)
{
    int count = 0;

    if (ctx->attribute_count == 0)
    {
        *_count = 0;
        return NULL;
    }

    const size_t len = sizeof(MOJOSHADER_attribute) * ctx->attribute_count;
    MOJOSHADER_attribute *retval = malloc(len);
    memset(retval, 0, len);

    RegisterList *item = ctx->attributes.next;
    MOJOSHADER_attribute *wptr = retval;
    int i;

    for(i = 0; i < ctx->attribute_count; i++)
    {
        if(item == NULL)
        {
            fail(ctx, "BUG: mismatched attribute list and count");
            break;
        }

        switch (item->regtype)
        {
            case REG_TYPE_RASTOUT:
            case REG_TYPE_ATTROUT:
            case REG_TYPE_TEXCRDOUT:
            case REG_TYPE_COLOROUT:
            case REG_TYPE_DEPTHOUT:
                wptr->usage = item->usage;
                wptr->index = item->index;
                wptr->name = alloc_varname(ctx, item);
                wptr++;
                count++;
                break;
            default:
                break;
        }

        item = item->next;
    }

    *_count = count;
    return retval;
} // build_outputs


static MOJOSHADER_parseData *build_parsedata(Context *ctx)
{
    char *output = NULL;
    MOJOSHADER_constant *constants = NULL;
    MOJOSHADER_attribute *attributes = NULL;
    MOJOSHADER_attribute *outputs = NULL;
    MOJOSHADER_sampler *samplers = NULL;
    MOJOSHADER_error *errors = NULL;
    MOJOSHADER_parseData *retval = NULL;
    size_t output_len = 0;
    int attribute_count = 0;
    int output_count = 0;

    retval = malloc(sizeof(MOJOSHADER_parseData));
    memset(retval, '\0', sizeof (MOJOSHADER_parseData));

    if(!isfail(ctx)) output = build_output(ctx, &output_len);
    if(!isfail(ctx)) constants = build_constants(ctx);
    if(!isfail(ctx)) attributes = build_attributes(ctx, &attribute_count);
    if(!isfail(ctx)) outputs = build_outputs(ctx, &output_count);
    if(!isfail(ctx)) samplers = build_samplers(ctx);

    const int error_count = errorlist_count(ctx->errors);
    errors = errorlist_flatten(ctx->errors);

    // check again, in case build_output, etc, ran out of memory.
    if (isfail(ctx))
    {
        int i;

        free(output);
        free(constants);

        if (attributes != NULL)
        {
            for (i = 0; i < attribute_count; i++)
                free((void*)attributes[i].name);
            free(attributes);
        }

        if (outputs != NULL)
        {
            for (i = 0; i < output_count; i++)
                free((void*)outputs[i].name);
            free(outputs);
        }

        if (samplers != NULL)
        {
            for(i = 0; i < ctx->sampler_count; i++)
                free((void*)samplers[i].name);
            free(samplers);
        }
    }
    else
    {
        retval->profile = ctx->profile->name;
        retval->output = output;
        retval->output_len = (int) output_len;
        retval->instruction_count = ctx->instruction_count;
        retval->token_count = ctx->tokens - ctx->orig_tokens;
        retval->shader_type = ctx->shader_type;
        retval->major_ver = (int) ctx->major_ver;
        retval->minor_ver = (int) ctx->minor_ver;
        retval->constant_count = ctx->constant_count;
        retval->constants = constants;
        retval->sampler_count = ctx->sampler_count;
        retval->samplers = samplers;
        retval->attribute_count = attribute_count;
        retval->attributes = attributes;
        retval->output_count = output_count;
        retval->outputs = outputs;
    }

    retval->error_count = error_count;
    retval->errors = errors;

    return retval;
}


static void process_definitions(Context *ctx)
{
    // !!! FIXME: apparently, pre ps_3_0, sampler registers don't need to be
    // !!! FIXME:  DCL'd before use (default to 2d?). We aren't checking
    // !!! FIXME:  this at the moment, though.

    determine_constants_arrays(ctx);  // in case this hasn't been called yet.

    RegisterList *uitem = &ctx->uniforms;
    RegisterList *prev = &ctx->used_registers;
    RegisterList *item = prev->next;

    while (item != NULL)
    {
        RegisterList *next = item->next;
        const RegisterType regtype = item->regtype;
        const int regnum = item->regnum;

        if (!get_defined_register(ctx, regtype, regnum))
        {
            // haven't already dealt with this one.
            switch (regtype)
            {
                // !!! FIXME: I'm not entirely sure this is right...
                case REG_TYPE_RASTOUT:
                case REG_TYPE_ATTROUT:
                case REG_TYPE_TEXCRDOUT:
                case REG_TYPE_COLOROUT:
                case REG_TYPE_DEPTHOUT:
                    if (shader_is_vertex(ctx) && shader_version_atleast(ctx,3,0))
                    {
                        fail(ctx, "vs_3 can't use output registers"
                                  " without declaring them first.");
                        return;
                    }

                    // Apparently this is an attribute that wasn't DCL'd.
                    //  Add it to the attribute list; deal with it later.
                    // !!! FIXME: we should use something other than UNKNOWN here.
                    add_attribute_register(ctx, regtype, regnum,
                                           MOJOSHADER_USAGE_UNKNOWN, 0, 0xF, 0);
                    break;

                case REG_TYPE_ADDRESS:
                case REG_TYPE_PREDICATE:
                case REG_TYPE_TEMP:
                case REG_TYPE_LOOP:
                case REG_TYPE_LABEL:
                    ctx->profile->global_emitter(ctx, regtype, regnum);
                    break;

                case REG_TYPE_CONST:
                case REG_TYPE_CONSTINT:
                case REG_TYPE_CONSTBOOL:
                    // separate uniforms into a different list for now.
                    prev->next = next;
                    item->next = NULL;
                    uitem->next = item;
                    uitem = item;
                    item = prev;
                    break;

                case REG_TYPE_INPUT:
                    // You don't have to dcl_ your inputs in Shader Model 1.
                    if (shader_is_pixel(ctx) && !shader_version_atleast(ctx,2,0))
                    {
                        add_attribute_register(ctx, regtype, regnum,
                                               MOJOSHADER_USAGE_COLOR, regnum,
                                               0xF, 0);
                        break;
                    }
                    // fall through...

                default:
                    fail(ctx, "BUG: we used a register we don't know how to define.");
            }
        }

        prev = item;
        item = next;
    }

    // ...and uniforms...
    for (item = ctx->uniforms.next; item != NULL; item = item->next)
    {
        ctx->profile->uniform_emitter(ctx, item->regtype, item->regnum);

        ctx->uniform_count++;
        switch (item->regtype)
        {
            case REG_TYPE_CONST:
                ctx->uniform_float4_count = Max(ctx->uniform_float4_count, item->regnum+1);
                break;
            case REG_TYPE_CONSTINT:
                ctx->uniform_int4_count = Max(ctx->uniform_int4_count, item->regnum+1);
                break;
            case REG_TYPE_CONSTBOOL:
                ctx->uniform_bool_count = Max(ctx->uniform_bool_count, item->regnum+1);
                break;
            default: break;
        }
    }
    if(ctx->have_relative_const_registers && ctx->uniform_float4_count > 0)
    {
        // If using relative constant arrays, define them all.
        if(shader_is_vertex(ctx))
            ctx->uniform_float4_count = Max(ctx->uniform_float4_count, 256);
        else if(shader_is_pixel(ctx))
            ctx->uniform_float4_count = Max(ctx->uniform_float4_count, 224);
    }

    // ...and samplers...
    for (item = ctx->samplers.next; item != NULL; item = item->next)
    {
        ctx->sampler_count++;
        ctx->profile->sampler_emitter(ctx, item->regnum,
                                      (TextureType) item->index,
                                      item->misc != 0);
    }

    // ...and attributes...
    for (item = ctx->attributes.next; item != NULL; item = item->next)
    {
        ctx->attribute_count++;
        ctx->profile->attribute_emitter(ctx, item->regtype, item->regnum,
                                        item->usage, item->index,
                                        item->writemask, item->misc);
    }
}


// API entry point...

// !!! FIXME:
// MSDN: "Shader validation will fail CreatePixelShader on any shader that
//  attempts to read from a temporary register that has not been written by a
//  previous instruction."  (true for ps_1_*, maybe others). Check this.

const MOJOSHADER_parseData *MOJOSHADER_parse(const char *profile,
                                             const unsigned char *tokenbuf,
                                             const unsigned int bufsize,
                                             const MOJOSHADER_samplerMap *smap,
                                             const unsigned int smapcount,
                                             const unsigned int shadowsamp)
{
    MOJOSHADER_parseData *retval = NULL;
    Context *ctx = NULL;
    int rc = 0;
    int failed = 0;

    ctx = build_context(profile, tokenbuf, bufsize, smap, smapcount, shadowsamp);
    if(isfail(ctx))
    {
        retval = build_parsedata(ctx);
        destroy_context(ctx);
        return retval;
    }

    // Version token always comes first.
    ctx->current_position = 0;
    rc = parse_version_token(ctx, profile);

    // drop out now if this definitely isn't bytecode. Saves lots of
    //  meaningless errors flooding through.
    if(rc < 0)
    {
        retval = build_parsedata(ctx);
        destroy_context(ctx);
        return retval;
    }

    if(((uint32)rc) > ctx->tokencount)
    {
        fail(ctx, "Corrupted or truncated shader");
        ctx->tokencount = rc;
    }

    adjust_token_position(ctx, rc);

    // parse out the rest of the tokens after the version token...
    while(ctx->tokencount > 0)
    {
        if(!ctx->know_shader_size)
            ctx->tokencount = 0xFFFFFFFF;  // keep this value obscenely large.

        // reset for each token.
        if(isfail(ctx))
        {
            failed = 1;
            ctx->isfail = 0;
        }

        rc = parse_token(ctx);
        if(((uint32)rc) > ctx->tokencount)
        {
            fail(ctx, "Corrupted or truncated shader");
            break;
        }

        adjust_token_position(ctx, rc);
    }

    ctx->current_position = MOJOSHADER_POSITION_AFTER;

    // for ps_1_*, the output color is written to r0...throw an
    //  error if this register was never written. This isn't
    //  important for vertex shaders, or shader model 2+.
    if(shader_is_pixel(ctx) && !shader_version_atleast(ctx, 2, 0))
    {
        if(!register_was_written(ctx, REG_TYPE_TEMP, 0))
            fail(ctx, "r0 (pixel shader 1.x color output) never written to");
    }

    if(!failed)
    {
        process_definitions(ctx);
        failed = isfail(ctx);
    }

    if(!failed)
        ctx->profile->finalize_emitter(ctx);

    ctx->isfail = failed;
    retval = build_parsedata(ctx);
    destroy_context(ctx);
    return retval;
}


void MOJOSHADER_freeParseData(const MOJOSHADER_parseData *_data)
{
    MOJOSHADER_parseData *data = (MOJOSHADER_parseData*)_data;
    if(data == NULL) return;  // no-op.

    int i;

    // we don't f(data->profile), because that's internal static data.

    free((void*)data->output);
    free((void*)data->constants);

    for(i = 0; i < data->error_count; i++)
    {
        free((void*)data->errors[i].error);
        free((void*)data->errors[i].filename);
    }
    free((void*)data->errors);

    for(i = 0; i < data->attribute_count; i++)
        free((void*)data->attributes[i].name);
    free((void*)data->attributes);

    for(i = 0; i < data->output_count; i++)
        free((void*)data->outputs[i].name);
    free((void*)data->outputs);

    for(i = 0; i < data->sampler_count; i++)
        free((void*)data->samplers[i].name);
    free((void*)data->samplers);

    free(data);
}


int MOJOSHADER_version(void)
{
    return MOJOSHADER_VERSION;
}


const char *MOJOSHADER_changeset(void)
{
    return MOJOSHADER_CHANGESET;
}


int MOJOSHADER_maxShaderModel(const char *profile)
{
    #define PROFILE_SHADER_MODEL(p,v) if (strcmp(profile, p) == 0) return v;
    PROFILE_SHADER_MODEL(MOJOSHADER_PROFILE_GLSL, 3);
    PROFILE_SHADER_MODEL(MOJOSHADER_PROFILE_GLSL120, 3);
    PROFILE_SHADER_MODEL(MOJOSHADER_PROFILE_GLSL330, 3);
    #undef PROFILE_SHADER_MODEL
    return -1;  // unknown profile?
}

// end of mojoshader.c ...
