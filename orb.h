#ifndef COM_ARCANEINGENUITY_ORB_H
#define COM_ARCANEINGENUITY_ORB_H

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "log/log.h"
#include "linmath.h"
#include "klib/kvec.h"

//KHASH...
#include "klib/khash.h"
// setup khash for key/value types
// shorthand way to get the key from hashtable or defVal if not found
#define kh_get_val(kname, hash, key, defVal) ({k=kh_get(kname, hash, key);(k!=kh_end(hash)?kh_val(hash,k):defVal);})

// shorthand way to set value in hash with single line command.  Returns value
// returns 0=replaced existing item, 1=bucket empty (new key), 2-adding element previously deleted
#define kh_set(kname, hash, key, val) ({int ret; k = kh_put(kname, hash,key,&ret); kh_value(hash,k) = val; ret;})

static const int StrInt = 33;
static const int IntInt = 34;
static const int IntFloat = 35;


//KHASH_DECLARE
KHASH_DECLARE(StrInt, kh_cstr_t, int)
KHASH_DECLARE(IntInt, khint32_t, int)
KHASH_DECLARE(IntFloat, khint32_t, float)
#ifndef KH_DECL_STRPTR
#define KH_DECL_STRPTR
static const int StrPtr = 36;
KHASH_DECLARE(StrPtr, kh_cstr_t, uintptr_t)
#endif//KH_DECL_STRPTR

#ifdef _WIN32
	//define something for Windows (32-bit and 64-bit, this part is common)
	#define DESKTOP 1
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#ifdef _WIN64
		//define something for Windows (64-bit only)
	#endif
#elif __APPLE__
	#include "TargetConditionals.h"
	#if TARGET_OS_IPHONE
		#define MOBILE 1
		#if TARGET_IPHONE_SIMULATOR
		#else //actual device
		#endif
		// iOS Simulator
	#elif TARGET_OS_MAC
		// Mac OS
		#define DESKTOP 1
	#else
		// Unsupported platform
	#endif
#elif __ANDROID__
	#include <android/sensor.h>
	//#include <android/log.h>
	#include "android_native_app_glue.h"

	#include <android/api-level.h>
	
	#define MAIN void android_main(struct android_app* app)
	
	#define MOBILE 1 //really? you don't know that this implies mobile. maybe we really should avoid these categories.
	#include <jni.h>
	#include <errno.h>
	#include <math.h>

	#include <dlfcn.h> //for dynamic linking of extensions	
	
	#include <EGL/egl.h>
	
	#define GL_GLEXT_PROTOTYPES 1
	//#include <GLES/glext.h>
	#include <GLES2/gl2ext.h>
	//#include <GLES/gl.h> //for e.g. GL_PERSPECTIVE_CORRECTION_HINT
	#include <GLES2/gl2.h>

	//#define glGenVertexArrays GenVertexArraysOES
	//#define glBindVertexArray BindVertexArrayOES
	

	//TODO API level checks via __ANDROID_API__
	// 1.0 	1
	// 1.1 	2
	// 1.5 	3
	// 1.6 	4
	// 2.0 	5
	// 2.0.1 	6
	// 2.1 	7
	// 2.2 	8
	// 2.3 	9
	// 2.3.3 	10
	// 3.0 	11
	
	//"polyfills"
	#define log2(value) (log(value) / log(2))
	
	//OpenGL ES Extensions
	PFNGLGENVERTEXARRAYSOESPROC glGenVertexArrays;
	PFNGLBINDVERTEXARRAYOESPROC glBindVertexArray;
	PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArrays;
	PFNGLISVERTEXARRAYOESPROC glIsVertexArray;

#elif __linux
	#define DESKTOP 1 //not certain about this, but should be true...?
	// linux
#elif __unix // all unices not caught above
	// Unix
#elif __posix
	// POSIX
#endif

#ifdef DESKTOP
	#define MAIN int main(int argc, char *argv[])
	
	#define GLEW_STATIC
	#include "glew/glew.h"
	#include "glfw/glfw3.h"

	//#include "stb_image_aug.h"
	#define STB_IMAGE_STATIC 1
	#define STB_IMAGE_IMPLEMENTATION
	#include "stb_image.h"
	#undef STB_IMAGE_IMPLEMENTATION
	
