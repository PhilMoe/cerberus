
#If TARGET<>"glfw" And TARGET<>"android" And TARGET<>"ios"
#Error "The opengl module is not avaiable for the ${TARGET} target."
#Endif

#OPENGL_GLES20_ENABLED=False
#OPENGL_DEPTH_BUFFER_ENABLED=True

Import monkeytarget

Import brl.databuffer

#If TARGET="glfw"
#If HOST="winnt"
#OPENGL_INIT_EXTENSIONS=True
#Endif
Import "native/gles11.glfw.cpp"
#Elseif TARGET="android"
Import "native/gles11.android.java"
#Elseif TARGET="ios"
Import "native/gles11.ios.cpp"
#Endif

'${CONST_DECLS}
Const GL_DEPTH_BUFFER_BIT              =$00000100
Const GL_STENCIL_BUFFER_BIT            =$00000400
Const GL_COLOR_BUFFER_BIT              =$00004000
Const GL_FALSE                         =0
Const GL_TRUE                          =1
Const GL_POINTS                        =$0000
Const GL_LINES                         =$0001
Const GL_LINE_LOOP                     =$0002
Const GL_LINE_STRIP                    =$0003
Const GL_TRIANGLES                     =$0004
Const GL_TRIANGLE_STRIP                =$0005
Const GL_TRIANGLE_FAN                  =$0006
Const GL_NEVER                         =$0200
Const GL_LESS                          =$0201
Const GL_EQUAL                         =$0202
Const GL_LEQUAL                        =$0203
Const GL_GREATER                       =$0204
Const GL_NOTEQUAL                      =$0205
Const GL_GEQUAL                        =$0206
Const GL_ALWAYS                        =$0207
Const GL_ZERO                          =0
Const GL_ONE                           =1
Const GL_SRC_COLOR                     =$0300
Const GL_ONE_MINUS_SRC_COLOR           =$0301
Const GL_SRC_ALPHA                     =$0302
Const GL_ONE_MINUS_SRC_ALPHA           =$0303
Const GL_DST_ALPHA                     =$0304
Const GL_ONE_MINUS_DST_ALPHA           =$0305
Const GL_DST_COLOR                     =$0306
Const GL_ONE_MINUS_DST_COLOR           =$0307
Const GL_SRC_ALPHA_SATURATE            =$0308
Const GL_CLIP_PLANE0                   =$3000
Const GL_CLIP_PLANE1                   =$3001
Const GL_CLIP_PLANE2                   =$3002
Const GL_CLIP_PLANE3                   =$3003
Const GL_CLIP_PLANE4                   =$3004
Const GL_CLIP_PLANE5                   =$3005
Const GL_FRONT                         =$0404
Const GL_BACK                          =$0405
Const GL_FRONT_AND_BACK                =$0408
Const GL_FOG                           =$0B60
Const GL_LIGHTING                      =$0B50
Const GL_TEXTURE_2D                    =$0DE1
Const GL_CULL_FACE                     =$0B44
Const GL_ALPHA_TEST                    =$0BC0
Const GL_BLEND                         =$0BE2
Const GL_COLOR_LOGIC_OP                =$0BF2
Const GL_DITHER                        =$0BD0
Const GL_STENCIL_TEST                  =$0B90
Const GL_DEPTH_TEST                    =$0B71
Const GL_POINT_SMOOTH                  =$0B10
Const GL_LINE_SMOOTH                   =$0B20
Const GL_COLOR_MATERIAL                =$0B57
Const GL_NORMALIZE                     =$0BA1
Const GL_RESCALE_NORMAL                =$803A
Const GL_VERTEX_ARRAY                  =$8074
Const GL_NORMAL_ARRAY                  =$8075
Const GL_COLOR_ARRAY                   =$8076
Const GL_TEXTURE_COORD_ARRAY           =$8078
Const GL_MULTISAMPLE                   =$809D
Const GL_SAMPLE_ALPHA_TO_COVERAGE      =$809E
Const GL_SAMPLE_ALPHA_TO_ONE           =$809F
Const GL_SAMPLE_COVERAGE               =$80A0
Const GL_NO_ERROR                      =0
Const GL_INVALID_ENUM                  =$0500
Const GL_INVALID_VALUE                 =$0501
Const GL_INVALID_OPERATION             =$0502
Const GL_STACK_OVERFLOW                =$0503
Const GL_STACK_UNDERFLOW               =$0504
Const GL_OUT_OF_MEMORY                 =$0505
Const GL_EXP                           =$0800
Const GL_EXP2                          =$0801
Const GL_FOG_DENSITY                   =$0B62
Const GL_FOG_START                     =$0B63
Const GL_FOG_END                       =$0B64
Const GL_FOG_MODE                      =$0B65
Const GL_FOG_COLOR                     =$0B66
Const GL_CW                            =$0900
Const GL_CCW                           =$0901
Const GL_CURRENT_COLOR                 =$0B00
Const GL_CURRENT_NORMAL                =$0B02
Const GL_CURRENT_TEXTURE_COORDS        =$0B03
Const GL_POINT_SIZE                    =$0B11
Const GL_POINT_SIZE_MIN                =$8126
Const GL_POINT_SIZE_MAX                =$8127
Const GL_POINT_FADE_THRESHOLD_SIZE     =$8128
Const GL_POINT_DISTANCE_ATTENUATION    =$8129
Const GL_SMOOTH_POINT_SIZE_RANGE       =$0B12
Const GL_LINE_WIDTH                    =$0B21
Const GL_SMOOTH_LINE_WIDTH_RANGE       =$0B22
Const GL_ALIASED_POINT_SIZE_RANGE      =$846D
Const GL_ALIASED_LINE_WIDTH_RANGE      =$846E
Const GL_CULL_FACE_MODE                =$0B45
Const GL_FRONT_FACE                    =$0B46
Const GL_SHADE_MODEL                   =$0B54
Const GL_DEPTH_RANGE                   =$0B70
Const GL_DEPTH_WRITEMASK               =$0B72
Const GL_DEPTH_CLEAR_VALUE             =$0B73
Const GL_DEPTH_FUNC                    =$0B74
Const GL_STENCIL_CLEAR_VALUE           =$0B91
Const GL_STENCIL_FUNC                  =$0B92
Const GL_STENCIL_VALUE_MASK            =$0B93
Const GL_STENCIL_FAIL                  =$0B94
Const GL_STENCIL_PASS_DEPTH_FAIL       =$0B95
Const GL_STENCIL_PASS_DEPTH_PASS       =$0B96
Const GL_STENCIL_REF                   =$0B97
Const GL_STENCIL_WRITEMASK             =$0B98
Const GL_MATRIX_MODE                   =$0BA0
Const GL_VIEWPORT                      =$0BA2
Const GL_MODELVIEW_STACK_DEPTH         =$0BA3
Const GL_PROJECTION_STACK_DEPTH        =$0BA4
Const GL_TEXTURE_STACK_DEPTH           =$0BA5
Const GL_MODELVIEW_MATRIX              =$0BA6
Const GL_PROJECTION_MATRIX             =$0BA7
Const GL_TEXTURE_MATRIX                =$0BA8
Const GL_ALPHA_TEST_FUNC               =$0BC1
Const GL_ALPHA_TEST_REF                =$0BC2
Const GL_BLEND_DST                     =$0BE0
Const GL_BLEND_SRC                     =$0BE1
Const GL_LOGIC_OP_MODE                 =$0BF0
Const GL_SCISSOR_BOX                   =$0C10
Const GL_SCISSOR_TEST                  =$0C11
Const GL_COLOR_CLEAR_VALUE             =$0C22
Const GL_COLOR_WRITEMASK               =$0C23
Const GL_MAX_LIGHTS                    =$0D31
Const GL_MAX_CLIP_PLANES               =$0D32
Const GL_MAX_TEXTURE_SIZE              =$0D33
Const GL_MAX_MODELVIEW_STACK_DEPTH     =$0D36
Const GL_MAX_PROJECTION_STACK_DEPTH    =$0D38
Const GL_MAX_TEXTURE_STACK_DEPTH       =$0D39
Const GL_MAX_VIEWPORT_DIMS             =$0D3A
Const GL_MAX_TEXTURE_UNITS             =$84E2
Const GL_SUBPIXEL_BITS                 =$0D50
Const GL_RED_BITS                      =$0D52
Const GL_GREEN_BITS                    =$0D53
Const GL_BLUE_BITS                     =$0D54
Const GL_ALPHA_BITS                    =$0D55
Const GL_DEPTH_BITS                    =$0D56
Const GL_STENCIL_BITS                  =$0D57
Const GL_POLYGON_OFFSET_UNITS          =$2A00
Const GL_POLYGON_OFFSET_FILL           =$8037
Const GL_POLYGON_OFFSET_FACTOR         =$8038
Const GL_TEXTURE_BINDING_2D            =$8069
Const GL_VERTEX_ARRAY_SIZE             =$807A
Const GL_VERTEX_ARRAY_TYPE             =$807B
Const GL_VERTEX_ARRAY_STRIDE           =$807C
Const GL_NORMAL_ARRAY_TYPE             =$807E
Const GL_NORMAL_ARRAY_STRIDE           =$807F
Const GL_COLOR_ARRAY_SIZE              =$8081
Const GL_COLOR_ARRAY_TYPE              =$8082
Const GL_COLOR_ARRAY_STRIDE            =$8083
Const GL_TEXTURE_COORD_ARRAY_SIZE      =$8088
Const GL_TEXTURE_COORD_ARRAY_TYPE      =$8089
Const GL_TEXTURE_COORD_ARRAY_STRIDE    =$808A
Const GL_VERTEX_ARRAY_POINTER          =$808E
Const GL_NORMAL_ARRAY_POINTER          =$808F
Const GL_COLOR_ARRAY_POINTER           =$8090
Const GL_TEXTURE_COORD_ARRAY_POINTER   =$8092
Const GL_SAMPLE_BUFFERS                =$80A8
Const GL_SAMPLES                       =$80A9
Const GL_SAMPLE_COVERAGE_VALUE         =$80AA
Const GL_SAMPLE_COVERAGE_INVERT        =$80AB
Const GL_NUM_COMPRESSED_TEXTURE_FORMATS=$86A2
Const GL_COMPRESSED_TEXTURE_FORMATS    =$86A3
Const GL_DONT_CARE                     =$1100
Const GL_FASTEST                       =$1101
Const GL_NICEST                        =$1102
Const GL_PERSPECTIVE_CORRECTION_HINT   =$0C50
Const GL_POINT_SMOOTH_HINT             =$0C51
Const GL_LINE_SMOOTH_HINT              =$0C52
Const GL_FOG_HINT                      =$0C54
Const GL_GENERATE_MIPMAP_HINT          =$8192
Const GL_LIGHT_MODEL_AMBIENT           =$0B53
Const GL_LIGHT_MODEL_TWO_SIDE          =$0B52
Const GL_AMBIENT                       =$1200
Const GL_DIFFUSE                       =$1201
Const GL_SPECULAR                      =$1202
Const GL_POSITION                      =$1203
Const GL_SPOT_DIRECTION                =$1204
Const GL_SPOT_EXPONENT                 =$1205
Const GL_SPOT_CUTOFF                   =$1206
Const GL_CONSTANT_ATTENUATION          =$1207
Const GL_LINEAR_ATTENUATION            =$1208
Const GL_QUADRATIC_ATTENUATION         =$1209
Const GL_BYTE                          =$1400
Const GL_UNSIGNED_BYTE                 =$1401
Const GL_SHORT                         =$1402
Const GL_UNSIGNED_SHORT                =$1403
Const GL_FLOAT                         =$1406
Const GL_FIXED                         =$140C
Const GL_CLEAR                         =$1500
Const GL_AND                           =$1501
Const GL_AND_REVERSE                   =$1502
Const GL_COPY                          =$1503
Const GL_AND_INVERTED                  =$1504
Const GL_NOOP                          =$1505
Const GL_XOR                           =$1506
Const GL_OR                            =$1507
Const GL_NOR                           =$1508
Const GL_EQUIV                         =$1509
Const GL_INVERT                        =$150A
Const GL_OR_REVERSE                    =$150B
Const GL_COPY_INVERTED                 =$150C
Const GL_OR_INVERTED                   =$150D
Const GL_NAND                          =$150E
Const GL_SET                           =$150F
Const GL_EMISSION                      =$1600
Const GL_SHININESS                     =$1601
Const GL_AMBIENT_AND_DIFFUSE           =$1602
Const GL_MODELVIEW                     =$1700
Const GL_PROJECTION                    =$1701
Const GL_TEXTURE                       =$1702
Const GL_ALPHA                         =$1906
Const GL_RGB                           =$1907
Const GL_RGBA                          =$1908
Const GL_LUMINANCE                     =$1909
Const GL_LUMINANCE_ALPHA               =$190A
Const GL_UNPACK_ALIGNMENT              =$0CF5
Const GL_PACK_ALIGNMENT                =$0D05
Const GL_UNSIGNED_SHORT_4_4_4_4        =$8033
Const GL_UNSIGNED_SHORT_5_5_5_1        =$8034
Const GL_UNSIGNED_SHORT_5_6_5          =$8363
Const GL_FLAT                          =$1D00
Const GL_SMOOTH                        =$1D01
Const GL_KEEP                          =$1E00
Const GL_REPLACE                       =$1E01
Const GL_INCR                          =$1E02
Const GL_DECR                          =$1E03
Const GL_VENDOR                        =$1F00
Const GL_RENDERER                      =$1F01
Const GL_VERSION                       =$1F02
Const GL_EXTENSIONS                    =$1F03
Const GL_MODULATE                      =$2100
Const GL_DECAL                         =$2101
Const GL_ADD                           =$0104
Const GL_TEXTURE_ENV_MODE              =$2200
Const GL_TEXTURE_ENV_COLOR             =$2201
Const GL_TEXTURE_ENV                   =$2300
Const GL_NEAREST                       =$2600
Const GL_LINEAR                        =$2601
Const GL_NEAREST_MIPMAP_NEAREST        =$2700
Const GL_LINEAR_MIPMAP_NEAREST         =$2701
Const GL_NEAREST_MIPMAP_LINEAR         =$2702
Const GL_LINEAR_MIPMAP_LINEAR          =$2703
Const GL_TEXTURE_MAG_FILTER            =$2800
Const GL_TEXTURE_MIN_FILTER            =$2801
Const GL_TEXTURE_WRAP_S                =$2802
Const GL_TEXTURE_WRAP_T                =$2803
Const GL_GENERATE_MIPMAP               =$8191
Const GL_TEXTURE0                      =$84C0
Const GL_TEXTURE1                      =$84C1
Const GL_TEXTURE2                      =$84C2
Const GL_TEXTURE3                      =$84C3
Const GL_TEXTURE4                      =$84C4
Const GL_TEXTURE5                      =$84C5
Const GL_TEXTURE6                      =$84C6
Const GL_TEXTURE7                      =$84C7
Const GL_TEXTURE8                      =$84C8
Const GL_TEXTURE9                      =$84C9
Const GL_TEXTURE10                     =$84CA
Const GL_TEXTURE11                     =$84CB
Const GL_TEXTURE12                     =$84CC
Const GL_TEXTURE13                     =$84CD
Const GL_TEXTURE14                     =$84CE
Const GL_TEXTURE15                     =$84CF
Const GL_TEXTURE16                     =$84D0
Const GL_TEXTURE17                     =$84D1
Const GL_TEXTURE18                     =$84D2
Const GL_TEXTURE19                     =$84D3
Const GL_TEXTURE20                     =$84D4
Const GL_TEXTURE21                     =$84D5
Const GL_TEXTURE22                     =$84D6
Const GL_TEXTURE23                     =$84D7
Const GL_TEXTURE24                     =$84D8
Const GL_TEXTURE25                     =$84D9
Const GL_TEXTURE26                     =$84DA
Const GL_TEXTURE27                     =$84DB
Const GL_TEXTURE28                     =$84DC
Const GL_TEXTURE29                     =$84DD
Const GL_TEXTURE30                     =$84DE
Const GL_TEXTURE31                     =$84DF
Const GL_ACTIVE_TEXTURE                =$84E0
Const GL_CLIENT_ACTIVE_TEXTURE         =$84E1
Const GL_REPEAT                        =$2901
Const GL_CLAMP_TO_EDGE                 =$812F
Const GL_LIGHT0                        =$4000
Const GL_LIGHT1                        =$4001
Const GL_LIGHT2                        =$4002
Const GL_LIGHT3                        =$4003
Const GL_LIGHT4                        =$4004
Const GL_LIGHT5                        =$4005
Const GL_LIGHT6                        =$4006
Const GL_LIGHT7                        =$4007
Const GL_ARRAY_BUFFER                  =$8892
Const GL_ELEMENT_ARRAY_BUFFER          =$8893
Const GL_ARRAY_BUFFER_BINDING              =$8894
Const GL_ELEMENT_ARRAY_BUFFER_BINDING      =$8895
Const GL_VERTEX_ARRAY_BUFFER_BINDING       =$8896
Const GL_NORMAL_ARRAY_BUFFER_BINDING       =$8897
Const GL_COLOR_ARRAY_BUFFER_BINDING        =$8898
Const GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING=$889A
Const GL_STATIC_DRAW                   =$88E4
Const GL_DYNAMIC_DRAW                  =$88E8
Const GL_BUFFER_SIZE                   =$8764
Const GL_BUFFER_USAGE                  =$8765
Const GL_SUBTRACT                      =$84E7
Const GL_COMBINE                       =$8570
Const GL_COMBINE_RGB                   =$8571
Const GL_COMBINE_ALPHA                 =$8572
Const GL_RGB_SCALE                     =$8573
Const GL_ADD_SIGNED                    =$8574
Const GL_INTERPOLATE                   =$8575
Const GL_CONSTANT                      =$8576
Const GL_PRIMARY_COLOR                 =$8577
Const GL_PREVIOUS                      =$8578
Const GL_OPERAND0_RGB                  =$8590
Const GL_OPERAND1_RGB                  =$8591
Const GL_OPERAND2_RGB                  =$8592
Const GL_OPERAND0_ALPHA                =$8598
Const GL_OPERAND1_ALPHA                =$8599
Const GL_OPERAND2_ALPHA                =$859A
Const GL_ALPHA_SCALE                   =$0D1C
Const GL_SRC0_RGB                      =$8580
Const GL_SRC1_RGB                      =$8581
Const GL_SRC2_RGB                      =$8582
Const GL_SRC0_ALPHA                    =$8588
Const GL_SRC1_ALPHA                    =$8589
Const GL_SRC2_ALPHA                    =$858A
Const GL_DOT3_RGB                      =$86AE
Const GL_DOT3_RGBA                     =$86AF
'${END}

