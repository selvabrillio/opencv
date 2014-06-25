﻿//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

// cannot include here because we can't export CaptureFrameGrabber WinRT class
// #include "../../../modules/highgui/src/cap_winrt_video.hpp"

#include "../../../modules/highgui/src/cap_winrt_highgui.hpp"

using namespace highgui_xaml;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

#include <ppl.h>
#include <ppltasks.h>
#include <concrt.h>
#include <agile.h>

using namespace ::concurrency;
using namespace ::Windows::Foundation;

using namespace Windows::UI::Xaml::Media::Imaging;

// debug
#include "../highgui_xaml.Windows/cdebug.h"

// needed for linker
extern bool initGrabber(int device, int w, int h);
extern void copyOutput();

namespace highgui_xaml
{


    MainPage::MainPage()
    {
        InitializeComponent();

        // from samples:
        //        visibilityToken = Window::Current->VisibilityChanged::add(ref new WindowVisibilityChangedEventHandler(this, &Scenario1::VisibilityChanged));
        // Window::Current->VisibilityChanged

        Window::Current->VisibilityChanged += ref new Windows::UI::Xaml::WindowVisibilityChangedEventHandler(this, &highgui_xaml::MainPage::OnVisibilityChanged);

        // set XAML elements
        HighguiBridge::getInstance().cvImage = cvImage;
        HighguiBridge::getInstance().cvSlider = cvSlider;

        // handler
        cvSlider->ValueChanged +=
            ref new RangeBaseValueChangedEventHandler(this, &MainPage::cvSlider_ValueChanged);

        auto asyncTask = TaskWithProgressAsync();
        asyncTask->Progress = ref new AsyncActionProgressHandler<int>([this](IAsyncActionWithProgress<int>^ act, int progress)
        {
            int action = progress;

            // these actions will be processed on the UI thread asynchronously
            switch (action)
            {
            case OPEN_CAMERA:
            {
                int device = HighguiBridge::getInstance().deviceIndex;
                int width = HighguiBridge::getInstance().width;
                int height = HighguiBridge::getInstance().height;

                // buffers must alloc'd on UI thread
                allocateBuffers(width, height);

                // video capture device init must be done on UI thread;
                // code is located in the OpenCV Highgui DLL, class Video
                // initGrabber will be called whenever the window is made visible
                ///initGrabber(device, width, height);
            }
                break;
            case CLOSE_CAMERA:
                closeGrabber();
                break;
            case UPDATE_IMAGE_ELEMENT:
                // zv
                // testing: for direct copy bypassing OpenCV:
                // HighguiBridge::getInstance().m_cvImage->Source = HighguiBridge::getInstance().backInputPtr;

                // copy output Mat to WBM
                copyOutput();

                // set XAML image element with image WBM
                HighguiBridge::getInstance().cvImage->Source = HighguiBridge::getInstance().outputBuffer;

                // test
                //HighguiBridge::getInstance().bIsFrameNew = false;

                break;
            case SHOW_TRACKBAR:
                cvSlider->Visibility = Windows::UI::Xaml::Visibility::Visible;
                break;
            }
        });


    }

    void MainPage::cvSlider_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
    {
        sliderChanged1(e->NewValue);
    }
}

// implemented in main.cpp
void cvMain();

// set the reporter method for the HighguiAssist singleton
// start the main OpenCV as an async thread in WinRT
IAsyncActionWithProgress<int>^ MainPage::TaskWithProgressAsync()
{
    return create_async([this](progress_reporter<int> reporter)
    {
        HighguiBridge::getInstance().setReporter(reporter);
        cvMain();
    });
}



void highgui_xaml::MainPage::OnVisibilityChanged(Platform::Object ^sender, Windows::UI::Core::VisibilityChangedEventArgs ^e)
{
    TCC("    OnVisibilityChanged"); TC(e->Visible); TCNL;

    if (e->Visible) 
    {
        // only start the grabber if the camera was opened in OpenCV
        if (HighguiBridge::getInstance().backInputPtr != nullptr)
        {
            int device = HighguiBridge::getInstance().deviceIndex;
            int width = HighguiBridge::getInstance().width;
            int height = HighguiBridge::getInstance().height;

            initGrabber(device, width, height);
        }
    }
    else 
    {
        closeGrabber();
    }

//    throw ref new Platform::NotImplementedException();
}