#endif//DESKTOP

#define GL_BGR 0x80E0
#define GL_BGRA 0x80E1
#define GL_RG 0x8227
#define GL_RED 0x1903
//#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
//#define GL_MAX_COLOR_ATTACHMENTS 0x8CDF

//FIXED CONSTANTS

#define XX 0
#define YY 1
#define ZZ 2
#define PRESSURE 2 //touch

//multitouch channels
#define XX0 0
#define YY0 1
#define PRESSURE0 2
#define XX1 3
#define YY1 4
#define PRESSURE1 5
#define XX2 6
#define YY2 7
#define PRESSURE2 8
#define XX3 9
#define YY3 10
#define PRESSURE3 11

//channel state/delta index
#define CURRENT 0
#define PREVIOUS 1

#define POSITION_COMPONENTS 3
#define TEXCOORD_COMPONENTS 2
#define VERTICES_PER_TRIANGLE 3
#define BITS_IN_BYTE 8

#ifndef MESH_VERTICES_SHORT_MAX
#define MESH_VERTICES_SHORT_MAX 65536
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//USER CONSTANTS (define before inclusion of this header to override)

#ifndef	HH_SHADERS_MAX
#define HH_SHADERS_MAX 64
#endif

#ifndef	HH_PROGRAMS_MAX
#define HH_PROGRAMS_MAX 64
#endif

#ifndef	HH_RENDERTEXTURES_MAX
#define HH_RENDERTEXTURES_MAX 8
#endif

#ifndef	HH_TEXTURES_MAX
#define HH_TEXTURES_MAX 128
#endif

#ifndef	HH_TEXTURE_PARAMETERS_MAX
#define HH_TEXTURE_PARAMETERS_MAX 16
#endif

#ifndef	HH_MATERIALS_MAX
#define HH_MATERIALS_MAX 64
#endif

#ifndef	HH_RENDERABLES_MAX
#define HH_RENDERABLES_MAX 16384
#endif

#ifndef	HH_TEXTURES_RENDERABLE_MAX
#define HH_TEXTURES_RENDERABLE_MAX 16
#endif

#ifndef	HH_MESHES_MAX
#define HH_MESHES_MAX 256
#endif

#ifndef	HH_ATTRIBUTES_MAX
#define HH_ATTRIBUTES_MAX 8
#endif

typedef struct Window
{
	#ifdef DESKTOP
	GLFWwindow * window;
	#else //DEV, ANDROID
		#ifdef __ANDROID__
	ANativeWindow * window;
		#endif //__ANDROID__
	#endif //DESKTOP
} Window;
//TODO set up other functions to wrap the different window types' provision of information e.g. resolution

typedef struct BMFontInfo
{
	char * face;
	unsigned short size;
	bool bold;
	bool italic;
	char * charset;
	bool unicode;
	unsigned short stretchH;
	bool smooth;
	unsigned char aa;
	unsigned short paddingUp;
	unsigned short paddingRight;
	unsigned short paddingDown;
	unsigned short paddingLeft;
	unsigned short spacingH;
	unsigned short spacingV;
	unsigned short outline;
	
} BMFontInfo;

typedef struct BMFontCommon
{
	unsigned short lineHeight;
	unsigned short base;
	unsigned short scaleW;
	unsigned short scaleH;
	unsigned char pages;
	bool packed;
	unsigned char alphaChnl;
	unsigned char redChnl;
	unsigned char greenChnl;
	unsigned char blueChnl;
} BMFontCommon;

typedef struct BMFontCharacter
{
	unsigned short id; //used as index into bmFont.characters array.
	unsigned short x;
	unsigned short y;
	unsigned short width;
	unsigned short height;
	unsigned short xoffset;
	unsigned short yoffset;
	unsigned short xadvance;
	unsigned char page;
	unsigned char chnl;
} BMFontCharacter;

