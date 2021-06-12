
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;

import android.opengl.GLSurfaceView;
import android.util.Log;

// This class shows how to use multisampling. To use this, call
//   myGLSurfaceView.setEGLConfigChooser(new MultisampleConfigChooser());
// before calling setRenderer(). Multisampling will probably slow down
// your app -- measure performance carefully and decide if the vastly
// improved visual quality is worth the cost.
class MultisampleConfigChooser implements GLSurfaceView.EGLConfigChooser {
	static private final String kTag = "GDC11";
	@Override
	public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
		mValue = new int[1];

		// Try to find a normal multisample configuration first.
		int[] configSpec = {
				EGL10.EGL_RED_SIZE, 5,
                EGL10.EGL_GREEN_SIZE, 6,
                EGL10.EGL_BLUE_SIZE, 5,
                EGL10.EGL_DEPTH_SIZE, 16,
                // Requires that setEGLContextClientVersion(2) is called on the view.
                EGL10.EGL_RENDERABLE_TYPE, 4 /* EGL_OPENGL_ES2_BIT */,
                EGL10.EGL_SAMPLE_BUFFERS, 1 /* true */,
                EGL10.EGL_SAMPLES, 4,
                EGL10.EGL_NONE
		};

		if (!egl.eglChooseConfig(display, configSpec, null, 0,
				mValue)) {
			throw new IllegalArgumentException("eglChooseConfig failed");
		}
		int numConfigs = mValue[0];

		if (numConfigs <= 0) {

			configSpec = new int[]{
					EGL10.EGL_RED_SIZE, 5,
					EGL10.EGL_GREEN_SIZE, 6,
					EGL10.EGL_BLUE_SIZE, 5,
					EGL10.EGL_DEPTH_SIZE, 16,
					// Requires that setEGLContextClientVersion(2) is called on the view.
					EGL10.EGL_RENDERABLE_TYPE, 4 /* EGL_OPENGL_ES2_BIT */,
					EGL10.EGL_SAMPLE_BUFFERS, 1 /* true */,
					EGL10.EGL_SAMPLES, 2,
					EGL10.EGL_NONE
			};

			if (!egl.eglChooseConfig(display, configSpec, null, 0,
					mValue)) {
				throw new IllegalArgumentException("2nd eglChooseConfig failed");
			}
			numConfigs = mValue[0];

			if (numConfigs <= 0) {
				// No normal multisampling config was found. Try to create a
				// converage multisampling configuration, for the nVidia Tegra2.
				// See the EGL_NV_coverage_sample documentation.

				final int EGL_COVERAGE_BUFFERS_NV = 0x30E0;
				final int EGL_COVERAGE_SAMPLES_NV = 0x30E1;

				configSpec = new int[]{
						EGL10.EGL_RED_SIZE, 5,
						EGL10.EGL_GREEN_SIZE, 6,
						EGL10.EGL_BLUE_SIZE, 5,
						EGL10.EGL_DEPTH_SIZE, 16,
						EGL10.EGL_RENDERABLE_TYPE, 4 /* EGL_OPENGL_ES2_BIT */,
						EGL_COVERAGE_BUFFERS_NV, 1 /* true */,
						EGL_COVERAGE_SAMPLES_NV, 2,  // always 5 in practice on tegra 2
						EGL10.EGL_NONE
				};

				if (!egl.eglChooseConfig(display, configSpec, null, 0,
						mValue)) {
					throw new IllegalArgumentException("3nd eglChooseConfig failed");
				}
				numConfigs = mValue[0];

				if (numConfigs <= 0) {
					// Give up, try without multisampling.
					configSpec = new int[]{
							EGL10.EGL_RED_SIZE, 5,
							EGL10.EGL_GREEN_SIZE, 6,
							EGL10.EGL_BLUE_SIZE, 5,
							EGL10.EGL_DEPTH_SIZE, 16,
							EGL10.EGL_RENDERABLE_TYPE, 4 /* EGL_OPENGL_ES2_BIT */,
							EGL10.EGL_NONE
					};

					if (!egl.eglChooseConfig(display, configSpec, null, 0,
							mValue)) {
						throw new IllegalArgumentException("4rd eglChooseConfig failed");
					}
					numConfigs = mValue[0];

					if (numConfigs <= 0) {
						throw new IllegalArgumentException("No configs match configSpec");
					}
				} else {
					mUsesCoverageAa = true;
				}
			}
		}

		// Get all matching configurations.
		EGLConfig[] configs = new EGLConfig[numConfigs];
		if (!egl.eglChooseConfig(display, configSpec, configs, numConfigs,
				mValue)) {
			throw new IllegalArgumentException("data eglChooseConfig failed");
		}

		// CAUTION! eglChooseConfigs returns configs with higher bit depth
		// first: Even though we asked for rgb565 configurations, rgb888
		// configurations are considered to be "better" and returned first.
		// You need to explicitly filter the data returned by eglChooseConfig!
		int index = -1;
		for (int i = 0; i < configs.length; ++i) {
			if (findConfigAttrib(egl, display, configs[i], EGL10.EGL_RED_SIZE, 0) == 5) {
				index = i;
				break;
			}
		}
		if (index == -1) {
			Log.w(kTag, "Did not find sane config, using first");
		}
		EGLConfig config = configs.length > 0 ? configs[index] : null;
		if (config == null) {
			throw new IllegalArgumentException("No config chosen");
		}
		return config;
	}

	private int findConfigAttrib(EGL10 egl, EGLDisplay display,
								 EGLConfig config, int attribute, int defaultValue) {
		if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
			return mValue[0];
		}
		return defaultValue;
	}

	public boolean usesCoverageAa() {
		return mUsesCoverageAa;
	}

	private int[] mValue;
	private boolean mUsesCoverageAa;
}