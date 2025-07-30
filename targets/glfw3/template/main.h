
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

//SoLoud Audio Lib
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>
#include <map>

//stb_image lib
//
#include <stb_image.h>
