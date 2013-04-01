
//Note: OS_CHAR and OS_STR are declared in os file os.cpp

int info_width;
int info_height;

int get_info_png( String path ){

	if( FILE *f=_fopen( OS_STR(path),OS_STR("rb") ) ){
		unsigned char data[32];
		int n=fread( data,1,24,f );
		fclose( f );
		if( n==24 && data[1]=='P' && data[2]=='N' && data[3]=='G' ){
			info_width=(data[16]<<24)|(data[17]<<16)|(data[18]<<8)|(data[19]);
			info_height=(data[20]<<24)|(data[21]<<16)|(data[22]<<8)|(data[23]);
			return 0;
		}
	}
	return -1;
}

int get_info_gif( String path ){

	if( FILE *f=_fopen( OS_STR(path),OS_STR("rb") ) ){
		unsigned char data[32];
		int n=fread( data,1,10,f );
		fclose( f );
		if( n==10 && data[0]=='G' && data[1]=='I' && data[2]=='F' ){
			info_width=(data[7]<<8)|data[6];
			info_height=(data[9]<<8)|data[8];
			return 0;
		}
	}
	return -1;
}

int get_info_jpg( String path ){

	if( FILE *f=_fopen( OS_STR(path),OS_STR("rb") ) ){
	
		unsigned char buf[32];
		
		if( fread( buf,1,2,f )==2 && buf[0]==0xff && buf[1]==0xd8 ){
		
			for(;;){
		
				while( fread( buf,1,1,f )==1 && buf[0]!=0xff ){}
				if( feof( f ) ) break;
				
				while( fread( buf,1,1,f )==1 && buf[0]==0xff ){}
				if( feof( f ) ) break;
				
				int marker=buf[0];
				
				switch( marker ){
				case 0xD0:case 0xD1:case 0xD2:case 0xD3:case 0xD4:case 0xD5:
				case 0xD6:case 0xD7:case 0xD8:case 0xD9:case 0x00:case 0xFF:

					break;
					
				default:
					if( fread( buf,1,2,f )==2 ){
					
						int datalen=((buf[0]<<8)|buf[1])-2;
						
						switch( marker ){
						case 0xC0:case 0xC1:case 0xC2:case 0xC3:
							if( datalen && fread( buf,1,5,f )==5 ){
								int bpp=buf[0];
								info_height=(buf[1]<<8)|buf[2];
								info_width=(buf[3]<<8)|buf[4];
								fclose( f );
								return 0;
							}
						}
						
						if( fseek( f,datalen,SEEK_CUR )<0 ){
							fclose( f );
							return -1;
						}
						
					}else{
						fclose( f );
						return -1;
					}
				}
			}
		}
		fclose( f );
	}
	return -1;
}
