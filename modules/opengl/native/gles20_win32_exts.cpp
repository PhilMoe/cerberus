#if _WIN32
typedef char GLchar;
typedef size_t GLintptr;
typedef size_t GLsizeiptr;
#define __gl2_h_ 
#define GL_ES_VERSION_2_0 1
#define GL_FUNC_ADD 0x8006
#define GL_BLEND_EQUATION 0x8009
#define GL_BLEND_EQUATION_RGB 0x8009
#define GL_BLEND_EQUATION_ALPHA 0x883D
#define GL_FUNC_SUBTRACT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_CONSTANT_COLOR 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_BLEND_COLOR 0x8005
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_BUFFER_SIZE 0x8764
#define GL_BUFFER_USAGE 0x8765
#define GL_CURRENT_VERTEX_ATTRIB 0x8626
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_SAMPLE_COVERAGE 0x80A0
#define GL_ALIASED_POINT_SIZE_RANGE 0x846D
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#define GL_STENCIL_BACK_FUNC 0x8800
#define GL_STENCIL_BACK_FAIL 0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL 0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS 0x8803
#define GL_STENCIL_BACK_REF 0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK 0x8CA4
#define GL_STENCIL_BACK_WRITEMASK 0x8CA5
#define GL_SAMPLE_BUFFERS 0x80A8
#define GL_SAMPLES 0x80A9
#define GL_SAMPLE_COVERAGE_VALUE 0x80AA
#define GL_SAMPLE_COVERAGE_INVERT 0x80AB
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS 0x86A3
#define GL_GENERATE_MIPMAP_HINT 0x8192
#define GL_FIXED 0x140C
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
#define GL_MAX_VARYING_VECTORS 0x8DFC
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS 0x8DFD
#define GL_SHADER_TYPE 0x8B4F
#define GL_DELETE_STATUS 0x8B80
#define GL_LINK_STATUS 0x8B82
#define GL_VALIDATE_STATUS 0x8B83
#define GL_ATTACHED_SHADERS 0x8B85
#define GL_ACTIVE_UNIFORMS 0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH 0x8B87
#define GL_ACTIVE_ATTRIBUTES 0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH 0x8B8A
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_INCR_WRAP 0x8507
#define GL_DECR_WRAP 0x8508
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6
#define GL_TEXTURE7 0x84C7
#define GL_TEXTURE8 0x84C8
#define GL_TEXTURE9 0x84C9
#define GL_TEXTURE10 0x84CA
#define GL_TEXTURE11 0x84CB
#define GL_TEXTURE12 0x84CC
#define GL_TEXTURE13 0x84CD
#define GL_TEXTURE14 0x84CE
#define GL_TEXTURE15 0x84CF
#define GL_TEXTURE16 0x84D0
#define GL_TEXTURE17 0x84D1
#define GL_TEXTURE18 0x84D2
#define GL_TEXTURE19 0x84D3
#define GL_TEXTURE20 0x84D4
#define GL_TEXTURE21 0x84D5
#define GL_TEXTURE22 0x84D6
#define GL_TEXTURE23 0x84D7
#define GL_TEXTURE24 0x84D8
#define GL_TEXTURE25 0x84D9
#define GL_TEXTURE26 0x84DA
#define GL_TEXTURE27 0x84DB
#define GL_TEXTURE28 0x84DC
#define GL_TEXTURE29 0x84DD
#define GL_TEXTURE30 0x84DE
#define GL_TEXTURE31 0x84DF
#define GL_ACTIVE_TEXTURE 0x84E0
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#define GL_MIRRORED_REPEAT 0x8370
#define GL_FLOAT_VEC2 0x8B50
#define GL_FLOAT_VEC3 0x8B51
#define GL_FLOAT_VEC4 0x8B52
#define GL_INT_VEC2 0x8B53
#define GL_INT_VEC3 0x8B54
#define GL_INT_VEC4 0x8B55
#define GL_BOOL 0x8B56
#define GL_BOOL_VEC2 0x8B57
#define GL_BOOL_VEC3 0x8B58
#define GL_BOOL_VEC4 0x8B59
#define GL_FLOAT_MAT2 0x8B5A
#define GL_FLOAT_MAT3 0x8B5B
#define GL_FLOAT_MAT4 0x8B5C
#define GL_SAMPLER_2D 0x8B5E
#define GL_SAMPLER_CUBE 0x8B60
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE 0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE 0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED 0x886A
#define GL_VERTEX_ATTRIB_ARRAY_POINTER 0x8645
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#define GL_IMPLEMENTATION_COLOR_READ_TYPE 0x8B9A
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT 0x8B9B
#define GL_COMPILE_STATUS 0x8B81
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_SHADER_SOURCE_LENGTH 0x8B88
#define GL_SHADER_COMPILER 0x8DFA
#define GL_SHADER_BINARY_FORMATS 0x8DF8
#define GL_NUM_SHADER_BINARY_FORMATS 0x8DF9
#define GL_LOW_FLOAT 0x8DF0
#define GL_MEDIUM_FLOAT 0x8DF1
#define GL_HIGH_FLOAT 0x8DF2
#define GL_LOW_INT 0x8DF3
#define GL_MEDIUM_INT 0x8DF4
#define GL_HIGH_INT 0x8DF5
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_RGB565 0x8D62
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_STENCIL_INDEX8 0x8D48
#define GL_RENDERBUFFER_WIDTH 0x8D42
#define GL_RENDERBUFFER_HEIGHT 0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT 0x8D44
#define GL_RENDERBUFFER_RED_SIZE 0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE 0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE 0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE 0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE 0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE 0x8D55
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_RENDERBUFFER_BINDING 0x8CA7
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
void(__stdcall*glActiveTexture)(GLenum texture);
void(__stdcall*glAttachShader)(GLuint program, GLuint shader);
void(__stdcall*glBindAttribLocation)(GLuint program, GLuint index, const GLchar* name);
void(__stdcall*glBindBuffer)(GLenum target, GLuint buffer);
void(__stdcall*glBindFramebuffer)(GLenum target, GLuint framebuffer);
void(__stdcall*glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
void(__stdcall*glBlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void(__stdcall*glBlendEquation)( GLenum mode );
void(__stdcall*glBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
void(__stdcall*glBlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void(__stdcall*glBufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
void(__stdcall*glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
GLenum(__stdcall*glCheckFramebufferStatus)(GLenum target);
void(__stdcall*glClearDepthf)(GLclampf depth);
void(__stdcall*glCompileShader)(GLuint shader);
void(__stdcall*glCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
void(__stdcall*glCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
GLuint(__stdcall*glCreateProgram)(void);
GLuint(__stdcall*glCreateShader)(GLenum type);
void(__stdcall*glDeleteBuffers)(GLsizei n, const GLuint* buffers);
void(__stdcall*glDeleteFramebuffers)(GLsizei n, const GLuint* framebuffers);
void(__stdcall*glDeleteProgram)(GLuint program);
void(__stdcall*glDeleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers);
void(__stdcall*glDeleteShader)(GLuint shader);
void(__stdcall*glDepthRangef)(GLclampf zNear, GLclampf zFar);
void(__stdcall*glDetachShader)(GLuint program, GLuint shader);
void(__stdcall*glDisableVertexAttribArray)(GLuint index);
void(__stdcall*glEnableVertexAttribArray)(GLuint index);
void(__stdcall*glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void(__stdcall*glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void(__stdcall*glGenBuffers)(GLsizei n, GLuint* buffers);
void(__stdcall*glGenerateMipmap)(GLenum target);
void(__stdcall*glGenFramebuffers)(GLsizei n, GLuint* framebuffers);
void(__stdcall*glGenRenderbuffers)(GLsizei n, GLuint* renderbuffers);
void(__stdcall*glGetActiveAttrib)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
void(__stdcall*glGetActiveUniform)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
void(__stdcall*glGetAttachedShaders)(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
int(__stdcall*glGetAttribLocation)(GLuint program, const GLchar* name);
void(__stdcall*glGetBufferParameteriv)(GLenum target, GLenum pname, GLint* params);
void(__stdcall*glGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
void(__stdcall*glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
void(__stdcall*glGetProgramInfoLog)(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
void(__stdcall*glGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint* params);
void(__stdcall*glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
void(__stdcall*glGetShaderInfoLog)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
void(__stdcall*glGetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
void(__stdcall*glGetShaderSource)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);
void(__stdcall*glGetUniformfv)(GLuint program, GLint location, GLfloat* params);
void(__stdcall*glGetUniformiv)(GLuint program, GLint location, GLint* params);
int(__stdcall*glGetUniformLocation)(GLuint program, const GLchar* name);
void(__stdcall*glGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat* params);
void(__stdcall*glGetVertexAttribiv)(GLuint index, GLenum pname, GLint* params);
void(__stdcall*glGetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid** pointer);
GLboolean(__stdcall*glIsBuffer)(GLuint buffer);
GLboolean(__stdcall*glIsFramebuffer)(GLuint framebuffer);
GLboolean(__stdcall*glIsProgram)(GLuint program);
GLboolean(__stdcall*glIsRenderbuffer)(GLuint renderbuffer);
GLboolean(__stdcall*glIsShader)(GLuint shader);
void(__stdcall*glLinkProgram)(GLuint program);
void(__stdcall*glReleaseShaderCompiler)(void);
void(__stdcall*glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void(__stdcall*glSampleCoverage)(GLclampf value, GLboolean invert);
void(__stdcall*glShaderBinary)(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length);
void(__stdcall*glShaderSource)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
void(__stdcall*glStencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
void(__stdcall*glStencilMaskSeparate)(GLenum face, GLuint mask);
void(__stdcall*glStencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
void(__stdcall*glUniform1f)(GLint location, GLfloat x);
void(__stdcall*glUniform1fv)(GLint location, GLsizei count, const GLfloat* v);
void(__stdcall*glUniform1i)(GLint location, GLint x);
void(__stdcall*glUniform1iv)(GLint location, GLsizei count, const GLint* v);
void(__stdcall*glUniform2f)(GLint location, GLfloat x, GLfloat y);
void(__stdcall*glUniform2fv)(GLint location, GLsizei count, const GLfloat* v);
void(__stdcall*glUniform2i)(GLint location, GLint x, GLint y);
void(__stdcall*glUniform2iv)(GLint location, GLsizei count, const GLint* v);
void(__stdcall*glUniform3f)(GLint location, GLfloat x, GLfloat y, GLfloat z);
void(__stdcall*glUniform3fv)(GLint location, GLsizei count, const GLfloat* v);
void(__stdcall*glUniform3i)(GLint location, GLint x, GLint y, GLint z);
void(__stdcall*glUniform3iv)(GLint location, GLsizei count, const GLint* v);
void(__stdcall*glUniform4f)(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void(__stdcall*glUniform4fv)(GLint location, GLsizei count, const GLfloat* v);
void(__stdcall*glUniform4i)(GLint location, GLint x, GLint y, GLint z, GLint w);
void(__stdcall*glUniform4iv)(GLint location, GLsizei count, const GLint* v);
void(__stdcall*glUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void(__stdcall*glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void(__stdcall*glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void(__stdcall*glUseProgram)(GLuint program);
void(__stdcall*glValidateProgram)(GLuint program);
void(__stdcall*glVertexAttrib1f)(GLuint indx, GLfloat x);
void(__stdcall*glVertexAttrib1fv)(GLuint indx, const GLfloat* values);
void(__stdcall*glVertexAttrib2f)(GLuint indx, GLfloat x, GLfloat y);
void(__stdcall*glVertexAttrib2fv)(GLuint indx, const GLfloat* values);
void(__stdcall*glVertexAttrib3f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
void(__stdcall*glVertexAttrib3fv)(GLuint indx, const GLfloat* values);
void(__stdcall*glVertexAttrib4f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void(__stdcall*glVertexAttrib4fv)(GLuint indx, const GLfloat* values);
void(__stdcall*glVertexAttribPointer)(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
void Init_GL_Exts(){
	(void*&)glActiveTexture=(void*)wglGetProcAddress("glActiveTexture");
	(void*&)glAttachShader=(void*)wglGetProcAddress("glAttachShader");
	(void*&)glBindAttribLocation=(void*)wglGetProcAddress("glBindAttribLocation");
	(void*&)glBindBuffer=(void*)wglGetProcAddress("glBindBuffer");
	(void*&)glBindFramebuffer=(void*)wglGetProcAddress("glBindFramebuffer");
	(void*&)glBindRenderbuffer=(void*)wglGetProcAddress("glBindRenderbuffer");
	(void*&)glBlendColor=(void*)wglGetProcAddress("glBlendColor");
	(void*&)glBlendEquation=(void*)wglGetProcAddress("glBlendEquation");
	(void*&)glBlendEquationSeparate=(void*)wglGetProcAddress("glBlendEquationSeparate");
	(void*&)glBlendFuncSeparate=(void*)wglGetProcAddress("glBlendFuncSeparate");
	(void*&)glBufferData=(void*)wglGetProcAddress("glBufferData");
	(void*&)glBufferSubData=(void*)wglGetProcAddress("glBufferSubData");
	(void*&)glCheckFramebufferStatus=(void*)wglGetProcAddress("glCheckFramebufferStatus");
	(void*&)glClearDepthf=(void*)wglGetProcAddress("glClearDepthf");
	(void*&)glCompileShader=(void*)wglGetProcAddress("glCompileShader");
	(void*&)glCompressedTexImage2D=(void*)wglGetProcAddress("glCompressedTexImage2D");
	(void*&)glCompressedTexSubImage2D=(void*)wglGetProcAddress("glCompressedTexSubImage2D");
	(void*&)glCreateProgram=(void*)wglGetProcAddress("glCreateProgram");
	(void*&)glCreateShader=(void*)wglGetProcAddress("glCreateShader");
	(void*&)glDeleteBuffers=(void*)wglGetProcAddress("glDeleteBuffers");
	(void*&)glDeleteFramebuffers=(void*)wglGetProcAddress("glDeleteFramebuffers");
	(void*&)glDeleteProgram=(void*)wglGetProcAddress("glDeleteProgram");
	(void*&)glDeleteRenderbuffers=(void*)wglGetProcAddress("glDeleteRenderbuffers");
	(void*&)glDeleteShader=(void*)wglGetProcAddress("glDeleteShader");
	(void*&)glDepthRangef=(void*)wglGetProcAddress("glDepthRangef");
	(void*&)glDetachShader=(void*)wglGetProcAddress("glDetachShader");
	(void*&)glDisableVertexAttribArray=(void*)wglGetProcAddress("glDisableVertexAttribArray");
	(void*&)glEnableVertexAttribArray=(void*)wglGetProcAddress("glEnableVertexAttribArray");
	(void*&)glFramebufferRenderbuffer=(void*)wglGetProcAddress("glFramebufferRenderbuffer");
	(void*&)glFramebufferTexture2D=(void*)wglGetProcAddress("glFramebufferTexture2D");
	(void*&)glGenBuffers=(void*)wglGetProcAddress("glGenBuffers");
	(void*&)glGenerateMipmap=(void*)wglGetProcAddress("glGenerateMipmap");
	(void*&)glGenFramebuffers=(void*)wglGetProcAddress("glGenFramebuffers");
	(void*&)glGenRenderbuffers=(void*)wglGetProcAddress("glGenRenderbuffers");
	(void*&)glGetActiveAttrib=(void*)wglGetProcAddress("glGetActiveAttrib");
	(void*&)glGetActiveUniform=(void*)wglGetProcAddress("glGetActiveUniform");
	(void*&)glGetAttachedShaders=(void*)wglGetProcAddress("glGetAttachedShaders");
	(void*&)glGetAttribLocation=(void*)wglGetProcAddress("glGetAttribLocation");
	(void*&)glGetBufferParameteriv=(void*)wglGetProcAddress("glGetBufferParameteriv");
	(void*&)glGetFramebufferAttachmentParameteriv=(void*)wglGetProcAddress("glGetFramebufferAttachmentParameteriv");
	(void*&)glGetProgramiv=(void*)wglGetProcAddress("glGetProgramiv");
	(void*&)glGetProgramInfoLog=(void*)wglGetProcAddress("glGetProgramInfoLog");
	(void*&)glGetRenderbufferParameteriv=(void*)wglGetProcAddress("glGetRenderbufferParameteriv");
	(void*&)glGetShaderiv=(void*)wglGetProcAddress("glGetShaderiv");
	(void*&)glGetShaderInfoLog=(void*)wglGetProcAddress("glGetShaderInfoLog");
	(void*&)glGetShaderPrecisionFormat=(void*)wglGetProcAddress("glGetShaderPrecisionFormat");
	(void*&)glGetShaderSource=(void*)wglGetProcAddress("glGetShaderSource");
	(void*&)glGetUniformfv=(void*)wglGetProcAddress("glGetUniformfv");
	(void*&)glGetUniformiv=(void*)wglGetProcAddress("glGetUniformiv");
	(void*&)glGetUniformLocation=(void*)wglGetProcAddress("glGetUniformLocation");
	(void*&)glGetVertexAttribfv=(void*)wglGetProcAddress("glGetVertexAttribfv");
	(void*&)glGetVertexAttribiv=(void*)wglGetProcAddress("glGetVertexAttribiv");
	(void*&)glGetVertexAttribPointerv=(void*)wglGetProcAddress("glGetVertexAttribPointerv");
	(void*&)glIsBuffer=(void*)wglGetProcAddress("glIsBuffer");
	(void*&)glIsFramebuffer=(void*)wglGetProcAddress("glIsFramebuffer");
	(void*&)glIsProgram=(void*)wglGetProcAddress("glIsProgram");
	(void*&)glIsRenderbuffer=(void*)wglGetProcAddress("glIsRenderbuffer");
	(void*&)glIsShader=(void*)wglGetProcAddress("glIsShader");
	(void*&)glLinkProgram=(void*)wglGetProcAddress("glLinkProgram");
	(void*&)glReleaseShaderCompiler=(void*)wglGetProcAddress("glReleaseShaderCompiler");
	(void*&)glRenderbufferStorage=(void*)wglGetProcAddress("glRenderbufferStorage");
	(void*&)glSampleCoverage=(void*)wglGetProcAddress("glSampleCoverage");
	(void*&)glShaderBinary=(void*)wglGetProcAddress("glShaderBinary");
	(void*&)glShaderSource=(void*)wglGetProcAddress("glShaderSource");
	(void*&)glStencilFuncSeparate=(void*)wglGetProcAddress("glStencilFuncSeparate");
	(void*&)glStencilMaskSeparate=(void*)wglGetProcAddress("glStencilMaskSeparate");
	(void*&)glStencilOpSeparate=(void*)wglGetProcAddress("glStencilOpSeparate");
	(void*&)glUniform1f=(void*)wglGetProcAddress("glUniform1f");
	(void*&)glUniform1fv=(void*)wglGetProcAddress("glUniform1fv");
	(void*&)glUniform1i=(void*)wglGetProcAddress("glUniform1i");
	(void*&)glUniform1iv=(void*)wglGetProcAddress("glUniform1iv");
	(void*&)glUniform2f=(void*)wglGetProcAddress("glUniform2f");
	(void*&)glUniform2fv=(void*)wglGetProcAddress("glUniform2fv");
	(void*&)glUniform2i=(void*)wglGetProcAddress("glUniform2i");
	(void*&)glUniform2iv=(void*)wglGetProcAddress("glUniform2iv");
	(void*&)glUniform3f=(void*)wglGetProcAddress("glUniform3f");
	(void*&)glUniform3fv=(void*)wglGetProcAddress("glUniform3fv");
	(void*&)glUniform3i=(void*)wglGetProcAddress("glUniform3i");
	(void*&)glUniform3iv=(void*)wglGetProcAddress("glUniform3iv");
	(void*&)glUniform4f=(void*)wglGetProcAddress("glUniform4f");
	(void*&)glUniform4fv=(void*)wglGetProcAddress("glUniform4fv");
	(void*&)glUniform4i=(void*)wglGetProcAddress("glUniform4i");
	(void*&)glUniform4iv=(void*)wglGetProcAddress("glUniform4iv");
	(void*&)glUniformMatrix2fv=(void*)wglGetProcAddress("glUniformMatrix2fv");
	(void*&)glUniformMatrix3fv=(void*)wglGetProcAddress("glUniformMatrix3fv");
	(void*&)glUniformMatrix4fv=(void*)wglGetProcAddress("glUniformMatrix4fv");
	(void*&)glUseProgram=(void*)wglGetProcAddress("glUseProgram");
	(void*&)glValidateProgram=(void*)wglGetProcAddress("glValidateProgram");
	(void*&)glVertexAttrib1f=(void*)wglGetProcAddress("glVertexAttrib1f");
	(void*&)glVertexAttrib1fv=(void*)wglGetProcAddress("glVertexAttrib1fv");
	(void*&)glVertexAttrib2f=(void*)wglGetProcAddress("glVertexAttrib2f");
	(void*&)glVertexAttrib2fv=(void*)wglGetProcAddress("glVertexAttrib2fv");
	(void*&)glVertexAttrib3f=(void*)wglGetProcAddress("glVertexAttrib3f");
	(void*&)glVertexAttrib3fv=(void*)wglGetProcAddress("glVertexAttrib3fv");
	(void*&)glVertexAttrib4f=(void*)wglGetProcAddress("glVertexAttrib4f");
	(void*&)glVertexAttrib4fv=(void*)wglGetProcAddress("glVertexAttrib4fv");
	(void*&)glVertexAttribPointer=(void*)wglGetProcAddress("glVertexAttribPointer");
}
#endif