typedef struct BMFontKerning
{
	unsigned short first; //id
	unsigned short second; //id
	unsigned short amount;
} BMFontKerning;

typedef struct BMFontPage
{
	unsigned char id;
	const char * file;
} BMFontPage;

typedef struct BMFont
{
	BMFontInfo info;
	BMFontCommon common;
	
	BMFontPage pages[16];
	BMFontCharacter characters[256]; //use BMFontCharacter.id as array index.
	BMFontKerning kerningsByFirst[256]; //index by first id, matches second array
	BMFontKerning kerningsBySecond[256]; //index by second id, matches first array
} BMFont;

//instance of the abstract, non-mesh-specific concept of a vertex attribute.
//there could be many ShaderAttributes which would work with a given Mesh Attribute, and vice versa, so no point linking these directly.
//the shader
typedef struct ShaderAttribute 
{
	char name[8]; //or instead, as key or numeric constant index?
	char type[8]; //vec2, mat4 etc.
	char typeCompound[8]; //vec, mat etc.
	char typeNumeric[8]; //float, int etc.
	int components; //2, 3, 16 etc.void * 

	//qualifiers
	bool polarity; //in/out/inout
	GLuint location; //layout, e.g. layout(location = 3)
	GLuint index; //layout, e.g. layout(location = 3, index = 1) UNUSED
} ShaderAttribute;

//Mesh Attribute is just raw data. It is up to ShaderPrograms (ShaderAttributes) to know what to do with it.
typedef struct Attribute 
{
	//count is in the Mesh holding the Attribute.
	void * vertex; //ShaderAttribute will know how to read it, depending on which attribute it is.
	GLuint id; //buffer id
    GLuint index; //attribute location
	GLint components; //"size" in glVertexAttribPointer 
	GLenum type; //as glVertexAttribPointer
	GLboolean normalized; //as glVertexAttribPointer
	GLsizei stride; //as glVertexAttribPointer
	const GLvoid * pointer;
	GLsizeiptr vertexBytes; //"size" in glBufferData
	GLenum  usage; //as glBufferData

} Attribute;

typedef void (*glUniformVectorFunction)(GLint location, GLsizei count, const GLint * value);
extern glUniformVectorFunction glUniformVectorFunctions[4][2];

typedef void (*glUniformMatrixFunction)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
extern glUniformMatrixFunction glUniformMatrixFunctions[4][4][2];

enum UniformTypeNumeric
{
	UniformFloat,
	UniformInt
	//UniformUInt, //not in ES 2.0
};

enum UniformType
{
	UniformVector,
	UniformMatrix,
	UniformTexture
};

//TODO lists of Uniforms should stand alone, and be able to run whenever calling code sees fit,
//whether that be once in a blue moon, once every frame for a group of renderables, or once
//every frame for each distinct renderable (based on its state).
typedef struct Uniform
{
	char * name;
	enum UniformType type;
	enum UniformTypeNumeric typeNumeric; //float, int or whatever - applies only if type != texture
	GLsizei componentsMajor; //count major (for matrices and vectors)
	GLsizei componentsMinor; //count minor (for matrices only)
	GLsizei elements; //count of array size
	
	void * valuesPtr; //"values" since many glUniform* are non-scalar
	GLboolean matrixTranspose; //for glUniformMatrix calls - applies only if type == matrix
	//GLint location; //cached from glgetUniformLocation
} Uniform;

typedef struct Texture
{
	char name[64]; //or as key?
    
    /** The byte buffer representing the decoded image data. */
	uint8_t * data;
    
    /** Texture width. */
    GLsizei width;
    
    /** Texture height. */
    GLsizei height;
	
	/** Texture components / channels. */
	GLsizei components;
	
	 /** OpenGL's texture ID as gotten from glGenTextures(). */
    GLuint id;
	
    /** The texture unit ordinal (NOT internal OpenGL code e.g. GL_TEXTURE0) currently in use for this texture. */
    GLuint unit;
	
	/** Did texels get modified / need upload? */
	bool changed;
	
	//atom="type", arrangedExternal="format", arrangedInternal="internalFormat" all in glTexImage2D()
	GLenum atomTypeExternal;
	GLenum arrangedExternal; //32 bit unsigned = same as GLuint
	GLint  arrangedInternal; //32 bit unsigned = same as GLenum
	
	GLenum dimensions; //glTexImage2D & glBindTexture "target"
	
	//TODO rename to ...byPname (as https://www.khronos.org/opengles/sdk/docs/man/xhtml/glTexParameter.xml)
	khash_t(IntFloat) * floatParametersByName;
	khash_t(IntInt) * intParametersByName;

} Texture;

