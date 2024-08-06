#include <fstream>
#include <iostream>
#include <string>

std::string get_file_contents(const char *filename)
{
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	return std::string();
}

std::ifstream::pos_type getFileSize(const char* filename)
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	return in.tellg(); 
}

void _CreateIcon(String srcFilename, String dstFilename, int w, int h,int round) {
	int width,height,n;
	
	stbi_uc *input_pixels = stbi_load(C_STR(srcFilename), &width, &height, &n, 4);

// Under MSVC. Use a pointer to stop error C2131: expression did not evaluate to a constant
#ifdef _MSC_VER
	stbi_uc *output_pixels = new stbi_uc[4*w*h];
#else
	stbi_uc output_pixels[4*w*h];
#endif
	
	stbir_resize_uint8(input_pixels,width,height,0, output_pixels, w,h,0,4);
	
	if (round) {
		int i,r;
		for (int x = 0; x < w; x++) {
			for (int y = 0; y < h; y++) {
				i = x + y*w;
				r = sqrt(((w/2-x)*(w/2-x)) + ((h/2-y)*(h/2-y)));
				if (r>w/2-1) {
					output_pixels[i*4+3] = 0;
				}
			}
		}
	}
	
	stbi_write_png(C_STR(dstFilename), w, h, 4, &output_pixels, 0);
#ifdef _MSC_VER
	delete output_pixels;
#endif
}

// crudely build the icon file... file format on wiki
// https://en.wikipedia.org/wiki/ICO_(file_format)
void _ConvertToIco(String srcFilename,String destFilename) {
	// widths for icons
	int widths[] = {16,32,48,64,128,256};
	
	// build indiviual icon files for icon file
	int num_of_images=sizeof(widths) / sizeof(widths[0]);;

#ifdef _MSC_VER
	int *size_of_files = new int[num_of_images];
#else
	int size_of_files[num_of_images];
#endif

	int width;
	int size_of_data=0;
	int i=0;
	while (i<num_of_images) {
		width = widths[i];
		
		String tmpname = srcFilename+".tmp."+width;
		_CreateIcon(srcFilename, tmpname, width,width,0);

		size_of_files[i] = getFileSize(C_STR(tmpname));
		size_of_data=size_of_data+size_of_files[i];

		i++;
	}

	// final file data
	int size_of_file=6+size_of_data+16*num_of_images;
	char* new_data=new char[size_of_file];
	// header
	new_data[0]=0;
	new_data[1]=0;
	new_data[2]=1;
	new_data[3]=0;
	// number of images
	new_data[4]=num_of_images & 255;
	new_data[5]=(num_of_images >> 8) & 255;

	// image headers
	int offset=6;
	int data_pointer = 6+num_of_images*16;
	
	i=0;
	while (i<num_of_images) {
		width = widths[i];

		// image header
		new_data[offset+0] = width;
		new_data[offset+1] = width;
		new_data[offset+2] = 0;
		new_data[offset+3] = 0;
		new_data[offset+4] = 0;
		new_data[offset+5] = 0;
		new_data[offset+6] = 32;
		new_data[offset+7] = 0;
		// size of data (little endian)
		new_data[offset+8] = (size_of_files[i] >> 0) & 255;
		new_data[offset+9] = (size_of_files[i] >> 8) & 255;
		new_data[offset+10] = (size_of_files[i] >> 16) & 255;
		new_data[offset+11] = (size_of_files[i] >> 24) & 255;
		// offset to image data (little endian)
		new_data[offset+12] = (data_pointer >> 0) & 255;
		new_data[offset+13] = (data_pointer >> 8) & 255;
		new_data[offset+14] = (data_pointer >> 16) & 255;
		new_data[offset+15] = (data_pointer >> 24) & 255;
		
		offset = offset+16;
		data_pointer = data_pointer + size_of_files[i];
		i++;
	}
	
	// add data
	data_pointer = 6+num_of_images*16;	// end of header
	
	i=0;
	while (i<num_of_images) {
		width = widths[i];

		String tmpname = srcFilename+".tmp."+width;
		std::ifstream infile (C_STR(tmpname),std::ifstream::binary);
		char* buffer = new char[size_of_files[i]];
		infile.read (buffer,size_of_files[i]);
		infile.close();

		// copy image data in
		for (int j = 0; j < size_of_files[i]; j++) {
			new_data[data_pointer+j] = buffer[j];
		}

		// delete temp file
		// this bit of code from stb_image as i struggled to convert string to *wchar
#if _WIN32
		wchar_t wFilename[1024];
		if (0 != MultiByteToWideChar(65001 /* UTF8 */, 0, C_STR(tmpname), -1, wFilename, sizeof(wFilename)))
			remove(wFilename);
#else
		remove(OS_STR(tmpname));
#endif
		data_pointer = data_pointer + size_of_files[i];
		i++;
	}

#ifdef _MSC_VER
	delete size_of_files;
#endif

	// write file
	std::ofstream outfile (C_STR(destFilename),std::ofstream::binary);
	outfile.write (new_data,size_of_file);
	outfile.close();
}

