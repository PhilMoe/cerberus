
//Lang/OS...
#include <ctime>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <typeinfo>
#include <signal.h>

#if _WIN32
#include <winsock2.h>
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#include <sys/stat.h>
#undef LoadString

#elif __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#include <ApplicationServices/ApplicationServices.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <dirent.h>
#include <copyfile.h>
#include <pthread.h>
#include <OpenGl/gl.h>  // header include necessary for mojo 1

#elif __linux
#define GL_GLEXT_PROTOTYPES
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#endif

// Graphics/Audio stuff

//OpenGL...
#include <GLFW/glfw3.h>

//OpenAL...
//
#include <al.h>
#include <alc.h>

//stb_image lib
//
#include <stb_image.h>

//stb_vorbis lib
//
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
