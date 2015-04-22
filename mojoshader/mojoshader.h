/**
 * MojoShader; generate shader programs from bytecode of compiled
 *  Direct3D shaders.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#ifndef _INCL_MOJOSHADER_H_
#define _INCL_MOJOSHADER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* You can define this if you aren't generating mojoshader_version.h */
#ifndef MOJOSHADER_NO_VERSION_INCLUDE
#include "mojoshader_version.h"
#endif

#ifndef MOJOSHADER_VERSION
#define MOJOSHADER_VERSION -1
#endif

#ifndef MOJOSHADER_CHANGESET
#define MOJOSHADER_CHANGESET "???"
#endif

/*
 * For determining the version of MojoShader you are using:
 *    const int compiled_against = MOJOSHADER_VERSION;
 *    const int linked_against = MOJOSHADER_version();
 *
 * The version is a single integer that increments, not a major/minor value.
 */
int MOJOSHADER_version(void);

/*
 * These are enum values, but they also can be used in bitmasks, so we can
 *  test if an opcode is acceptable: if (op->shader_types & ourtype) {} ...
 */
typedef enum
{
    MOJOSHADER_TYPE_UNKNOWN  = 0,
    MOJOSHADER_TYPE_PIXEL    = (1 << 0),
    MOJOSHADER_TYPE_VERTEX   = (1 << 1),
    MOJOSHADER_TYPE_GEOMETRY = (1 << 2),  /* (not supported yet.) */
    MOJOSHADER_TYPE_ANY = 0xFFFFFFFF   /* used for bitmasks */
} MOJOSHADER_shaderType;

/*
 * Data types for vertex attribute streams.
 */
typedef enum
{
    MOJOSHADER_ATTRIBUTE_UNKNOWN = -1,  /* housekeeping; not returned. */
    MOJOSHADER_ATTRIBUTE_BYTE,
    MOJOSHADER_ATTRIBUTE_UBYTE,
    MOJOSHADER_ATTRIBUTE_SHORT,
    MOJOSHADER_ATTRIBUTE_USHORT,
    MOJOSHADER_ATTRIBUTE_INT,
    MOJOSHADER_ATTRIBUTE_UINT,
    MOJOSHADER_ATTRIBUTE_FLOAT,
    MOJOSHADER_ATTRIBUTE_DOUBLE,
    MOJOSHADER_ATTRIBUTE_HALF_FLOAT,  /* MAYBE available in your OpenGL! */
} MOJOSHADER_attributeType;

/*
 * Data types for uniforms. See MOJOSHADER_uniform for more information.
 */
typedef enum
{
    MOJOSHADER_UNIFORM_UNKNOWN = -1, /* housekeeping value; never returned. */
    MOJOSHADER_UNIFORM_FLOAT,
    MOJOSHADER_UNIFORM_INT,
    MOJOSHADER_UNIFORM_BOOL,
} MOJOSHADER_uniformType;

/*
 * These are the uniforms to be set for a shader. "Uniforms" are what Direct3D
 *  calls "Constants" ... IDirect3DDevice::SetVertexShaderConstantF() would
 *  need this data, for example. These integers are register indexes. So if
 *  index==6 and type==MOJOSHADER_UNIFORM_FLOAT, that means we'd expect a
 *  4-float vector to be specified for what would be register "c6" in D3D
 *  assembly language, before drawing with the shader.
 * (array_count) means this is an array of uniforms...this happens in some
 *  profiles when we see a relative address ("c0[a0.x]", not the usual "c0").
 *  In those cases, the shader was built to set some range of constant
 *  registers as an array. You should set this array with (array_count)
 *  elements from the constant register file, starting at (index) instead of
 *  just a single uniform. To be extra difficult, you'll need to fill in the
 *  correct values from the MOJOSHADER_constant data into the appropriate
 *  parts of the array, overriding the constant register file. Fun!
 * (constant) says whether this is a constant array; these need to be loaded
 *  once at creation time, from the constant list and not ever updated from
 *  the constant register file. This is a workaround for limitations in some
 *  profiles.
 * (name) is a profile-specific variable name; it may be NULL if it isn't
 *  applicable to the requested profile.
 */
