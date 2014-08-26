
// Stdcpp trans.system runtime.
//
// Placed into the public domain 24/02/2011.
// No warranty implied; use as your own risk.

#if _WIN32

#define mkdir( X,Y ) _wmkdir( X )
#define rmdir _wrmdir
#define remove _wremove
#define rename _wrename
#define stat _wstat
#define _fopen _wfopen

#else

#define _fopen fopen

#endif

class BBFileSystem{

#if _WIN32
	typedef wchar_t OS_CHAR;
	typedef struct _stat stat_t;
#else
	typedef char OS_CHAR;
	typedef struct stat stat_t;
#endif

	static String::CString<char> C_STR( const String &t ){
		return t.ToCString<char>();
	}
	
	static String::CString<OS_CHAR> OS_STR( const String &t ){
		return t.ToCString<OS_CHAR>();
	}
	
	public:
	
	static String FixPath( String path ){
		return BBGame::Game()->PathToFilePath( path );
	}
	
	static String RealPath( String path ){
#if _WIN32
		OS_CHAR buf[ MAX_PATH+1 ];
		GetFullPathNameW( OS_STR(path),MAX_PATH,buf,0 );
		return String( buf );
#else
		OS_CHAR buf[ PATH_MAX+1 ];
		realpath( OS_STR( path ),buf );
		return String( buf );
/*		
		std::vector<OS_CHAR> buf( PATH_MAX+1 );
		if( realpath( OS_STR( path ),&buf[0] ) ){}
		buf[buf.size()-1]=0;
		for( int i=0;i<PATH_MAX && buf[i];++i ){
			if( buf[i]=='\\' ) buf[i]='/';
			
		}
		return String( &buf[0] );
*/
#endif
	}
	
	static int FileType( String path ){
		stat_t st;
		if( stat( OS_STR(path),&st ) ) return 0;
		switch( st.st_mode & S_IFMT ){
		case S_IFREG : return 1;
		case S_IFDIR : return 2;
		}
		return 0;
	}
	
	static int FileSize( String path ){
		stat_t st;
		if( stat( OS_STR(path),&st ) ) return 0;
		return st.st_size;
	}
	
	static int FileTime( String path ){
		stat_t st;
		if( stat( OS_STR(path),&st ) ) return 0;
		return st.st_mtime;
	}
	
	static bool DeleteFile( String path ){
		remove( OS_STR(path) );
		return FileType(path)==0;
	}
		
	static bool CopyFile( String srcpath,String dstpath ){
	
#if _WIN32
		return CopyFileW( OS_STR(srcpath),OS_STR(dstpath),FALSE );
#elif __APPLE__
	
		// Would like to use COPY_ALL here, but it breaks trans on MacOS - produces weird 'pch out of date' error with copied projects.
		//
		// Ranlib strikes back!
		//
		return copyfile( OS_STR(srcpath),OS_STR(dstpath),0,COPYFILE_DATA )>=0;
#else
		int err=-1;
		if( FILE *srcp=_fopen( OS_STR( srcpath ),OS_STR("rb") ) ){
			err=-2;
			if( FILE *dstp=_fopen( OS_STR( dstpath ),OS_STR("wb") ) ){
				err=0;
				char buf[1024];
				while( int n=fread( buf,1,1024,srcp ) ){
					if( fwrite( buf,1,n,dstp )!=n ){
						err=-3;
						break;
					}
				}
				fclose( dstp );
			}else{
//				printf( "FOPEN 'wb' for CopyFile(%s,%s) failed\n",C_STR(srcpath),C_STR(dstpath) );
				fflush( stdout );
			}
			fclose( srcp );
		}else{
//			printf( "FOPEN 'rb' for CopyFile(%s,%s) failed\n",C_STR(srcpath),C_STR(dstpath) );
			fflush( stdout );
		}
		return err==0;
#endif
	}
	
	static bool CreateFile( String path ){
		if( FILE *f=_fopen( OS_STR( path ),OS_STR( "wb" ) ) ){
			fclose( f );
			return true;
		}
		return false;
	}
	
	static bool CreateDir( String path ){
		mkdir( OS_STR( path ),0777 );
		return FileType(path)==2;
	}
	
	static bool DeleteDir( String path ){
		rmdir( OS_STR(path) );
		return FileType(path)==0;
	}
	
	static Array<String> LoadDir( String path ){
		std::vector<String> files;
		
#if _WIN32
		WIN32_FIND_DATAW filedata;
		HANDLE handle=FindFirstFileW( OS_STR(path+"/*"),&filedata );
		if( handle!=INVALID_HANDLE_VALUE ){
			do{
				String f=filedata.cFileName;
				if( f=="." || f==".." ) continue;
				files.push_back( f );
			}while( FindNextFileW( handle,&filedata ) );
			FindClose( handle );
		}else{
//			printf( "FindFirstFileW for LoadDir(%s) failed\n",C_STR(path) );
			fflush( stdout );
		}
#else
		if( DIR *dir=opendir( OS_STR(path) ) ){
			while( dirent *ent=readdir( dir ) ){
				String f=ent->d_name;
				if( f=="." || f==".." ) continue;
				files.push_back( f );
			}
			closedir( dir );
		}else{
//			printf( "opendir for LoadDir(%s) failed\n",C_STR(path) );
			fflush( stdout );
		}
#endif
		return files.size() ? Array<String>( &files[0],files.size() ) : Array<String>();
	}
};