typedef struct Face
{
	GLushort index[VERTICES_PER_TRIANGLE]; //into the owning Mesh's arrays: position, texcoord etc.
	GLfloat normal[VERTICES_PER_TRIANGLE];
} Face;

typedef struct Mesh
{
	GLuint topology; //GL_TRIANGLES or whatever (see also Program)

	//TODO bounds only require a single attribute. Maybe we should make this a pointer to an array? But Attribute is really small, points to data elsewhere.
	Attribute attribute[HH_ATTRIBUTES_MAX]; //also contains positions needed for faces
	uint8_t attributeCount;
	
	GLushort * index; //what we send to GPU.... TODO generate from face
	
	GLsizei indexCount;
	GLsizei vertexCount; //to be applied to each attribute
	GLsizei faceCount;
	
	GLuint vao;
	GLuint sampler;
	
	Face * face;
	
	/** Did vertices get modified / need upload? */
	bool changed;
} Mesh;

//Renderable is a unique combination of some Mesh (vertex data) and some Material (shader + uniforms).
//It is optional for grouping these related render-time aspects together. It is best used where efficiency is not of the essence, e.g. UI (where efficiency is important, rather use multiple arrays with same index into each). 
//Materials are either treated explicitly or simply as the input interface + matching renderable information for a given ProgramPath
typedef struct Renderable
{
	//char * id;
	Uniform * uniforms;
	uint8_t uniformsCount;
	
	khash_t(StrPtr) * uniformPtrsByName;
	
	//occasional upload i.e. not performance-critical, so these objects can be pointers to structs.
	Mesh * mesh;
	
	//TODO Material (with Texture)
	//Material materials[];
	//A Material consists of:
	//-a ShaderPath/Pipe, which consists of multiple shader Programs running in sequence
	//-the parameters needed to populate that pipe at each stage
	
	void * userData;
} Renderable;

//TODO you will need to memcpy out the range of units used in a single instances draw call
//TODO make it use a particular Mesh, Material (with Texture) etc.
typedef struct RenderableSet
{
	//Renderable * renderable;
	Mesh * mesh;
	Texture * textures[HH_TEXTURES_RENDERABLE_MAX];

	//we always need a contiguous buffer. So to keep things fast, client app *should*
	//memcpy out a contiguous block from a super-array: this means pre-sorting.
	GLsizei count;
	GLuint buffer;
	const GLvoid * data; //for now, model matrices.
} RenderableSet;

//both ShaderComponents will have these (duplicated) -- however we will check in against out and type against type
typedef struct ShaderVariable
{
	char * type; //for now, just use a string and check equality
	unsigned char inoutQualifier;
	unsigned char layoutQualifier;
} ShaderVariable;

typedef struct ShaderComponent
{
	ShaderVariable * inputs;
	ShaderVariable * outputs;//one shader's output is another's input - this is how they're linked
} ShaderComponent;

typedef struct Shader
{
	GLuint id;
	GLenum type; //(NOPE!) SHADER_VERTEX, _FRAGMENT, _GEOMETRY, _TESSELATION_CONTROL, _TESSELATION_EVALULATION
	const char * source;
	
	//inputs:
	khash_t(StrInt) attributesByName;
	khash_t(StrInt) uniformPtrsByName;
	
	//shader version
} Shader;

//NB this is only needed once we get to the stage where we determine if we can compact the pipeline more, since it will be built of functions
typedef struct ShaderFunction
{
	//arguments
	
	//return value

} ShaderFunction;

