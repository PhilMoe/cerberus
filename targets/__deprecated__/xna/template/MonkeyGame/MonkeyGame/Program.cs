
//Enable this ONLY if you upgrade the Windows Phone project to 7.1!
//#define MANGO

using System;
using System.IO;
using System.IO.IsolatedStorage;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Globalization;
using System.Threading;
using System.Diagnostics;
//#if WINDOWS
//using System.Windows.Forms;
//#endif

using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Audio;
using Microsoft.Xna.Framework.Content;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Microsoft.Xna.Framework.Media;


#if WINDOWS_PHONE
using Microsoft.Devices.Sensors;
using Microsoft.Xna.Framework.Input.Touch;
#endif

public class MonkeyConfig{
//${CONFIG_BEGIN}
//${CONFIG_END}
}

//${TRANSCODE_BEGIN}
//${TRANSCODE_END}
