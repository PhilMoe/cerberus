
import java.nio.channels.FileChannel;
import android.widget.Toast;

class BBFileSystem{

	static File file( String path ){
		return new File( path );
	}
	
	static String FixPath( String path ){
		return BBGame.Game().PathToFilePath( path );
	}
	
	static String RealPath( String path ){
		return file( path ).getAbsolutePath();
	}

	static int FileType( String path ){
		File f=file( path );
		return f.isFile() ? 1 : (f.isDirectory() ? 2 : 0);
	}
	
	static int FileSize( String path ){
		File f=file( path );
		return (int)f.length();
	}
	
	static int FileTime( String path ){
		File f=file( path );
		return (int)(f.lastModified()/1000);
	}
	
	static boolean CreateFile( String path ){
		File f=file( path );
		if( f.isFile() && !f.delete() ) return false;
		try{
			return f.createNewFile();
		}catch( IOException ex ){
		}
		return false;
	}
	
	static boolean DeleteFile( String path ){
		File f=file( path );
		return (f.isFile() && f.delete());
	}
	
	static boolean CopyFile( String src,String dst ){
	
		boolean ok=false;
		FileChannel srcc,dstc;
		
		try{
		
			File srcf=file( src );
			
			if( !srcf.isFile() ) {
				AssetManager am=BBAndroidGame.AndroidGame()._activity.getAssets();
				InputStream inputStream=am.open(src);
				
				try{
					File dstf=file(dst);
					if( dstf.exists() && (!dstf.isFile() || !dstf.delete()) ) return false;
					if( !dstf.createNewFile() ) return false;
						
					OutputStream outputStream=new FileOutputStream( dstf );
					byte buffer[]=new byte[1024];
					int length=0;

					while( (length=inputStream.read( buffer ))>0 ){
						outputStream.write( buffer,0,length );
					}

					outputStream.close();
					inputStream.close();

					return true;
				}catch( IOException e ){
				}
				return false;				
			}
			
			File dstf=file( dst );
			if( dstf.exists() && (!dstf.isFile() || !dstf.delete()) ) return false;
			if( !dstf.createNewFile() ) return false;
			
			srcc=new FileInputStream( srcf ).getChannel();
			if( srcc!=null ){
				dstc=new FileOutputStream( dstf ).getChannel();
				if( dstc!=null ){
					ok=srcc.transferTo( 0,srcf.length(),dstc )==srcf.length();
					dstc.close();
				}
				srcc.close();
			}
		}catch( Exception ex ){
		}
		
		return ok;
	}
	
	static boolean CreateDir( String path ){
		File f=file( path );
		return f.mkdir();
	}
	
	static boolean DeleteDir( String path ){
		File f=file( path );
		return (f.isDirectory() && f.delete());
	}
	
	static String[] LoadDir( String path ){
		File f=file( path );
		File[] files=f.listFiles();
		if( files==null ) return new String[0];
		String[] names=new String[files.length];
		for( int i=0;i<files.length;++i ){
			names[i]=files[i].getName();
		}
		return names;
    }
}