typedef struct MOJOSHADER_uniform
{
    MOJOSHADER_uniformType type;
    int index;
    int array_count;
    int constant;
    const char *name;
} MOJOSHADER_uniform;

/*
 * These are the constants defined in a shader. These are data values
 *  hardcoded in a shader (with the DEF, DEFI, DEFB instructions), which
 *  override your Uniforms. This data is largely for informational purposes,
 *  since they are compiled in and can't be changed, like Uniforms can be.
 * These integers are register indexes. So if index==6 and
 *  type==MOJOSHADER_UNIFORM_FLOAT, that means we'd expect a 4-float vector
 *  to be specified for what would be register "c6" in D3D assembly language,
 *  before drawing with the shader.
 * (value) is the value of the constant, unioned by type.
 */
typedef struct MOJOSHADER_constant
{
    MOJOSHADER_uniformType type;
    int index;
    union {
        float f[4];  /* if type==MOJOSHADER_UNIFORM_FLOAT */
        int i[4];    /* if type==MOJOSHADER_UNIFORM_INT */
        int b;       /* if type==MOJOSHADER_UNIFORM_BOOL */
    } value;
} MOJOSHADER_constant;

/*
 * Data types for samplers. See MOJOSHADER_sampler for more information.
 */
typedef enum
{
    MOJOSHADER_SAMPLER_UNKNOWN = -1, /* housekeeping value; never returned. */
    MOJOSHADER_SAMPLER_2D,
    MOJOSHADER_SAMPLER_CUBE,
    MOJOSHADER_SAMPLER_VOLUME,
} MOJOSHADER_samplerType;

/*
 * These are the samplers to be set for a shader. ...
 *  IDirect3DDevice::SetTexture() would need this data, for example.
 * These integers are the sampler "stage". So if index==6 and
 *  type==MOJOSHADER_SAMPLER_2D, that means we'd expect a regular 2D texture
 *  to be specified for what would be register "s6" in D3D assembly language,
 *  before drawing with the shader.
 * (name) is a profile-specific variable name; it may be NULL if it isn't
 *  applicable to the requested profile.
 * (texbem) will be non-zero if a TEXBEM opcode references this sampler. This
 *  is only used in legacy shaders (ps_1_1 through ps_1_3), but it needs some
 *  special support to work, as we have to load a magic uniform behind the
 *  scenes to support it. Most code can ignore this field in general, and no
 *  one has to touch it unless they really know what they're doing.
 */
typedef struct MOJOSHADER_sampler
{
    MOJOSHADER_samplerType type;
    int index;
    const char *name;
    int texbem;
} MOJOSHADER_sampler;


/*
 * This struct is used if you have to force a sampler to a specific type.
 *  Generally, you can ignore this, but if you have, say, a ps_1_1
 *  shader, you might need to specify what the samplers are meant to be
 *  to get correct results, as Shader Model 1 samples textures according
 *  to what is bound to a sampler at the moment instead of what the shader
 *  is hardcoded to expect.
 */
typedef struct MOJOSHADER_samplerMap
{
    int index;
    MOJOSHADER_samplerType type;
} MOJOSHADER_samplerMap;

/*
 * Data types for attributes. See MOJOSHADER_attribute for more information.
 */
typedef enum
{
    MOJOSHADER_USAGE_UNKNOWN = -1,  /* housekeeping value; never returned. */
    MOJOSHADER_USAGE_POSITION,
    MOJOSHADER_USAGE_BLENDWEIGHT,
    MOJOSHADER_USAGE_BLENDINDICES,
    MOJOSHADER_USAGE_NORMAL,
    MOJOSHADER_USAGE_POINTSIZE,
    MOJOSHADER_USAGE_TEXCOORD,
    MOJOSHADER_USAGE_TANGENT,
    MOJOSHADER_USAGE_BINORMAL,
    MOJOSHADER_USAGE_TESSFACTOR,
    MOJOSHADER_USAGE_POSITIONT,
    MOJOSHADER_USAGE_COLOR,
    MOJOSHADER_USAGE_FOG,
    MOJOSHADER_USAGE_DEPTH,
    MOJOSHADER_USAGE_SAMPLE,
    MOJOSHADER_USAGE_TOTAL,  /* housekeeping value; never returned. */
} MOJOSHADER_usage;

