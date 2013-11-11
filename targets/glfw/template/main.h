
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
#include <windows.h>
#include <direct.h>
#include <sys/stat.h>
#undef LoadString

#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <dirent.h>
#include <copyfile.h>
#include <pthread.h>

#elif __linux
#define GL_GLEXT_PROTOTYPES
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#endif

// Graphics/Audio stuff

//OpenGL...
#include <GL/glfw.h>

//OpenAL...
#include <al.h>
#include <alc.h>

//STB_image lib...
#define STBI_HEADER_FILE_ONLY
#include <stb_image.c>

//stb_vorbis lib
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
