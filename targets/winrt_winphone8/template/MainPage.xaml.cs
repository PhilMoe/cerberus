using System;
using System.Windows;
using MonkeyGame.Resources;
using Microsoft.Phone.Controls;
using System.ComponentModel;

using MonkeyGame_PhoneComponent;

namespace MonkeyGame
{
    public partial class MainPage : PhoneApplicationPage
    {
        int _rot = 0;
        Direct3DBackground _background = null;

        // Constructor
        public MainPage()
        {
            InitializeComponent();
        }

        public Direct3DBackground D3dBackground
        {
            get{ return _background; }
        }

        private void DrawingSurfaceBackground_Loaded(object sender, RoutedEventArgs e)
        {
            if (_background == null)
            {
                _background = new Direct3DBackground();
                _background.DeviceRotation = _rot;

                // Set window bounds in dips
                _background.WindowBounds = new Windows.Foundation.Size(
                    (float)Application.Current.Host.Content.ActualWidth,
                    (float)Application.Current.Host.Content.ActualHeight
                    );

                // Set native resolution in pixels
                _background.NativeResolution = new Windows.Foundation.Size(
                    (float)Math.Floor(Application.Current.Host.Content.ActualWidth * Application.Current.Host.Content.ScaleFactor / 100.0f + 0.5f),
                    (float)Math.Floor(Application.Current.Host.Content.ActualHeight * Application.Current.Host.Content.ScaleFactor / 100.0f + 0.5f)
                    );

                // Set render resolution to the full native resolution
                _background.RenderResolution=_background.NativeResolution;

                // Hook-up native component to DrawingSurfaceBackgroundGrid
                DrawingSurfaceBackground.SetBackgroundContentProvider(_background.CreateContentProvider());
                DrawingSurfaceBackground.SetBackgroundManipulationHandler(_background);
                
                _background.PostToUIThread=PostToUIThread;
            }
        }
        
        private void PostToUIThread(){

            Dispatcher.BeginInvoke( () => { _background.RunOnUIThread(); } );
        }

        protected override void OnBackKeyPress(CancelEventArgs e)
        {
            e.Cancel=_background.OnBackKeyPress();
        }

        //Can be called before Loaded!
        protected override void OnOrientationChanged(OrientationChangedEventArgs e)
        {
            switch (this.Orientation)
            {
                case PageOrientation.PortraitUp: _rot = 0; break;
                case PageOrientation.LandscapeLeft: _rot = 1; break;
                case PageOrientation.PortraitDown: _rot = 2; break;
                case PageOrientation.LandscapeRight: _rot = 3; break;
            }
            if (_background!=null )
            {
                _background.DeviceRotation = _rot;
            }
        } 
    }
}