/*
 * These are the attributes to be set for a shader. "Attributes" are what
 *  Direct3D calls "Vertex Declarations Usages" ...
 *  IDirect3DDevice::CreateVertexDeclaration() would need this data, for
 *  example. Each attribute is associated with an array of data that uses one
 *  element per-vertex. So if usage==MOJOSHADER_USAGE_COLOR and index==1, that
 *  means we'd expect a secondary color array to be bound to this shader
 *  before drawing.
 * (name) is a profile-specific variable name; it may be NULL if it isn't
 *  applicable to the requested profile.
 */
typedef struct MOJOSHADER_attribute
{
    MOJOSHADER_usage usage;
    int index;
    const char *name;
} MOJOSHADER_attribute;

/*
 * Use this if you want to specify newly-parsed code to swizzle incoming
 *  data. This can be useful if you know, at parse time, that a shader
 *  will be processing data on COLOR0 that should be RGBA, but you'll
 *  be passing it a vertex array full of ARGB instead.
 */
typedef struct MOJOSHADER_swizzle
{
    MOJOSHADER_usage usage;
    unsigned int index;
    unsigned char swizzles[4];  /* {0,1,2,3} == .xyzw, {2,2,2,2} == .zzzz */
} MOJOSHADER_swizzle;


/*
 * These are used with MOJOSHADER_error as special case positions.
 */
#define MOJOSHADER_POSITION_NONE (-3)
#define MOJOSHADER_POSITION_BEFORE (-2)
#define MOJOSHADER_POSITION_AFTER (-1)

typedef struct MOJOSHADER_error
{
    /*
     * Human-readable error, if there is one. Will be NULL if there was no
     *  error. The string will be UTF-8 encoded, and English only. Most of
     *  these shouldn't be shown to the end-user anyhow.
     */
    const char *error;

    /*
     * Filename where error happened. This can be NULL if the information
     *  isn't available.
     */
    const char *filename;

    /*
     * Position of error, if there is one. Will be MOJOSHADER_POSITION_NONE if
     *  there was no error, MOJOSHADER_POSITION_BEFORE if there was an error
     *  before processing started, and MOJOSHADER_POSITION_AFTER if there was
     *  an error during final processing. If >= 0, MOJOSHADER_parse() sets
     *  this to the byte offset (starting at zero) into the bytecode you
     *  supplied, and MOJOSHADER_assemble(), MOJOSHADER_parseAst(), and
     *  MOJOSHADER_compile() sets this to a a line number in the source code
     *  you supplied (starting at one).
     */
    int error_position;
} MOJOSHADER_error;


/*
 * Structure used to return data from parsing of a shader...
 */