/*
//joins one shader's input to another's output... if appropriate
typedef struct ShaderPipe
{
	ShaderVariable from;
	ShaderVariable to;
	unsigned char status; //fixed / broken / warning
} ShaderPipe;
*/

typedef struct Program
{
	//contains information on a single shader pass
	//batching & instancing require *exactly* the same state per instance - this includes uniforms & attributes
	char * name;
	//char * version (copied from shaders' version, or later used to dictate this)
	
	GLuint id;
	GLuint topology; //GL_TRIANGLES or whatever (see also Mesh)
	
	Shader vertex;
	Shader geometry;
	Shader fragment;
	
	//const char * attributeLocations[16]; //TODO see Program_construct() for similar quandary WRT size.
	//List attributeLocationsList;
	//inputs: as vertex shader's inputs?

	//output (frame or render) buffer
} Program;

typedef struct ProgramConfig
{
	char * programName;
	char * vertexName;
	char * fragmentName;
	
	char * attributeLocations[8];
	uint8_t attributeLocationsCount;
	
} ProgramConfig;


typedef struct RenderPath
{
	//Tree * rootProgram; //we work backwards from the root, fulfilling each set of input dependencies till we reach the root and can conclude.
	
	//for multipass rendering.
	//a tree of programs, each with a defined input and output interface, that lead to a single program producing a final result
	//interfaces must be fulfilled on input end.
	//as for output end, we must put that out to some render target.

	//some inputs come from previous Programs; others from elsewhere. We need to figure out how that should be done.

} RenderPath;

typedef struct Material
{
	Program program;
	Texture * texture;
	khash_t(StrInt) uniforms; //for referencing uniforms by name
	khash_t(StrInt) attributes; //ref attribs by name
	
	//root of the graph from which we determine the assembled code for each class of shader.
	char * vertexShader;
	char * fragmentShader;
	char * geometryShader;

} Material;

typedef struct Color
{
	float r;
	float g;
	float b;
	float a;
} Color;

