
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
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <dirent.h>
#include <copyfile.h>
#include <unistd.h>
#include <pthread.h>

#elif __linux
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#endif

#define _QUOTE(X) #X
#define _STRINGIZE( X ) _QUOTE(X)
