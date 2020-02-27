void _CreateIcon(String srcFilename, String dstFilename, int w, int h,int round) {
	int width,height,n;
	
	stbi_uc *input_pixels = stbi_load(C_STR(srcFilename), &width, &height, &n, 4);
	stbi_uc output_pixels[4*w*h];
	
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
}
