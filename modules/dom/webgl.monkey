
Import dom

Import "webgl.js"

Extern

'Based on interfaces published at:
'
'https://www.khronos.org/registry/webgl/specs/1.0/#5.3

Class ArrayBuffer
	Field byteLength	'ReadOnly
End

Class ArrayBufferView
	Field buffer:ArrayBuffer	'ReadOnly
	Field byteOffset			'ReadOnly
	Field byteLength			'ReadOnly
End

Class DataView Extends ArrayBufferView
	Method getInt8( byteOffset )
	Method getUint8( byteOffset )
	Method getInt16( byteoffset )
	Method getUint16( byteOffset )
	Method getInt32( byteOffset )
	Method getUint32( byteOffset )
	Method getFloat32#( byteOffset )
	Method getFloat64#( byteOffset )
	Method getInt16( byteoffset,littleEndian? )
	Method getUint16( byteOffset,littleEndian? )
	Method getInt32( byteOffset,littleEndian? )
	Method getUint32( byteOffset,littleEndian? )
	Method getFloat32#( byteOffset,littleEndian? )
	Method getFloat64#( byteOffset,littleEndian? )
	Method setInt8( byteOffset,value )
	Method setUint8( byteOffset,value )
	Method setInt16( byteOffset,value )
	Method setUint16( byteOffset,value )
	Method setInt32( byteOffset,value )
	Method setUint32( byteOffset,value )
	Method setFloat32( byteOffset,value# )
	Method setFloat64( byteOffset,value# )
	Method setInt16( byteOffset,value,littleEndian? )
	Method setUint16( byteOffset,value,littleEndian? )
	Method setInt32( byteOffset,value,littleEndian? )
	Method setUint32( byteOffset,value,littleEndian? )
	Method setFloat32( byteOffset,value#,littleEndian? )
	Method setFloat64( byteOffset,value#,littleEndian? )
End

Class TypedArray Extends ArrayBufferView
	Field BYTES_PER_ELEMENT		'Const
	Field length				'ReadOnly
End

Function createArrayBuffer:ArrayBuffer( byteLength )
Function createDataView:DataView( buffer:ArrayBuffer,byteOffset,byteLength )

Function createInt8Array[]( buffer:ArrayBuffer,byteOffset,length ) 
Function createInt16Array[]( buffer:ArrayBuffer,byteOffset,length ) 
Function createInt32Array[]( buffer:ArrayBuffer,byteOffset,length ) 
Function createFloat32Array#[]( buffer:ArrayBuffer,byteOffset,length )

Class WebGLObject Extends DOMObject
End

Class WebGLBuffer Extends WebGLObject
End

Class WebGLFramebuffer Extends WebGLObject
End

Class WebGLProgram Extends WebGLObject
End

Class WebGLRenderbuffer Extends WebGLObject
End

Class WebGLShader Extends WebGLObject
End

Class WebGLTexture Extends WebGLObject
End

Class WebGLUniformLocation Extends DOMObject
End

Class WebGLActiveInfo Extends DOMObject
	Field size
	Field type
	Field name$
End

Class WebGLContextAttributes Extends DOMObject
	Field alpha?
	Field depth?
	Field stencil?
	Field antialias?
	Field premultipliedAlpha?
	Field preserveDrawingBuffer?
End

Class WebGLRenderingContext Extends DOMObject

	Field canvas:HTMLCanvasElement	'ReadOnly
	Field drawingBufferWidth		'ReadOnly
	Field drawingBufferHeight		'ReadOnly

	Method getContextAttributes:WebGLContextAttributes()
	Method isContextLost?()
	
	Method getSupportedExtensions$[]()
	Method getExtension:Object( name$ )

'Generated code starts below next line
'
'*****[WebGLRenderingContext]*****
	'/* ClearBufferMask */
	Field DEPTH_BUFFER_BIT
	Field STENCIL_BUFFER_BIT
	Field COLOR_BUFFER_BIT
	'/* BeginMode */
	Field POINTS
	Field LINES
	Field LINE_LOOP
	Field LINE_STRIP
	Field TRIANGLES
	Field TRIANGLE_STRIP
	Field TRIANGLE_FAN
	'/* AlphaFunction (not supported in ES20) */
	'/*      NEVER */
	'/*      LESS */
	'/*      EQUAL */
	'/*      LEQUAL */
	'/*      GREATER */
	'/*      NOTEQUAL */
	'/*      GEQUAL */
	'/*      ALWAYS */
	'/* BlendingFactorDest */
	Field ZERO
	Field ONE
	Field SRC_COLOR
	Field ONE_MINUS_SRC_COLOR
	Field SRC_ALPHA
	Field ONE_MINUS_SRC_ALPHA
	Field DST_ALPHA
	Field ONE_MINUS_DST_ALPHA
	'/* BlendingFactorSrc */
	'/*      ZERO */
	'/*      ONE */
	Field DST_COLOR
	Field ONE_MINUS_DST_COLOR
	Field SRC_ALPHA_SATURATE
	'/*      SRC_ALPHA */
	'/*      ONE_MINUS_SRC_ALPHA */
	'/*      DST_ALPHA */
	'/*      ONE_MINUS_DST_ALPHA */
	'/* BlendEquationSeparate */
	Field FUNC_ADD
	Field BLEND_EQUATION
	Field BLEND_EQUATION_RGB
	Field BLEND_EQUATION_ALPHA
	'/* BlendSubtract */
	Field FUNC_SUBTRACT
	Field FUNC_REVERSE_SUBTRACT
	'/* Separate Blend Functions */
	Field BLEND_DST_RGB
	Field BLEND_SRC_RGB
	Field BLEND_DST_ALPHA
	Field BLEND_SRC_ALPHA
	Field CONSTANT_COLOR
	Field ONE_MINUS_CONSTANT_COLOR
	Field CONSTANT_ALPHA
	Field ONE_MINUS_CONSTANT_ALPHA
	Field BLEND_COLOR
	'/* Buffer Objects */
	Field ARRAY_BUFFER
	Field ELEMENT_ARRAY_BUFFER
	Field ARRAY_BUFFER_BINDING
	Field ELEMENT_ARRAY_BUFFER_BINDING
	Field STREAM_DRAW
	Field STATIC_DRAW
	Field DYNAMIC_DRAW
	Field BUFFER_SIZE
	Field BUFFER_USAGE
	Field CURRENT_VERTEX_ATTRIB
	'/* CullFaceMode */
	Field FRONT
	Field BACK
	Field FRONT_AND_BACK
	'/* DepthFunction */
	'/*      NEVER */
	'/*      LESS */
	'/*      EQUAL */
	'/*      LEQUAL */
	'/*      GREATER */
	'/*      NOTEQUAL */
	'/*      GEQUAL */
	'/*      ALWAYS */
	'/* EnableCap */
	'/* TEXTURE_2D */
	Field CULL_FACE
	Field BLEND
	Field DITHER
	Field STENCIL_TEST
	Field DEPTH_TEST
	Field SCISSOR_TEST
	Field POLYGON_OFFSET_FILL
	Field SAMPLE_ALPHA_TO_COVERAGE
	Field SAMPLE_COVERAGE
	'/* ErrorCode */
	Field NO_ERROR
	Field INVALID_ENUM
	Field INVALID_VALUE
	Field INVALID_OPERATION
	Field OUT_OF_MEMORY
	'/* FrontFaceDirection */
	Field CW
	Field CCW
	'/* GetPName */
	Field LINE_WIDTH
	Field ALIASED_POINT_SIZE_RANGE
	Field ALIASED_LINE_WIDTH_RANGE
	Field CULL_FACE_MODE
	Field FRONT_FACE
	Field DEPTH_RANGE
	Field DEPTH_WRITEMASK
	Field DEPTH_CLEAR_VALUE
	Field DEPTH_FUNC
	Field STENCIL_CLEAR_VALUE
	Field STENCIL_FUNC
	Field STENCIL_FAIL
	Field STENCIL_PASS_DEPTH_FAIL
	Field STENCIL_PASS_DEPTH_PASS
	Field STENCIL_REF
	Field STENCIL_VALUE_MASK
	Field STENCIL_WRITEMASK
	Field STENCIL_BACK_FUNC
	Field STENCIL_BACK_FAIL
	Field STENCIL_BACK_PASS_DEPTH_FAIL
	Field STENCIL_BACK_PASS_DEPTH_PASS
	Field STENCIL_BACK_REF
	Field STENCIL_BACK_VALUE_MASK
	Field STENCIL_BACK_WRITEMASK
	Field VIEWPORT
	Field SCISSOR_BOX
	'/*      SCISSOR_TEST */
	Field COLOR_CLEAR_VALUE
	Field COLOR_WRITEMASK
	Field UNPACK_ALIGNMENT
	Field PACK_ALIGNMENT
	Field MAX_TEXTURE_SIZE
	Field MAX_VIEWPORT_DIMS
	Field SUBPIXEL_BITS
	Field RED_BITS
	Field GREEN_BITS
	Field BLUE_BITS
	Field ALPHA_BITS
	Field DEPTH_BITS
	Field STENCIL_BITS
	Field POLYGON_OFFSET_UNITS
	'/*      POLYGON_OFFSET_FILL */
	Field POLYGON_OFFSET_FACTOR
	Field TEXTURE_BINDING_2D
	Field SAMPLE_BUFFERS
	Field SAMPLES
	Field SAMPLE_COVERAGE_VALUE
	Field SAMPLE_COVERAGE_INVERT
	'/* GetTextureParameter */
	'/*      TEXTURE_MAG_FILTER */
	'/*      TEXTURE_MIN_FILTER */
	'/*      TEXTURE_WRAP_S */
	'/*      TEXTURE_WRAP_T */
	Field NUM_COMPRESSED_TEXTURE_FORMATS
	Field COMPRESSED_TEXTURE_FORMATS
	'/* HintMode */
	Field DONT_CARE
	Field FASTEST
	Field NICEST
	'/* HintTarget */
	Field GENERATE_MIPMAP_HINT
	'/* DataType */
	Field BYTE
	Field UNSIGNED_BYTE
	Field SHORT
	Field UNSIGNED_SHORT
	Field INT_="INT"
	Field UNSIGNED_INT
	Field FLOAT_="FLOAT"
	'/* PixelFormat */
	Field DEPTH_COMPONENT
	Field ALPHA
	Field RGB
	Field RGBA
	Field LUMINANCE
	Field LUMINANCE_ALPHA
	'/* PixelType */
	'/*      UNSIGNED_BYTE */
	Field UNSIGNED_SHORT_4_4_4_4
	Field UNSIGNED_SHORT_5_5_5_1
	Field UNSIGNED_SHORT_5_6_5
	'/* Shaders */
	Field FRAGMENT_SHADER
	Field VERTEX_SHADER
	Field MAX_VERTEX_ATTRIBS
	Field MAX_VERTEX_UNIFORM_VECTORS
	Field MAX_VARYING_VECTORS
	Field MAX_COMBINED_TEXTURE_IMAGE_UNITS
	Field MAX_VERTEX_TEXTURE_IMAGE_UNITS
	Field MAX_TEXTURE_IMAGE_UNITS
	Field MAX_FRAGMENT_UNIFORM_VECTORS
	Field SHADER_TYPE
	Field DELETE_STATUS
	Field LINK_STATUS
	Field VALIDATE_STATUS
	Field ATTACHED_SHADERS
	Field ACTIVE_UNIFORMS
	Field ACTIVE_ATTRIBUTES
	Field SHADING_LANGUAGE_VERSION
	Field CURRENT_PROGRAM
	'/* StencilFunction */
	Field NEVER
	Field LESS
	Field EQUAL
	Field LEQUAL
	Field GREATER
	Field NOTEQUAL
	Field GEQUAL
	Field ALWAYS
	'/* StencilOp */
	'/*      ZERO */
	Field KEEP
	Field REPLACE
	Field INCR
	Field DECR
	Field INVERT
	Field INCR_WRAP
	Field DECR_WRAP
	'/* StringName */
	Field VENDOR
	Field RENDERER
	Field VERSION
	'/* TextureMagFilter */
	Field NEAREST
	Field LINEAR
	'/* TextureMinFilter */
	'/*      NEAREST */
	'/*      LINEAR */
	Field NEAREST_MIPMAP_NEAREST
	Field LINEAR_MIPMAP_NEAREST
	Field NEAREST_MIPMAP_LINEAR
	Field LINEAR_MIPMAP_LINEAR
	'/* TextureParameterName */
	Field TEXTURE_MAG_FILTER
	Field TEXTURE_MIN_FILTER
	Field TEXTURE_WRAP_S
	Field TEXTURE_WRAP_T
	'/* TextureTarget */
	Field TEXTURE_2D
	Field TEXTURE
	Field TEXTURE_CUBE_MAP
	Field TEXTURE_BINDING_CUBE_MAP
	Field TEXTURE_CUBE_MAP_POSITIVE_X
	Field TEXTURE_CUBE_MAP_NEGATIVE_X
	Field TEXTURE_CUBE_MAP_POSITIVE_Y
	Field TEXTURE_CUBE_MAP_NEGATIVE_Y
	Field TEXTURE_CUBE_MAP_POSITIVE_Z
	Field TEXTURE_CUBE_MAP_NEGATIVE_Z
	Field MAX_CUBE_MAP_TEXTURE_SIZE
	'/* TextureUnit */
	Field TEXTURE0
	Field TEXTURE1
	Field TEXTURE2
	Field TEXTURE3
	Field TEXTURE4
	Field TEXTURE5
	Field TEXTURE6
	Field TEXTURE7
	Field TEXTURE8
	Field TEXTURE9
	Field TEXTURE10
	Field TEXTURE11
	Field TEXTURE12
	Field TEXTURE13
	Field TEXTURE14
	Field TEXTURE15
	Field TEXTURE16
	Field TEXTURE17
	Field TEXTURE18
	Field TEXTURE19
	Field TEXTURE20
	Field TEXTURE21
	Field TEXTURE22
	Field TEXTURE23
	Field TEXTURE24
	Field TEXTURE25
	Field TEXTURE26
	Field TEXTURE27
	Field TEXTURE28
	Field TEXTURE29
	Field TEXTURE30
	Field TEXTURE31
	Field ACTIVE_TEXTURE
	'/* TextureWrapMode */
	Field REPEAT_="REPEAT"
	Field CLAMP_TO_EDGE
	Field MIRRORED_REPEAT
	'/* Uniform Types */
	Field FLOAT_VEC2
	Field FLOAT_VEC3
	Field FLOAT_VEC4
	Field INT_VEC2
	Field INT_VEC3
	Field INT_VEC4
	Field BOOL_="BOOL"
	Field BOOL_VEC2
	Field BOOL_VEC3
	Field BOOL_VEC4
	Field FLOAT_MAT2
	Field FLOAT_MAT3
	Field FLOAT_MAT4
	Field SAMPLER_2D
	Field SAMPLER_CUBE
	'/* Vertex Arrays */
	Field VERTEX_ATTRIB_ARRAY_ENABLED
	Field VERTEX_ATTRIB_ARRAY_SIZE
	Field VERTEX_ATTRIB_ARRAY_STRIDE
	Field VERTEX_ATTRIB_ARRAY_TYPE
	Field VERTEX_ATTRIB_ARRAY_NORMALIZED
	Field VERTEX_ATTRIB_ARRAY_POINTER
	Field VERTEX_ATTRIB_ARRAY_BUFFER_BINDING
	'/* Shader Source */
	Field COMPILE_STATUS
	'/* Shader Precision-Specified Types */
	Field LOW_FLOAT
	Field MEDIUM_FLOAT
	Field HIGH_FLOAT
	Field LOW_INT
	Field MEDIUM_INT
	Field HIGH_INT
	'/* Framebuffer Object. */
	Field FRAMEBUFFER
	Field RENDERBUFFER
	Field RGBA4
	Field RGB5_A1
	Field RGB565
	Field DEPTH_COMPONENT16
	Field STENCIL_INDEX
	Field STENCIL_INDEX8
	Field DEPTH_STENCIL
	Field RENDERBUFFER_WIDTH
	Field RENDERBUFFER_HEIGHT
	Field RENDERBUFFER_INTERNAL_FORMAT
	Field RENDERBUFFER_RED_SIZE
	Field RENDERBUFFER_GREEN_SIZE
	Field RENDERBUFFER_BLUE_SIZE
	Field RENDERBUFFER_ALPHA_SIZE
	Field RENDERBUFFER_DEPTH_SIZE
	Field RENDERBUFFER_STENCIL_SIZE
	Field FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE
	Field FRAMEBUFFER_ATTACHMENT_OBJECT_NAME
	Field FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL
	Field FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE
	Field COLOR_ATTACHMENT0
	Field DEPTH_ATTACHMENT
	Field STENCIL_ATTACHMENT
	Field DEPTH_STENCIL_ATTACHMENT
	Field NONE
	Field FRAMEBUFFER_COMPLETE
	Field FRAMEBUFFER_INCOMPLETE_ATTACHMENT
	Field FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
	Field FRAMEBUFFER_INCOMPLETE_DIMENSIONS
	Field FRAMEBUFFER_UNSUPPORTED
	Field FRAMEBUFFER_BINDING
	Field RENDERBUFFER_BINDING
	Field MAX_RENDERBUFFER_SIZE
	Field INVALID_FRAMEBUFFER_OPERATION
	'/* WebGL-specific enums */
	Field UNPACK_FLIP_Y_WEBGL
	Field UNPACK_PREMULTIPLY_ALPHA_WEBGL
	Field CONTEXT_LOST_WEBGL
	Field UNPACK_COLORSPACE_CONVERSION_WEBGL
	Field BROWSER_DEFAULT_WEBGL
	Method activeTexture:Void(texture)
	Method attachShader:Void(program:WebGLProgram,shader:WebGLShader)
	Method bindAttribLocation:Void(program:WebGLProgram,index,name$)
	Method bindBuffer:Void(target,buffer:WebGLBuffer)
	Method bindFramebuffer:Void(target,framebuffer:WebGLFramebuffer)
	Method bindRenderbuffer:Void(target,renderbuffer:WebGLRenderbuffer)
	Method bindTexture:Void(target,texture:WebGLTexture)
	Method blendColor:Void(red#,green#,blue#,alpha#)
	Method blendEquation:Void(mode)
	Method blendEquationSeparate:Void(modeRGB,modeAlpha)
	Method blendFunc:Void(sfactor,dfactor)
	Method blendFuncSeparate:Void(srcRGB,dstRGB,srcAlpha,dstAlpha)
	Method bufferData:Void(target,size,usage)
	Method bufferData:Void(target,data:ArrayBufferView,usage)
	Method bufferData:Void(target,data:ArrayBuffer,usage)
	Method bufferSubData:Void(target,offset,data:ArrayBufferView)
	Method bufferSubData:Void(target,offset,data:ArrayBuffer)
	Method checkFramebufferStatus(target)
	Method clear:Void(mask)
	Method clearColor:Void(red#,green#,blue#,alpha#)
	Method clearDepth:Void(depth#)
	Method clearStencil:Void(s)
	Method colorMask:Void(red?,green?,blue?,alpha?)
	Method compileShader:Void(shader:WebGLShader)
	Method copyTexImage2D:Void(target,level,internalformat,x,y,width,height,border)
	Method copyTexSubImage2D:Void(target,level,xoffset,yoffset,x,y,width,height)
	Method createBuffer:WebGLBuffer()
	Method createFramebuffer:WebGLFramebuffer()
	Method createProgram:WebGLProgram()
	Method createRenderbuffer:WebGLRenderbuffer()
	Method createShader:WebGLShader(type)
	Method createTexture:WebGLTexture()
	Method cullFace:Void(mode)
	Method deleteBuffer:Void(buffer:WebGLBuffer)
	Method deleteFramebuffer:Void(framebuffer:WebGLFramebuffer)
	Method deleteProgram:Void(program:WebGLProgram)
	Method deleteRenderbuffer:Void(renderbuffer:WebGLRenderbuffer)
	Method deleteShader:Void(shader:WebGLShader)
	Method deleteTexture:Void(texture:WebGLTexture)
	Method depthFunc:Void(func)
	Method depthMask:Void(flag?)
	Method depthRange:Void(zNear#,zFar#)
	Method detachShader:Void(program:WebGLProgram,shader:WebGLShader)
	Method disable:Void(cap)
	Method disableVertexAttribArray:Void(index)
	Method drawArrays:Void(mode,first,count)
	Method drawElements:Void(mode,count,type,offset)
	Method enable:Void(cap)
	Method enableVertexAttribArray:Void(index)
	Method finish:Void()
	Method flush:Void()
	Method framebufferRenderbuffer:Void(target,attachment,renderbuffertarget,renderbuffer:WebGLRenderbuffer)
	Method framebufferTexture2D:Void(target,attachment,textarget,texture:WebGLTexture,level)
	Method frontFace:Void(mode)
	Method generateMipmap:Void(target)
	Method getActiveAttrib:WebGLActiveInfo(program:WebGLProgram,index)
	Method getActiveUniform:WebGLActiveInfo(program:WebGLProgram,index)
	Method getAttachedShaders:WebGLShader[](program:WebGLProgram)
	Method getAttribLocation(program:WebGLProgram,name$)
	'Method getParameter:any(pname)
	'Method getBufferParameter:any(target,pname)
	Method getError()
	'Method getFramebufferAttachmentParameter:any(target,attachment,pname)
	'/* Munged by Mark */
	'/* any getProgramParameter(WebGLProgram program, GLenum pname); */
	Method getProgramParameter(program:WebGLProgram,pname)
	Method getProgramInfoLog$(program:WebGLProgram)
	'Method getRenderbufferParameter:any(target,pname)
	'/* Munged by Mark */
	'/* any getShaderParameter(WebGLShader shader, GLenum pname); */
	Method getShaderParameter(shader:WebGLShader,pname)
	Method getShaderInfoLog$(shader:WebGLShader)
	Method getShaderSource$(shader:WebGLShader)
	'Method getTexParameter:any(target,pname)
	'Method getUniform:any(program:WebGLProgram,location:WebGLUniformLocation)
	Method getUniformLocation:WebGLUniformLocation(program:WebGLProgram,name$)
	'Method getVertexAttrib:any(index,pname)
	Method getVertexAttribOffset(index,pname)
	Method hint:Void(target,mode)
	Method isBuffer?(buffer:WebGLBuffer)
	Method isEnabled?(cap)
	Method isFramebuffer?(framebuffer:WebGLFramebuffer)
	Method isProgram?(program:WebGLProgram)
	Method isRenderbuffer?(renderbuffer:WebGLRenderbuffer)
	Method isShader?(shader:WebGLShader)
	Method isTexture?(texture:WebGLTexture)
	Method lineWidth:Void(width#)
	Method linkProgram:Void(program:WebGLProgram)
	Method pixelStorei:Void(pname,param)
	Method polygonOffset:Void(factor#,units#)
	Method readPixels:Void(x,y,width,height,format,type,pixels:ArrayBufferView)
	Method renderbufferStorage:Void(target,internalformat,width,height)
	Method sampleCoverage:Void(value#,invert?)
	Method scissor:Void(x,y,width,height)
	Method shaderSource:Void(shader:WebGLShader,source$)
	Method stencilFunc:Void(func,ref,mask)
	Method stencilFuncSeparate:Void(face,func,ref,mask)
	Method stencilMask:Void(mask)
	Method stencilMaskSeparate:Void(face,mask)
	Method stencilOp:Void(fail,zfail,zpass)
	Method stencilOpSeparate:Void(face,fail,zfail,zpass)
	Method texImage2D:Void(target,level,internalformat,width,height,border,format,type,pixels:ArrayBufferView)
	Method texImage2D:Void(target,level,internalformat,format,type,pixels:ImageData)
	Method texImage2D:Void(target,level,internalformat,format,type,image:HTMLImageElement)
	Method texImage2D:Void(target,level,internalformat,format,type,canvas:HTMLCanvasElement)
	Method texImage2D:Void(target,level,internalformat,format,type,video:HTMLVideoElement)
	Method texParameterf:Void(target,pname,param#)
	Method texParameteri:Void(target,pname,param)
	Method texSubImage2D:Void(target,level,xoffset,yoffset,width,height,format,type,pixels:ArrayBufferView)
	Method texSubImage2D:Void(target,level,xoffset,yoffset,format,type,pixels:ImageData)
	Method texSubImage2D:Void(target,level,xoffset,yoffset,format,type,image:HTMLImageElement)
	Method texSubImage2D:Void(target,level,xoffset,yoffset,format,type,canvas:HTMLCanvasElement)
	Method texSubImage2D:Void(target,level,xoffset,yoffset,format,type,video:HTMLVideoElement)
	Method uniform1f:Void(location:WebGLUniformLocation,x#)
	'Method uniform1fv:Void(location:WebGLUniformLocation,v:Float32Array)
	Method uniform1fv:Void(location:WebGLUniformLocation,v#[])
	Method uniform1i:Void(location:WebGLUniformLocation,x)
	'Method uniform1iv:Void(location:WebGLUniformLocation,v:Int32Array)
	Method uniform1iv:Void(location:WebGLUniformLocation,v[])
	Method uniform2f:Void(location:WebGLUniformLocation,x#,y#)
	'Method uniform2fv:Void(location:WebGLUniformLocation,v:Float32Array)
	Method uniform2fv:Void(location:WebGLUniformLocation,v#[])
	Method uniform2i:Void(location:WebGLUniformLocation,x,y)
	'Method uniform2iv:Void(location:WebGLUniformLocation,v:Int32Array)
	Method uniform2iv:Void(location:WebGLUniformLocation,v[])
	Method uniform3f:Void(location:WebGLUniformLocation,x#,y#,z#)
	'Method uniform3fv:Void(location:WebGLUniformLocation,v:Float32Array)
	Method uniform3fv:Void(location:WebGLUniformLocation,v#[])
	Method uniform3i:Void(location:WebGLUniformLocation,x,y,z)
	'Method uniform3iv:Void(location:WebGLUniformLocation,v:Int32Array)
	Method uniform3iv:Void(location:WebGLUniformLocation,v[])
	Method uniform4f:Void(location:WebGLUniformLocation,x#,y#,z#,w#)
	'Method uniform4fv:Void(location:WebGLUniformLocation,v:Float32Array)
	Method uniform4fv:Void(location:WebGLUniformLocation,v#[])
	Method uniform4i:Void(location:WebGLUniformLocation,x,y,z,w)
	'Method uniform4iv:Void(location:WebGLUniformLocation,v:Int32Array)
	Method uniform4iv:Void(location:WebGLUniformLocation,v[])
	'Method uniformMatrix2fv:Void(location:WebGLUniformLocation,transpose?,value:Float32Array)
	Method uniformMatrix2fv:Void(location:WebGLUniformLocation,transpose?,value#[])
	'Method uniformMatrix3fv:Void(location:WebGLUniformLocation,transpose?,value:Float32Array)
	Method uniformMatrix3fv:Void(location:WebGLUniformLocation,transpose?,value#[])
	'Method uniformMatrix4fv:Void(location:WebGLUniformLocation,transpose?,value:Float32Array)
	Method uniformMatrix4fv:Void(location:WebGLUniformLocation,transpose?,value#[])
	Method useProgram:Void(program:WebGLProgram)
	Method validateProgram:Void(program:WebGLProgram)
	Method vertexAttrib1f:Void(indx,x#)
	'Method vertexAttrib1fv:Void(indx,values:Float32Array)
	Method vertexAttrib1fv:Void(indx,values#[])
	Method vertexAttrib2f:Void(indx,x#,y#)
	'Method vertexAttrib2fv:Void(indx,values:Float32Array)
	Method vertexAttrib2fv:Void(indx,values#[])
	Method vertexAttrib3f:Void(indx,x#,y#,z#)
	'Method vertexAttrib3fv:Void(indx,values:Float32Array)
	Method vertexAttrib3fv:Void(indx,values#[])
	Method vertexAttrib4f:Void(indx,x#,y#,z#,w#)
	'Method vertexAttrib4fv:Void(indx,values:Float32Array)
	Method vertexAttrib4fv:Void(indx,values#[])
	Method vertexAttribPointer:Void(indx,size,type,normalized?,stride,offset)
	Method viewport:Void(x,y,width,height)
End