//unlike e.g. GLFW, these are ordered zero-based, compact so they may be used as indices into an packed array
typedef enum Key
{
	ORB_KEY_UNKNOWN,
	ORB_KEY_0,
	ORB_KEY_1,
	ORB_KEY_2,
	ORB_KEY_3,
	ORB_KEY_4,
	ORB_KEY_5,
	ORB_KEY_6,
	ORB_KEY_7,
	ORB_KEY_8,
	ORB_KEY_9,
	
	ORB_KEY_KP_0,
	ORB_KEY_KP_1,
	ORB_KEY_KP_2,
	ORB_KEY_KP_3,
	ORB_KEY_KP_4,
	ORB_KEY_KP_5,
	ORB_KEY_KP_6,
	ORB_KEY_KP_7,
	ORB_KEY_KP_8,
	ORB_KEY_KP_9,
	
	ORB_KEY_A,
	ORB_KEY_B,
	ORB_KEY_C,
	ORB_KEY_D,
	ORB_KEY_E,
	ORB_KEY_F,
	ORB_KEY_G,
	ORB_KEY_H,
	ORB_KEY_I,
	ORB_KEY_J,
	ORB_KEY_K,
	ORB_KEY_L,
	ORB_KEY_M,
	ORB_KEY_N,
	ORB_KEY_O,
	ORB_KEY_P,
	ORB_KEY_Q,
	ORB_KEY_R,
	ORB_KEY_S,
	ORB_KEY_T,
	ORB_KEY_U,
	ORB_KEY_V,
	ORB_KEY_W,
	ORB_KEY_X,
	ORB_KEY_Y,
	ORB_KEY_Z,
	
	ORB_KEY_F1,
	ORB_KEY_F2,
	ORB_KEY_F3,
	ORB_KEY_F4,
	ORB_KEY_F5,
	ORB_KEY_F6,
	ORB_KEY_F7,
	ORB_KEY_F8,
	ORB_KEY_F9,
	ORB_KEY_F10,
	ORB_KEY_F11,
	ORB_KEY_F12,
	ORB_KEY_F13,
	ORB_KEY_F14,
	ORB_KEY_F15,
	ORB_KEY_F16,
	ORB_KEY_F17,
	ORB_KEY_F18,
	ORB_KEY_F19,
	ORB_KEY_F20,
	ORB_KEY_F21,
	ORB_KEY_F22,
	ORB_KEY_F23,
	ORB_KEY_F24,
	ORB_KEY_F25,
	
	ORB_KEY_RIGHT,
	ORB_KEY_LEFT,
	ORB_KEY_DOWN,
	ORB_KEY_UP,
	
	ORB_KEY_LEFT_SHIFT,
	ORB_KEY_LEFT_CONTROL,
	ORB_KEY_LEFT_ALT,
	ORB_KEY_LEFT_SUPER,
	ORB_KEY_RIGHT_SHIFT,
	ORB_KEY_RIGHT_CONTROL,
	ORB_KEY_RIGHT_ALT,
	ORB_KEY_RIGHT_SUPER,
	
	ORB_KEY_SPACE,
	ORB_KEY_APOSTROPHE,
	ORB_KEY_COMMA,
	ORB_KEY_MINUS,
	ORB_KEY_PERIOD,
	ORB_KEY_SLASH,
	ORB_KEY_SEMICOLON,
	ORB_KEY_EQUAL,
	ORB_KEY_BACKSLASH,
	ORB_KEY_LEFT_BRACKET,
	ORB_KEY_RIGHT_BRACKET,
	ORB_KEY_GRAVE_ACCENT,
	ORB_KEY_WORLD_1, //non-US #1
	ORB_KEY_WORLD_2, //non-US #2
	ORB_KEY_ESCAPE,
	ORB_KEY_ENTER,
	ORB_KEY_TAB,
	ORB_KEY_BACKSPACE,
	ORB_KEY_INSERT,
	ORB_KEY_DELETE,
	ORB_KEY_PAGE_UP,
	ORB_KEY_PAGE_DOWN,
	ORB_KEY_HOME,
	ORB_KEY_END,
	ORB_KEY_CAPS_LOCK,
	ORB_KEY_SCROLL_LOCK,
	ORB_KEY_NUM_LOCK,
	ORB_KEY_PRINT_SCREEN,
	ORB_KEY_PAUSE,
	ORB_KEY_KP_DECIMAL,
	ORB_KEY_KP_DIVIDE,
	ORB_KEY_KP_MULTIPLY,
	ORB_KEY_KP_SUBTRACT,
	ORB_KEY_KP_ADD,
	ORB_KEY_KP_ENTER,
	ORB_KEY_KP_EQUAL,
	ORB_KEY_MENU,
	
	ORB_KEYS_COUNT
} Key;

typedef enum InputBasis
{
	STATE, //default
	DELTA
} InputBasis;

typedef struct DeviceChannel
{
	//2 states & 2 deltas is enough to calculate the most recent other (state for delta or delta for state)
	float state[2];
	float delta[2];
	InputBasis basis;
	//bool blocked;
	bool active; //framework-private, particularly useful for multi-touch or devices that have disconnected but are waiting on clean-up by OS etc.
} DeviceChannel;

typedef struct Device
{
	DeviceChannel channels[ORB_KEYS_COUNT];
	//void * other; //special reference to other information, e.g. an array of fingers for a touch device.
	//TODO Finger fingers[]; //or rather, a pointer to an array elsewhere, if touchscreen device.
	uint64_t channelsActiveMask;
} Device;
/*
typedef struct Finger
{
	size_t index; //id?
	DeviceChannel channels[3]; //X, Y, pressure
} Finger;

//use as alternative to Device
typedef struct TouchDevice
{
	Finger fingers[10];
	size_t fingerCount; //fingers in actual use at this moment
}
*/
typedef float (*InputFunction) ();
typedef void  (*ResponseFunction) (void * model, float value, float valueLast);

