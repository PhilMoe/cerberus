
import java.nio
class BBFileSystemDriver{

	int FileType( String path ){
		File f=new File( path );
		if( f.isFile() ) return 1;
		if( f.isDirectory() ) return 2;
		return 0;
	}
	
	int FileSize( String path ){
		File f=new File( path );
		return (int)f.length();
	}
	
	int FileTime( String path ){
		File f=new File( path );
		return (int)f.lastModified();
	}
	
	int CreateFile( String path ){
		File f=new File( path );
		try{
			return f.createNewFile() ? 1 : 0;
		}catch( IOException ex ){
		}
		return 0;
	}
	
	int DeleteFile( String path ){
		File f=new File( path );
		return f.delete() ? 1 : 0;
	}
	
	int CopyFile( String srcPath,String dstPath ){
	/*
		File srcFile=new File( srcPath );
		File dstFile=new File( dstPath );
		
		if( !dstFile.exists() ) dstFile.createNewFile();

	    FileChannel srcChan=null;
    	FileChannel dstChan=null;
    	
		try{
			srcChan=new FileInputStream( srcFile ).getChannel();
			dstChan=new FileOutputStream( dstFile ).getChannel();
			dstChan.transferFrom( srcChan,0,srcChan.size() );
		}finally{
	    	if( srcChan!=null ) srcChan.close();
	    	if( dstChan!=null ) dstChan.close();
		}
		*/
		return 0;
	}
	
	int CreateDir( String path ){
		File f=new File( path );
		return f.mkdir() ? 1 : 0;
	}
	
	
	int DeleteDir( String path ){
		File f=new File( path );
		return f.delete() ? 1 : 0;
	}
	
	String[] LoadDir( String path ){
		File f=new File( path );
		return f.list();
	}
	
	String CurrentDir(){
		return System.getProperty( "user.dir" );
	}
	
	void ChangeDir( String path ){
		System.setProperty( "user.dir",path );
	}
};