/* !!! FIXME: most of these ints should be unsigned. */
typedef struct MOJOSHADER_parseData
{
    /*
     * The number of elements pointed to by (errors).
     */
    int error_count;

    /*
     * (error_count) elements of data that specify errors that were generated
     *  by parsing this shader.
     * This can be NULL if there were no errors or if (error_count) is zero.
     */
    MOJOSHADER_error *errors;

    /*
     * The name of the profile used to parse the shader. Will be NULL on error.
     */
    const char *profile;

    /*
     * Bytes of output from parsing. Most profiles produce a string of source
     *  code, but profiles that do binary output may not be text at all.
     *  Will be NULL on error.
     */
    const char *output;

    /*
     * Byte count for output, not counting any null terminator. Most profiles
     *  produce an ASCII string of source code (which will be null-terminated
     *  even though that null char isn't included in output_len), but profiles
     *  that do binary output may not be text at all. Will be 0 on error.
     */
    int output_len;

    /*
     * Count of Direct3D instruction slots used. This is meaningless in terms
     *  of the actual output, as the profile will probably grow or reduce
     *  the count (or for high-level languages, not have that information at
     *  all). Also, as with Microsoft's own assembler, this value is just a
     *  rough estimate, as unpredicable real-world factors make the actual
     *  value vary at least a little from this count. Still, it can give you
     *  a rough idea of the size of your shader. Will be zero on error.
     */
    int instruction_count;

    /*
     * The type of shader we parsed. Will be MOJOSHADER_TYPE_UNKNOWN on error.
     */
    MOJOSHADER_shaderType shader_type;

    /*
     * The shader's major version. If this was a "vs_3_0", this would be 3.
     */
    int major_ver;

    /*
     * The shader's minor version. If this was a "ps_1_4", this would be 4.
     *  Two notes: for "vs_2_x", this is 1, and for "vs_3_sw", this is 255.
     */
    int minor_ver;

    /*
     * The number of elements pointed to by (uniforms).
     */
    int uniform_count;

    /*
     * (uniform_count) elements of data that specify Uniforms to be set for
     *  this shader. See discussion on MOJOSHADER_uniform for details.
     * This can be NULL on error or if (uniform_count) is zero.
     */
    MOJOSHADER_uniform *uniforms;

    /*
     * The number of elements pointed to by (constants).
     */
    int constant_count;

    /*
     * (constant_count) elements of data that specify constants used in
     *  this shader. See discussion on MOJOSHADER_constant for details.
     * This can be NULL on error or if (constant_count) is zero.
     *  This is largely informational: constants are hardcoded into a shader.
     *  The constants that you can set like parameters are in the "uniforms"
     *  list.
     */
    MOJOSHADER_constant *constants;

    /*
     * The number of elements pointed to by (samplers).
     */
    int sampler_count;

    /*
     * (sampler_count) elements of data that specify Samplers to be set for
     *  this shader. See discussion on MOJOSHADER_sampler for details.
     * This can be NULL on error or if (sampler_count) is zero.
     */
    MOJOSHADER_sampler *samplers;

    /* !!! FIXME: this should probably be "input" and not "attribute" */
    /*
     * The number of elements pointed to by (attributes).
     */
    int attribute_count;

    /* !!! FIXME: this should probably be "input" and not "attribute" */
    /*
     * (attribute_count) elements of data that specify Attributes to be set
     *  for this shader. See discussion on MOJOSHADER_attribute for details.
     * This can be NULL on error or if (attribute_count) is zero.
     */
    MOJOSHADER_attribute *attributes;

    /*
     * The number of elements pointed to by (outputs).
     */
    int output_count;

    /*
     * (output_count) elements of data that specify outputs this shader
     *  writes to. See discussion on MOJOSHADER_attribute for details.
     * This can be NULL on error or if (output_count) is zero.
     */
    MOJOSHADER_attribute *outputs;

    /*
     * The number of elements pointed to by (swizzles).
     */
    int swizzle_count;

    /* !!! FIXME: this should probably be "input" and not "attribute" */
    /*
     * (swizzle_count) elements of data that specify swizzles the shader will
     *  apply to incoming attributes. This is a copy of what was passed to
     *  MOJOSHADER_parseData().
     * This can be NULL on error or if (swizzle_count) is zero.
     */
    MOJOSHADER_swizzle *swizzles;
} MOJOSHADER_parseData;


/*
 * Profile string for Direct3D assembly language output.
 */
#define MOJOSHADER_PROFILE_D3D "d3d"

/*
 * Profile string for passthrough of the original bytecode, unchanged.
 */
#define MOJOSHADER_PROFILE_BYTECODE "bytecode"

/*
 * Profile string for GLSL: OpenGL high-level shader language output.
 */
#define MOJOSHADER_PROFILE_GLSL "glsl"

/*
 * Profile string for GLSL 1.20: minor improvements to base GLSL spec.
 */
#define MOJOSHADER_PROFILE_GLSL120 "glsl120"

/*
 * Profile string for OpenGL ARB 1.0 shaders: GL_ARB_(vertex|fragment)_program.
 */
#define MOJOSHADER_PROFILE_ARB1 "arb1"

/*
 * Profile string for OpenGL ARB 1.0 shaders with Nvidia 2.0 extensions:
 *  GL_NV_vertex_program2_option and GL_NV_fragment_program2
 */
#define MOJOSHADER_PROFILE_NV2 "nv2"

/*
 * Profile string for OpenGL ARB 1.0 shaders with Nvidia 3.0 extensions:
 *  GL_NV_vertex_program3 and GL_NV_fragment_program2
 */