//abstracts raw channel inputs into game logic inputs, provides a response for same, and stores recent values
typedef struct Input
{
	//InputFunction inputPos;
	//InputFunction inputNeg;
	DeviceChannel * channelPos;
	DeviceChannel * channelNeg;
	ResponseFunction response;
	InputBasis basis;
	//TODO optional custom function for combining raw channel values?
		
	float state[2];
	float delta[2];
} Input;
const struct Input inputEmpty;

typedef kvec_t(Input) InputList;

void Input_executeList(InputList * list, void * model, bool debug);
void Input_add(InputList * list, Input input);
typedef struct Capabilities
{
	bool vao;
} Capabilities;

typedef struct Engine
{
	Capabilities capabilities;
	bool debugDesktopNoVAO;
	
	int inputEventCount;
	
	khash_t(StrPtr) * programsByName;
	khash_t(StrPtr) * shadersByName;
	khash_t(StrPtr) * texturesByName;
	khash_t(StrPtr) * materialsByName;
	khash_t(StrPtr) * meshesByName;
	khash_t(StrPtr) * renderPathsByName;
	khash_t(StrPtr) * renderTexturesByName;
	
	//until we create merged map and list that contain not only pointers to arrays but also the arrays themselves... use these as backing arrays
	Shader   	shaders[HH_SHADERS_MAX];
	Texture * 	textures[HH_TEXTURES_MAX];
	Texture * 	renderTextures[HH_RENDERTEXTURES_MAX];
	Material 	materials[HH_MATERIALS_MAX];
	Program  	programs[HH_PROGRAMS_MAX];
	Mesh *		meshes[HH_MESHES_MAX];
	
	uint64_t shaderKeys[HH_SHADERS_MAX];
	uint64_t textureKeys[HH_TEXTURES_MAX];
	uint64_t rendertextureKeys[HH_RENDERTEXTURES_MAX];
	uint64_t materialKeys[HH_MATERIALS_MAX];
	uint64_t programKeys[HH_PROGRAMS_MAX];
	uint64_t meshKeys[HH_MESHES_MAX];
	//TODO - these lists but not generic / pointer - int lists! then remove same fields from Renderables.
	//List changedMesh; //indices of Renderables that changed vertex data
	//List changedMaterials; //indices of Renderables that changed uniforms
	
	//map is best - if devices connect in different orders at runtime, we are not relying on compile-time indices into an array
	//DeviceHub deviceHub;
	//Device array[2];
	khash_t(StrPtr) * devicesByName;
	
	Program * program; //the current shader program TODO should be an index into array? then use a getter to access. 
	
	//Transforms may not all have associated renderables...
	
	//TODO incorporate as separate RTT renderables array?
	Renderable fullscreenQuad;
	mat4x4 fullscreenQuadMatrix;

	#ifdef __ANDROID__	
	struct android_app* app;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	EGLConfig config;
	EGLint * attribsList;
	
	bool touches[10]; //DEV
	bool paused;
	#endif//__ANDROID__
	
	int32_t width;
	int32_t height;
	int32_t touchX;
	int32_t touchY;
	
	void (*userInitialiseFunc)();
	bool   userInitialised;
	
	void (*userUpdateFunc)(void *);
	void * userUpdateArg;
	
	void (*userSuspendFunc)(void *);
	void * userSuspendArg;
	
	float deltaSec;
} Engine;

typedef void (*IndexedRenderableFunction)(struct Renderable * renderable, uint16_t i, void * model);


/*
typedef struct IndexedRenderableManager
{
	Renderable * renderables; //no count needed - correct indices are user's responsibility
	
} IndexedRenderableManager;
*/

typedef kvec_t(uint16_t) IndexList;

/// creates new Renderable%s and adds them to this View%'s list for rendering
void IndexedRenderableManager_create(
	Renderable * const renderables,
	IndexList * const indexRenderListPtr,
	IndexList * const indexListPtr,
	IndexedRenderableFunction fnc,
	void * model);

/// updates a Renderable that has (at any point previously) been created
void IndexedRenderableManager_update(
	Renderable * const renderables,
	IndexList * const indexRenderListPtr, //actually unused here, but keeps the arg lists uniform between create/update/render
	IndexList * const indexListPtr,
	IndexedRenderableFunction fnc,
	void * model);