Extern

#If TARGET="glfw"

Function BBLoadImageData:BBDataBuffer( buf:BBDataBuffer,path$,info[]=[] )="BBLoadImageData"

'${GLFW_DECLS}
Function glBindBuffer:Void( target,buffer )
Function glIsBuffer:Bool( buffer )
Function glGenTextures:Void( n,textures[],offset=0 )="_glGenTextures"
Function glDeleteTextures:Void( n,textures[],offset=0 )="_glDeleteTextures"
Function glGenBuffers:Void( n,buffers[],offset=0 )="_glGenBuffers"
Function glDeleteBuffers:Void( n,buffers[],offset=0 )="_glDeleteBuffers"
Function glClipPlanef:Void( plane,equation#[],offset=0 )="_glClipPlanef"
Function glFogfv:Void( pname,params#[],offset=0 )="_glFogfv"
Function glGetBufferParameteriv:Void( target,pname,params[],offset=0 )="_glGetBufferParameteriv"
Function glGetClipPlanef:Void( plane,equation#[],offset=0 )="_glGetClipPlanef"
Function glGetFloatv:Void( pname,params#[],offset=0 )="_glGetFloatv"
Function glGetLightfv:Void( light,pname,params#[],offset=0 )="_glGetLightfv"
Function glGetMaterialfv:Void( face,pname,params#[],offset=0 )="_glGetMaterialfv"
Function glGetTexEnvfv:Void( env,pname,params#[],offset=0 )="_glGetTexEnvfv"
Function glGetTexParameterfv:Void( target,pname,params#[],offset=0 )="_glGetTexParameterfv"
Function glLightfv:Void( light,pname,params#[],offset=0 )="_glLightfv"
Function glLightModelfv:Void( pname,params#[],offset=0 )="_glLightModelfv"
Function glLoadMatrixf:Void( m#[],offset=0 )="_glLoadMatrixf"
Function glMaterialfv:Void( face,pname,params#[],offset=0 )="_glMaterialfv"
Function glMultMatrixf:Void( m#[],offset=0 )="_glMultMatrixf"
Function glTexEnvfv:Void( target,pname,params#[],offset=0 )="_glTexEnvfv"
Function glTexParameterfv:Void( target,pname,params#[],offset=0 )="_glTexParameterfv"
Function glGetIntegerv:Void( pname,params[],offset=0 )="_glGetIntegerv"
Function glGetString:String( name )="_glGetString"
Function glGetTexEnviv:Void( env,pname,params[],offset=0 )="_glGetTexEnviv"
Function glGetTexParameteriv:Void( target,pname,params[],offset=0 )="_glGetTexParameteriv"
Function glTexEnviv:Void( target,pname,params[],offset=0 )="_glTexEnviv"
Function glTexParameteriv:Void( target,pname,params[],offset=0 )="_glTexParameteriv"
Function glVertexPointer:Void( size,type,stride,pointer:DataBuffer )="_glVertexPointer"
Function glColorPointer:Void( size,type,stride,pointer:DataBuffer )="_glColorPointer"
Function glNormalPointer:Void( type,stride,pointer:DataBuffer )="_glNormalPointer"
Function glTexCoordPointer:Void( size,type,stride,pointer:DataBuffer )="_glTexCoordPointer"
Function glDrawElements:Void( mode,count,type,indices:DataBuffer )="_glDrawElements"
Function glBufferData:Void( target,size,data:DataBuffer,usage )="_glBufferData"
Function glBufferSubData:Void( target,offset,size,data:DataBuffer )="_glBufferSubData"
Function glTexImage2D:Void( target,level,internalformat,width,height,border,format,type,pixels:DataBuffer )="_glTexImage2D"
Function glTexSubImage2D:Void( target,level,xoffset,yoffset,width,height,format,type,pixels:DataBuffer )="_glTexSubImage2D"
Function glCompressedTexImage2D:Void( target,level,internalformat,width,height,border,imageSize,Data:DataBuffer )="_glCompressedTexImage2D"
Function glCompressedTexSubImage2D:Void( target,level,xoffset,yoffset,width,height,format,imageSize,data:DataBuffer )="_glCompressedTexSubImage2D"
Function glReadPixels:Void( x,y,width,height,format,type,pixels:DataBuffer )="_glReadPixels"
Function glVertexPointer:Void( size,type,stride,offset )="_glVertexPointer"
Function glColorPointer:Void( size,type,stride,offset )="_glColorPointer"
Function glNormalPointer:Void( type,stride,offset )="_glNormalPointer"
Function glTexCoordPointer:Void( size,type,stride,offset )="_glTexCoordPointer"
Function glDrawElements:Void( mode,count,type,offset )="_glDrawElements"
Function glFrustumf:Void( left#,right#,bottom#,top#,zNear#,zFar# )="_glFrustumf"
Function glOrthof:Void( left#,right#,bottom#,top#,zNear#,zFar# )="_glOrthof"
Function glClearDepthf:Void( depth# )="_glClearDepthf"
Function glDepthRangef:Void( zNear#,zFar# )="_glDepthRangef"
Function glAlphaFunc:Void( func,ref# )
Function glClearColor:Void( red#,green#,blue#,alpha# )
Function glColor4f:Void( red#,green#,blue#,alpha# )
Function glFogf:Void( pname,param# )
Function glLightModelf:Void( pname,param# )
Function glLightf:Void( light,pname,param# )
Function glLineWidth:Void( width# )
Function glMaterialf:Void( face,pname,param# )
Function glMultiTexCoord4f:Void( target,s#,t#,r#,q# )
Function glNormal3f:Void( nx#,ny#,nz# )
Function glPointParameterf:Void( pname,param# )
Function glPointSize:Void( size# )
Function glPolygonOffset:Void( factor#,units# )
Function glRotatef:Void( angle#,x#,y#,z# )
Function glScalef:Void( x#,y#,z# )
Function glTexEnvf:Void( target,pname,param# )
Function glTexParameterf:Void( target,pname,param# )
Function glTranslatef:Void( x#,y#,z# )
Function glActiveTexture:Void( texture )
Function glBindTexture:Void( target,texture )
Function glBlendFunc:Void( sfactor,dfactor )
Function glClear:Void( mask )
Function glClearStencil:Void( s )
Function glClientActiveTexture:Void( texture )
Function glColor4ub:Void( red,green,blue,alpha )
Function glColorMask:Void( red?,green?,blue?,alpha? )
Function glCopyTexImage2D:Void( target,level,internalformat,x,y,width,height,border )
Function glCopyTexSubImage2D:Void( target,level,xoffset,yoffset,x,y,width,height )
Function glCullFace:Void( mode )
Function glDepthFunc:Void( func )
Function glDepthMask:Void( flag? )
Function glDisable:Void( cap )
Function glDisableClientState:Void( arry )
Function glDrawArrays:Void( mode,first,count )
Function glEnable:Void( cap )
Function glEnableClientState:Void( arry )
Function glFinish:Void()
Function glFlush:Void()
Function glFrontFace:Void( mode )
Function glGetError:Int()
Function glHint:Void( target,mode )
Function glIsEnabled:Bool( cap )
Function glIsTexture:Bool( texture )
Function glLoadIdentity:Void()
Function glLogicOp:Void( opcode )
Function glMatrixMode:Void( mode )
Function glPixelStorei:Void( pname,param )
Function glPopMatrix:Void()
Function glPushMatrix:Void()
Function glSampleCoverage:Void( value#,invert? )
Function glScissor:Void( x,y,width,height )
Function glShadeModel:Void( mode )
Function glStencilFunc:Void( func,ref,mask )
Function glStencilMask:Void( mask )
Function glStencilOp:Void( fail,zfail,zpass )
Function glTexEnvi:Void( target,pname,param )
Function glTexParameteri:Void( target,pname,param )
Function glViewport:Void( x,y,width,height )
'${END}

#Elseif TARGET="android"

Function BBLoadImageData:BBDataBuffer( buf:BBDataBuffer,path$,info[]=[] )="bb_opengl_gles11.LoadImageData"

'${ANDROID_DECLS}
Function glBindBuffer:Void( target,buffer )="GLES11.glBindBuffer"
Function glIsBuffer:Bool( buffer )="GLES11.glIsBuffer"
Function glGenTextures:Void( n,textures[],offset=0 )="GLES11.glGenTextures"
Function glDeleteTextures:Void( n,textures[],offset=0 )="GLES11.glDeleteTextures"
Function glGenBuffers:Void( n,buffers[],offset=0 )="GLES11.glGenBuffers"
Function glDeleteBuffers:Void( n,buffers[],offset=0 )="GLES11.glDeleteBuffers"
Function glClipPlanef:Void( plane,equation#[],offset=0 )="GLES11.glClipPlanef"
Function glFogfv:Void( pname,params#[],offset=0 )="GLES11.glFogfv"
Function glGetBufferParameteriv:Void( target,pname,params[],offset=0 )="GLES11.glGetBufferParameteriv"
Function glGetClipPlanef:Void( plane,equation#[],offset=0 )="GLES11.glGetClipPlanef"
Function glGetFloatv:Void( pname,params#[],offset=0 )="GLES11.glGetFloatv"
Function glGetLightfv:Void( light,pname,params#[],offset=0 )="GLES11.glGetLightfv"
Function glGetMaterialfv:Void( face,pname,params#[],offset=0 )="GLES11.glGetMaterialfv"
Function glGetTexEnvfv:Void( env,pname,params#[],offset=0 )="GLES11.glGetTexEnvfv"
Function glGetTexParameterfv:Void( target,pname,params#[],offset=0 )="GLES11.glGetTexParameterfv"
Function glLightfv:Void( light,pname,params#[],offset=0 )="GLES11.glLightfv"
Function glLightModelfv:Void( pname,params#[],offset=0 )="GLES11.glLightModelfv"
Function glLoadMatrixf:Void( m#[],offset=0 )="GLES11.glLoadMatrixf"
Function glMaterialfv:Void( face,pname,params#[],offset=0 )="GLES11.glMaterialfv"
Function glMultMatrixf:Void( m#[],offset=0 )="GLES11.glMultMatrixf"
Function glTexEnvfv:Void( target,pname,params#[],offset=0 )="GLES11.glTexEnvfv"
Function glTexParameterfv:Void( target,pname,params#[],offset=0 )="GLES11.glTexParameterfv"
Function glGetIntegerv:Void( pname,params[],offset=0 )="GLES11.glGetIntegerv"
Function glGetString:String( name )="GLES11.glGetString"
Function glGetTexEnviv:Void( env,pname,params[],offset=0 )="GLES11.glGetTexEnviv"
Function glGetTexParameteriv:Void( target,pname,params[],offset=0 )="GLES11.glGetTexParameteriv"
Function glTexEnviv:Void( target,pname,params[],offset=0 )="GLES11.glTexEnviv"
Function glTexParameteriv:Void( target,pname,params[],offset=0 )="GLES11.glTexParameteriv"
Function glVertexPointer:Void( size,type,stride,pointer:DataBuffer )="bb_opengl_gles11._glVertexPointer"
Function glColorPointer:Void( size,type,stride,pointer:DataBuffer )="bb_opengl_gles11._glColorPointer"
Function glNormalPointer:Void( type,stride,pointer:DataBuffer )="bb_opengl_gles11._glNormalPointer"
Function glTexCoordPointer:Void( size,type,stride,pointer:DataBuffer )="bb_opengl_gles11._glTexCoordPointer"
Function glDrawElements:Void( mode,count,type,indices:DataBuffer )="bb_opengl_gles11._glDrawElements"
Function glBufferData:Void( target,size,data:DataBuffer,usage )="bb_opengl_gles11._glBufferData"
Function glBufferSubData:Void( target,offset,size,data:DataBuffer )="bb_opengl_gles11._glBufferSubData"
Function glTexImage2D:Void( target,level,internalformat,width,height,border,format,type,pixels:DataBuffer )="bb_opengl_gles11._glTexImage2D"
Function glTexSubImage2D:Void( target,level,xoffset,yoffset,width,height,format,type,pixels:DataBuffer )="bb_opengl_gles11._glTexSubImage2D"
Function glCompressedTexImage2D:Void( target,level,internalformat,width,height,border,imageSize,Data:DataBuffer )="bb_opengl_gles11._glCompressedTexImage2D"
Function glCompressedTexSubImage2D:Void( target,level,xoffset,yoffset,width,height,format,imageSize,data:DataBuffer )="bb_opengl_gles11._glCompressedTexSubImage2D"
Function glReadPixels:Void( x,y,width,height,format,type,pixels:DataBuffer )="bb_opengl_gles11._glReadPixels"
Function glVertexPointer:Void( size,type,stride,offset )="GLES11.glVertexPointer"
Function glColorPointer:Void( size,type,stride,offset )="GLES11.glColorPointer"
Function glNormalPointer:Void( type,stride,offset )="GLES11.glNormalPointer"
Function glTexCoordPointer:Void( size,type,stride,offset )="GLES11.glTexCoordPointer"
Function glDrawElements:Void( mode,count,type,offset )="GLES11.glDrawElements"
Function glFrustumf:Void( left#,right#,bottom#,top#,zNear#,zFar# )="GLES11.glFrustumf"
Function glOrthof:Void( left#,right#,bottom#,top#,zNear#,zFar# )="GLES11.glOrthof"
Function glClearDepthf:Void( depth# )="GLES11.glClearDepthf"
Function glDepthRangef:Void( zNear#,zFar# )="GLES11.glDepthRangef"
Function glAlphaFunc:Void( func,ref# )="GLES11.glAlphaFunc"
Function glClearColor:Void( red#,green#,blue#,alpha# )="GLES11.glClearColor"
Function glColor4f:Void( red#,green#,blue#,alpha# )="GLES11.glColor4f"
Function glFogf:Void( pname,param# )="GLES11.glFogf"
Function glLightModelf:Void( pname,param# )="GLES11.glLightModelf"
Function glLightf:Void( light,pname,param# )="GLES11.glLightf"
Function glLineWidth:Void( width# )="GLES11.glLineWidth"
Function glMaterialf:Void( face,pname,param# )="GLES11.glMaterialf"
Function glMultiTexCoord4f:Void( target,s#,t#,r#,q# )="GLES11.glMultiTexCoord4f"
Function glNormal3f:Void( nx#,ny#,nz# )="GLES11.glNormal3f"
Function glPointParameterf:Void( pname,param# )="GLES11.glPointParameterf"
Function glPointSize:Void( size# )="GLES11.glPointSize"
Function glPolygonOffset:Void( factor#,units# )="GLES11.glPolygonOffset"
Function glRotatef:Void( angle#,x#,y#,z# )="GLES11.glRotatef"
Function glScalef:Void( x#,y#,z# )="GLES11.glScalef"
Function glTexEnvf:Void( target,pname,param# )="GLES11.glTexEnvf"
Function glTexParameterf:Void( target,pname,param# )="GLES11.glTexParameterf"
Function glTranslatef:Void( x#,y#,z# )="GLES11.glTranslatef"
Function glActiveTexture:Void( texture )="GLES11.glActiveTexture"
Function glBindTexture:Void( target,texture )="GLES11.glBindTexture"
Function glBlendFunc:Void( sfactor,dfactor )="GLES11.glBlendFunc"
Function glClear:Void( mask )="GLES11.glClear"
Function glClearStencil:Void( s )="GLES11.glClearStencil"
Function glClientActiveTexture:Void( texture )="GLES11.glClientActiveTexture"
Function glColor4ub:Void( red,green,blue,alpha )="GLES11.glColor4ub"
Function glColorMask:Void( red?,green?,blue?,alpha? )="GLES11.glColorMask"
Function glCopyTexImage2D:Void( target,level,internalformat,x,y,width,height,border )="GLES11.glCopyTexImage2D"
Function glCopyTexSubImage2D:Void( target,level,xoffset,yoffset,x,y,width,height )="GLES11.glCopyTexSubImage2D"
Function glCullFace:Void( mode )="GLES11.glCullFace"
Function glDepthFunc:Void( func )="GLES11.glDepthFunc"
Function glDepthMask:Void( flag? )="GLES11.glDepthMask"
Function glDisable:Void( cap )="GLES11.glDisable"
Function glDisableClientState:Void( arry )="GLES11.glDisableClientState"
Function glDrawArrays:Void( mode,first,count )="GLES11.glDrawArrays"
Function glEnable:Void( cap )="GLES11.glEnable"
Function glEnableClientState:Void( arry )="GLES11.glEnableClientState"
Function glFinish:Void()="GLES11.glFinish"
Function glFlush:Void()="GLES11.glFlush"
Function glFrontFace:Void( mode )="GLES11.glFrontFace"
Function glGetError:Int()="GLES11.glGetError"
Function glHint:Void( target,mode )="GLES11.glHint"
Function glIsEnabled:Bool( cap )="GLES11.glIsEnabled"
Function glIsTexture:Bool( texture )="GLES11.glIsTexture"
Function glLoadIdentity:Void()="GLES11.glLoadIdentity"
Function glLogicOp:Void( opcode )="GLES11.glLogicOp"
Function glMatrixMode:Void( mode )="GLES11.glMatrixMode"
Function glPixelStorei:Void( pname,param )="GLES11.glPixelStorei"
Function glPopMatrix:Void()="GLES11.glPopMatrix"
Function glPushMatrix:Void()="GLES11.glPushMatrix"
Function glSampleCoverage:Void( value#,invert? )="GLES11.glSampleCoverage"
Function glScissor:Void( x,y,width,height )="GLES11.glScissor"
Function glShadeModel:Void( mode )="GLES11.glShadeModel"
Function glStencilFunc:Void( func,ref,mask )="GLES11.glStencilFunc"
Function glStencilMask:Void( mask )="GLES11.glStencilMask"
Function glStencilOp:Void( fail,zfail,zpass )="GLES11.glStencilOp"
Function glTexEnvi:Void( target,pname,param )="GLES11.glTexEnvi"
Function glTexParameteri:Void( target,pname,param )="GLES11.glTexParameteri"
Function glViewport:Void( x,y,width,height )="GLES11.glViewport"
'${END}

#ElseIf TARGET="ios"

Function BBLoadImageData:BBDataBuffer( buf:BBDataBuffer,path$,info[]=[] )="BBLoadImageData"

'${IOS_DECLS}
Function glBindBuffer:Void( target,buffer )
Function glIsBuffer:Bool( buffer )
Function glGenTextures:Void( n,textures[],offset=0 )="_glGenTextures"
Function glDeleteTextures:Void( n,textures[],offset=0 )="_glDeleteTextures"
Function glGenBuffers:Void( n,buffers[],offset=0 )="_glGenBuffers"
Function glDeleteBuffers:Void( n,buffers[],offset=0 )="_glDeleteBuffers"
Function glClipPlanef:Void( plane,equation#[],offset=0 )="_glClipPlanef"
Function glFogfv:Void( pname,params#[],offset=0 )="_glFogfv"
Function glGetBufferParameteriv:Void( target,pname,params[],offset=0 )="_glGetBufferParameteriv"
Function glGetClipPlanef:Void( plane,equation#[],offset=0 )="_glGetClipPlanef"
Function glGetFloatv:Void( pname,params#[],offset=0 )="_glGetFloatv"
Function glGetLightfv:Void( light,pname,params#[],offset=0 )="_glGetLightfv"
Function glGetMaterialfv:Void( face,pname,params#[],offset=0 )="_glGetMaterialfv"
Function glGetTexEnvfv:Void( env,pname,params#[],offset=0 )="_glGetTexEnvfv"
Function glGetTexParameterfv:Void( target,pname,params#[],offset=0 )="_glGetTexParameterfv"
Function glLightfv:Void( light,pname,params#[],offset=0 )="_glLightfv"
Function glLightModelfv:Void( pname,params#[],offset=0 )="_glLightModelfv"
Function glLoadMatrixf:Void( m#[],offset=0 )="_glLoadMatrixf"
Function glMaterialfv:Void( face,pname,params#[],offset=0 )="_glMaterialfv"
Function glMultMatrixf:Void( m#[],offset=0 )="_glMultMatrixf"
Function glTexEnvfv:Void( target,pname,params#[],offset=0 )="_glTexEnvfv"
Function glTexParameterfv:Void( target,pname,params#[],offset=0 )="_glTexParameterfv"
Function glGetIntegerv:Void( pname,params[],offset=0 )="_glGetIntegerv"
Function glGetString:String( name )="_glGetString"
Function glGetTexEnviv:Void( env,pname,params[],offset=0 )="_glGetTexEnviv"
Function glGetTexParameteriv:Void( target,pname,params[],offset=0 )="_glGetTexParameteriv"
Function glTexEnviv:Void( target,pname,params[],offset=0 )="_glTexEnviv"
Function glTexParameteriv:Void( target,pname,params[],offset=0 )="_glTexParameteriv"
Function glVertexPointer:Void( size,type,stride,pointer:DataBuffer )="_glVertexPointer"
Function glColorPointer:Void( size,type,stride,pointer:DataBuffer )="_glColorPointer"
Function glNormalPointer:Void( type,stride,pointer:DataBuffer )="_glNormalPointer"
Function glTexCoordPointer:Void( size,type,stride,pointer:DataBuffer )="_glTexCoordPointer"
Function glDrawElements:Void( mode,count,type,indices:DataBuffer )="_glDrawElements"
Function glBufferData:Void( target,size,data:DataBuffer,usage )="_glBufferData"
Function glBufferSubData:Void( target,offset,size,data:DataBuffer )="_glBufferSubData"
Function glTexImage2D:Void( target,level,internalformat,width,height,border,format,type,pixels:DataBuffer )="_glTexImage2D"
Function glTexSubImage2D:Void( target,level,xoffset,yoffset,width,height,format,type,pixels:DataBuffer )="_glTexSubImage2D"
Function glCompressedTexImage2D:Void( target,level,internalformat,width,height,border,imageSize,Data:DataBuffer )="_glCompressedTexImage2D"
Function glCompressedTexSubImage2D:Void( target,level,xoffset,yoffset,width,height,format,imageSize,data:DataBuffer )="_glCompressedTexSubImage2D"
Function glReadPixels:Void( x,y,width,height,format,type,pixels:DataBuffer )="_glReadPixels"
Function glVertexPointer:Void( size,type,stride,offset )="_glVertexPointer"
Function glColorPointer:Void( size,type,stride,offset )="_glColorPointer"
Function glNormalPointer:Void( type,stride,offset )="_glNormalPointer"
Function glTexCoordPointer:Void( size,type,stride,offset )="_glTexCoordPointer"
Function glDrawElements:Void( mode,count,type,offset )="_glDrawElements"
Function glFrustumf:Void( left#,right#,bottom#,top#,zNear#,zFar# )
Function glOrthof:Void( left#,right#,bottom#,top#,zNear#,zFar# )
Function glClearDepthf:Void( depth# )
Function glDepthRangef:Void( zNear#,zFar# )
Function glAlphaFunc:Void( func,ref# )
Function glClearColor:Void( red#,green#,blue#,alpha# )
Function glColor4f:Void( red#,green#,blue#,alpha# )
Function glFogf:Void( pname,param# )
Function glLightModelf:Void( pname,param# )
Function glLightf:Void( light,pname,param# )
Function glLineWidth:Void( width# )
Function glMaterialf:Void( face,pname,param# )
Function glMultiTexCoord4f:Void( target,s#,t#,r#,q# )
Function glNormal3f:Void( nx#,ny#,nz# )
Function glPointParameterf:Void( pname,param# )
Function glPointSize:Void( size# )
Function glPolygonOffset:Void( factor#,units# )
Function glRotatef:Void( angle#,x#,y#,z# )
Function glScalef:Void( x#,y#,z# )
Function glTexEnvf:Void( target,pname,param# )
Function glTexParameterf:Void( target,pname,param# )
Function glTranslatef:Void( x#,y#,z# )
Function glActiveTexture:Void( texture )
Function glBindTexture:Void( target,texture )
Function glBlendFunc:Void( sfactor,dfactor )
Function glClear:Void( mask )
Function glClearStencil:Void( s )
Function glClientActiveTexture:Void( texture )
Function glColor4ub:Void( red,green,blue,alpha )
Function glColorMask:Void( red?,green?,blue?,alpha? )
Function glCopyTexImage2D:Void( target,level,internalformat,x,y,width,height,border )
Function glCopyTexSubImage2D:Void( target,level,xoffset,yoffset,x,y,width,height )
Function glCullFace:Void( mode )
Function glDepthFunc:Void( func )
Function glDepthMask:Void( flag? )
Function glDisable:Void( cap )
Function glDisableClientState:Void( arry )
Function glDrawArrays:Void( mode,first,count )
Function glEnable:Void( cap )
Function glEnableClientState:Void( arry )
Function glFinish:Void()
Function glFlush:Void()
Function glFrontFace:Void( mode )
Function glGetError:Int()
Function glHint:Void( target,mode )
Function glIsEnabled:Bool( cap )
Function glIsTexture:Bool( texture )
Function glLoadIdentity:Void()
Function glLogicOp:Void( opcode )
Function glMatrixMode:Void( mode )
Function glPixelStorei:Void( pname,param )
Function glPopMatrix:Void()
Function glPushMatrix:Void()
Function glSampleCoverage:Void( value#,invert? )
Function glScissor:Void( x,y,width,height )
Function glShadeModel:Void( mode )
Function glStencilFunc:Void( func,ref,mask )
Function glStencilMask:Void( mask )
Function glStencilOp:Void( fail,zfail,zpass )
Function glTexEnvi:Void( target,pname,param )
Function glTexParameteri:Void( target,pname,param )
Function glViewport:Void( x,y,width,height )
'${END}

#Endif

Public

#If TARGET="glfw" Or TARGET="ios" Or TARGET="android"

Function LoadImageData:DataBuffer( path:String,info:Int[]=[] )
	Local buf:=New DataBuffer
	If BBLoadImageData( buf,path,info ) Return buf
	Return Null
End

#Endif