#define MOJOSHADER_PROFILE_NV3 "nv3"

/*
 * Profile string for OpenGL ARB 1.0 shaders with Nvidia 4.0 extensions:
 *  GL_NV_gpu_program4
 */
#define MOJOSHADER_PROFILE_NV4 "nv4"

/*
 * Determine the highest supported Shader Model for a profile.
 */
int MOJOSHADER_maxShaderModel(const char *profile);


/*
 * Parse a compiled Direct3D shader's bytecode.
 *
 * This is your primary entry point into MojoShader. You need to pass it
 *  a compiled D3D shader and tell it which "profile" you want to use to
 *  convert it into useful data.
 *
 * The available profiles are the set of MOJOSHADER_PROFILE_* defines.
 *  Note that MojoShader may be built without support for all listed
 *  profiles (in which case using one here will return with an error).
 *
 * As parsing requires some memory to be allocated, you may provide a custom
 *  allocator to this function, which will be used to allocate/free memory.
 *  They function just like malloc() and free(). We do not use realloc().
 *  If you don't care, pass NULL in for the allocator functions. If your
 *  allocator needs instance-specific data, you may supply it with the
 *  (d) parameter. This pointer is passed as-is to your (m) and (f) functions.
 *
 * This function returns a MOJOSHADER_parseData.
 *
 * This function will never return NULL, even if the system is completely
 *  out of memory upon entry (in which case, this function returns a static
 *  MOJOSHADER_parseData object, which is still safe to pass to
 *  MOJOSHADER_freeParseData()).
 *
 * You can tell the generated program to swizzle certain inputs. If you know
 *  that COLOR0 should be RGBA but you're passing in ARGB, you can specify
 *  a swizzle of { MOJOSHADER_USAGE_COLOR, 0, {1,2,3,0} } to (swiz). If the
 *  input register in the code would produce reg.ywzx, that swizzle would
 *  change it to reg.wzxy ... (swiz) can be NULL.
 *
 * You can force the shader to expect samplers of certain types. Generally
 *  you don't need this, as Shader Model 2 and later always specify what they
 *  expect samplers to be (2D, cubemap, etc). Shader Model 1, however, just
 *  uses whatever is bound to a given sampler at draw time, but this doesn't
 *  work in OpenGL, etc. In these cases, MojoShader will default to
 *  2D texture sampling (or cubemap sampling, in cases where it makes sense,
 *  like the TEXM3X3TEX opcode), which works 75% of the time, but if you
 *  really needed something else, you'll need to specify it here. This can
 *  also be used, at your own risk, to override DCL opcodes in shaders: if
 *  the shader explicit says 2D, but you want Cubemap, for example, you can
 *  use this to override. If you aren't sure about any of this stuff, you can
 *  (and should) almost certainly ignore it: (smap) can be NULL.
 *
 * (bufsize) is the size in bytes of (tokenbuf). If (bufsize) is zero,
 *  MojoShader will attempt to figure out the size of the buffer, but you
 *  risk a buffer overflow if you have corrupt data, etc. Supply the value
 *  if you can.
 *
 * This function is thread safe, so long as (m) and (f) are too, and that
 *  (tokenbuf) remains intact for the duration of the call. This allows you
 *  to parse several shaders on separate CPU cores at the same time.
 */
const MOJOSHADER_parseData *MOJOSHADER_parse(const char *profile,
                                             const unsigned char *tokenbuf,
                                             const unsigned int bufsize,
                                             const MOJOSHADER_swizzle *swiz,
                                             const unsigned int swizcount,
                                             const MOJOSHADER_samplerMap *smap,
                                             const unsigned int smapcount);

/*
 * Call this to dispose of parsing results when you are done with them.
 *  This will call the MOJOSHADER_free function you provided to
 *  MOJOSHADER_parse multiple times, if you provided one.
 *  Passing a NULL here is a safe no-op.
 *
 * This function is thread safe, so long as any allocator you passed into
 *  MOJOSHADER_parse() is, too.
 */
void MOJOSHADER_freeParseData(const MOJOSHADER_parseData *data);

#ifdef __cplusplus
}
#endif

#endif  /* include-once blocker. */

/* end of mojoshader.h ... */