/// renders a group of Renderables from an index list
void IndexedRenderableManager_render(
	Renderable * const renderables,
	IndexList * indexRenderListPtr,
	Engine * enginePtr);

void Mesh_calculateNormals(Mesh * this);

void Attribute_submitData(Attribute * attribute, Engine * engine);
void Attribute_prepare(Attribute * attribute);
void Attribute_tryPrepare(Attribute * attribute, Engine * engine);

void UniformGroup_update(khash_t(StrPtr) * uniformPtrsByName, Program * program);

Texture * Texture_create();
void Texture_loadData(Texture * texture, const char * filename);
void Texture_createData(Texture * texture);
void Texture_clearData(Texture * texture, bool alphaFull);
//Texture * Texture_loadFromMemory(const char * filename);

GLenum Texture_getTextureUnitConstant(Texture * this);
void Texture_fresh(Texture * this);
void Texture_refresh(Texture * this);
void Texture_prepare(Texture * this, Program * program);
void Texture_setTexelFormats(Texture * this, GLenum arranged, GLenum atomTypeExternal); //set both internal and external format
void Texture_setDimensionCount(Texture * this, GLenum dimensions);

int Texture_free(Texture * texture);

void * Texture_read2(int x, int y, void * texel);
void Texture_write2(int x, int y, void * texel);
void * Texture_read3(int x, int y, int z);
void Texture_write3(int x, int y, int z);

void Transform_finalise(mat4x4 * matTrans, mat4x4 * matPos, mat4x4 * matRot);
void Transform_rotate(mat4x4 * matRot, vec3 * rotation, vec3 delta);
void Transform_translate(mat4x4 * matPos, vec3 * position, vec3 delta);

void RenderTexture_createDepth(Texture * const this, GLuint i, uint16_t width, uint16_t height);
void RenderTexture_createColor(Texture * const this, GLuint i, uint16_t width, uint16_t height, GLenum format);

GLuint GLBuffer_create(
    GLenum target,
    GLsizei size,
    const void *data,
	GLenum usage
);

void Shader_construct(Shader * this);

void Program_construct(Program * this, GLuint vertex_shader, GLuint fragment_shader, const char * attributeLocations[], size_t attributeLocationsCount);

void Engine_initialise(Engine * this);
void Engine_dispose(Engine * engine);
void Engine_clear(); //TODO should be Render_clear?
Program * Engine_setCurrentProgram(Engine * this, char * name);
Program * Engine_getCurrentProgram(Engine * this);
void Engine_loadShader(Engine * this, Shader ** shader, const char * path, const char * name, GLenum type);
void Engine_loadProgramFromConfig(Engine * engine, ProgramConfig programConfig, const char * path);
void Engine_loadProgramsFromConfig(Engine * engine, ProgramConfig programConfigs[], uint8_t programConfigsCount, const char * path);
void Engine_many(Program * program, RenderableSet * renderableSet, const GLfloat * matVP);
void Engine_one(Engine * this, Renderable * renderable);
void Engine_createScreenQuad(Engine * this, Mesh * mesh, GLuint positionVertexAttributeIndex, GLuint texcoordVertexAttributeIndex,
	int w, int h,
	int rcx, int rcy
);
float Engine_smoothstep(float t);

char* Text_load(char* filename);

void GLFW_errorCallback(int error, const char * description);
bool GLTool_isExtensionSupported(const char * extension); //redundant, see GLFW

/*
//Private:
#ifdef __ANDROID__
void 	Android_frame(Engine * engine);
void 	Android_onAppCmd(struct android_app* app, int32_t cmd);
int32_t Android_onInputEvent(struct android_app* app, AInputEvent* event);
#endif//__ANDROID__
*/

//globals
//Engine engine; //allows every other file to ref as extern, and no requirement to include main from renderer etc.
#ifdef DESKTOP
GLFWwindow * window; //TODO include as void * window in Engine
#endif//DESKTOP
#ifdef __ANDROID__
#endif//__ANDROID__

#endif //COM_ARCANEINGENUITY_ORB_H