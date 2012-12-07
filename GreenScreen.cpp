﻿//------------------------------------------------------------------------------
// <copyright file="GreenScreen.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <strsafe.h>
#include "GreenScreen.h"
#include "resource.h"
#include "SimpleMIDIPlayer.h"

// FROM SKELETONBASICS
static const float g_JointThickness = 3.0f;
static const float g_TrackedBoneThickness = 6.0f;
static const float g_InferredBoneThickness = 1.0f;

// Global MIDI player
SimpleMIDIPlayer* midiPlayer;

/// <summary>
/// Entry point for the application
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="hPrevInstance">always 0</param>
/// <param name="lpCmdLine">command line arguments</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
/// <returns>status</returns>
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    CGreenScreen application;
	midiPlayer = new SimpleMIDIPlayer();
    application.Run(hInstance, nCmdShow);
}

/// <summary>
/// Constructor
/// </summary>
CGreenScreen::CGreenScreen() :
    m_pDrawGreenScreen(NULL),
    m_hNextDepthFrameEvent(INVALID_HANDLE_VALUE),
    m_hNextColorFrameEvent(INVALID_HANDLE_VALUE),
    m_pDepthStreamHandle(INVALID_HANDLE_VALUE),
    m_pColorStreamHandle(INVALID_HANDLE_VALUE),
    m_bNearMode(false),
	m_hNextSkeletonEvent(INVALID_HANDLE_VALUE),
    m_pSkeletonStreamHandle(INVALID_HANDLE_VALUE),
    m_bSeatedMode(false),
    m_pNuiSensor(NULL)
{
	// BELOW ZEROMEMORY FROM SKELETONBASICS
	ZeroMemory(m_Points,sizeof(m_Points));

    // get resolution as DWORDS, but store as LONGs to avoid casts later
    DWORD width = 0;
    DWORD height = 0;

    NuiImageResolutionToSize(cDepthResolution, width, height);
    m_depthWidth  = static_cast<LONG>(width);
    m_depthHeight = static_cast<LONG>(height);

    NuiImageResolutionToSize(cColorResolution, width, height);
    m_colorWidth  = static_cast<LONG>(width);
    m_colorHeight = static_cast<LONG>(height);

    m_colorToDepthDivisor = m_colorWidth/m_depthWidth;

    m_depthTimeStamp.QuadPart = 0;
    m_colorTimeStamp.QuadPart = 0;

    // create heap storage for depth pixel data in RGBX format
    m_depthD16 = new USHORT[m_depthWidth*m_depthHeight];
    m_colorCoordinates = new LONG[m_depthWidth*m_depthHeight*2];

    m_colorRGBX = new BYTE[m_colorWidth*m_colorHeight*cBytesPerPixel];
    m_outputRGBX = new BYTE[m_colorWidth*m_colorHeight*cBytesPerPixel];
}

/// <summary>
/// Destructor
/// </summary>
CGreenScreen::~CGreenScreen()
{
    if (m_pNuiSensor)
    {
        m_pNuiSensor->NuiShutdown();
    }

    if (m_hNextDepthFrameEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextDepthFrameEvent);
    }

    if (m_hNextColorFrameEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextColorFrameEvent);
    }

	if (m_hNextSkeletonEvent && (m_hNextSkeletonEvent != INVALID_HANDLE_VALUE))
    {
        CloseHandle(m_hNextSkeletonEvent);
    }

    // clean up Direct2D renderer
    delete m_pDrawGreenScreen;
    m_pDrawGreenScreen = NULL;

    // done with pixel data
    delete[] m_depthD16;
    delete[] m_colorCoordinates;

    delete[] m_colorRGBX;
    delete[] m_outputRGBX;

    SafeRelease(m_pNuiSensor);
}

/// <summary>
/// Creates the main window and begins processing
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
int CGreenScreen::Run(HINSTANCE hInstance, int nCmdShow)
{
    MSG       msg = {0};
    //WNDCLASS  wc;
	WNDCLASS wc = {0};

    // Dialog custom window class
    ZeroMemory(&wc, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbWndExtra    = DLGWINDOWEXTRA;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.lpfnWndProc   = DefDlgProcW;
    wc.lpszClassName = L"GreenScreenAppDlgWndClass";

    if (!RegisterClassW(&wc))
    {
        return 0;
    }

    // Create main application window
    HWND hWndApp = CreateDialogParamW(
        hInstance,
        MAKEINTRESOURCE(IDD_APP),
        NULL,
        (DLGPROC)CGreenScreen::MessageRouter, 
        reinterpret_cast<LPARAM>(this));

    // Show window
    ShowWindow(hWndApp, nCmdShow);

    //const int eventCount = 2;
	const int eventCount = 3;
    HANDLE hEvents[eventCount];

    // Main message loop
    while (WM_QUIT != msg.message)
    {
        hEvents[0] = m_hNextDepthFrameEvent;
        hEvents[1] = m_hNextColorFrameEvent;
		hEvents[2] = m_hNextSkeletonEvent;

        // Check to see if we have either a message (by passing in QS_ALLINPUT)
        // Or a Kinect event (hEvents)
        // Update() will check for Kinect events individually, in case more than one are signalled
        DWORD dwEvent = MsgWaitForMultipleObjects(eventCount, hEvents, FALSE, INFINITE, QS_ALLINPUT);

        // Check if this is an event we're waiting on and not a timeout or message
        if (WAIT_OBJECT_0 == dwEvent || WAIT_OBJECT_0 + 1 == dwEvent || WAIT_OBJECT_0 + 2 == dwEvent)
        {
            Update();
        }

        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // If a dialog message will be taken care of by the dialog proc
            if ((hWndApp != NULL) && IsDialogMessageW(hWndApp, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

/// <summary>
/// Main processing function
/// </summary>
void CGreenScreen::Update()
{
    if (NULL == m_pNuiSensor)
    {
        return;
    }

    bool needToDraw = false;
	handleSkeletons = false;

    if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextDepthFrameEvent, 0) )
    {
        // if we have received any valid new depth data we may need to draw
        if ( SUCCEEDED(ProcessDepth()) )
        {
            needToDraw = true;
        }
    }

    if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextColorFrameEvent, 0) )
    {
        // if we have received any valid new color data we may need to draw
        if ( SUCCEEDED(ProcessColor()) )
        {
            needToDraw = true;
        }
    }

    if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextSkeletonEvent, 0) )
    {
        ProcessSkeleton();
		handleSkeletons = true;
		needToDraw = true;
    }

    // Depth is 30 fps.  For any given combination of FPS, we should ensure we are within half a frame of the more frequent of the two.  
    // But depth is always the greater (or equal) of the two, so just use depth FPS.
    const int depthFps = 30;
    const int halfADepthFrameMs = (1000 / depthFps) / 2;

    // If we have not yet received any data for either color or depth since we started up, we shouldn't draw
    if (m_colorTimeStamp.QuadPart == 0 || m_depthTimeStamp.QuadPart == 0)
    {
        needToDraw = false;
    }

    // If the color frame is more than half a depth frame ahead of the depth frame we have,
    // then we should wait for another depth frame.  Otherwise, just go with what we have.
    if (m_colorTimeStamp.QuadPart - m_depthTimeStamp.QuadPart > halfADepthFrameMs)
    {
        needToDraw = false;
    }

    if (needToDraw)
    {
        int outputIndex = 0;
        LONG* pDest;
        LONG* pSrc;
		bool transparent;

        // loop over each row and column of the color
        for (LONG y = 0; y < m_colorHeight; ++y)
        {
            for (LONG x = 0; x < m_colorWidth; ++x)
            {
                // calculate index into depth array
                int depthIndex = x/m_colorToDepthDivisor + y/m_colorToDepthDivisor * m_depthWidth;

                USHORT depth  = m_depthD16[depthIndex];
                USHORT player = NuiDepthPixelToPlayerIndex(depth);

                // Changed this to use a 'transparency' flag
                pSrc  = NULL;
				transparent = true;

                // if we're tracking a player for the current pixel, draw from the color camera
                if ( player > 0 )
                {
                    // retrieve the depth to color mapping for the current depth pixel
                    LONG colorInDepthX = m_colorCoordinates[depthIndex * 2];
                    LONG colorInDepthY = m_colorCoordinates[depthIndex * 2 + 1];

                    // make sure the depth pixel maps to a valid point in color space
                    if ( colorInDepthX >= 0 && colorInDepthX < m_colorWidth && colorInDepthY >= 0 && colorInDepthY < m_colorHeight )
                    {
                        // calculate index into color array
                        LONG colorIndex = colorInDepthX + colorInDepthY * m_colorWidth;

                        // set source for copy to the color pixel
                        pSrc  = (LONG *)m_colorRGBX + colorIndex;
						transparent = false;
                    }
                }

                // calculate output pixel location
				pDest = (LONG *)m_outputRGBX + outputIndex++;

                // write output. If the pixel is transparent, set it to the TRANSPARENCY macro
				if ( !transparent )
					*pDest = *pSrc;
				else
					*pDest = TRANSPARENCY;
            }
        }

        // Draw the data with Direct2D
        m_pDrawGreenScreen->Draw(m_outputRGBX, m_feetPoints );
    }
}

/// <summary>
/// Handles window messages, passes most to the class instance to handle
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CGreenScreen::MessageRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CGreenScreen* pThis = NULL;
    
    if (WM_INITDIALOG == uMsg)
    {
        pThis = reinterpret_cast<CGreenScreen*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<CGreenScreen*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->DlgProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

/// <summary>
/// Handle windows messages for the class instance
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CGreenScreen::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Bind application window handle
            m_hWnd = hWnd;

            // Create and initialize a new Direct2D image renderer (take a look at ImageRenderer.h)
            // We'll use this to draw the data we receive from the Kinect to the screen
            m_pDrawGreenScreen = new ImageRenderer();

            HRESULT hr = m_pDrawGreenScreen->Initialize(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), m_colorWidth, m_colorHeight, m_colorWidth * sizeof(long) );
            if (FAILED(hr))
            {
                SetStatusMessage(L"Failed to initialize the OpenGL draw device.");
            }

            // Look for a connected Kinect, and create it if found
            CreateFirstConnected();
        }
        break;

        // If the titlebar X is clicked, destroy app
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            // Quit the main message pump
            PostQuitMessage(0);
            break;

        // Handle button press
        case WM_COMMAND:
            // If it was for the near mode control and a clicked event, change near mode
            if (IDC_CHECK_NEARMODE == LOWORD(wParam) && BN_CLICKED == HIWORD(wParam))
            {
                // Toggle out internal state for near mode
                m_bNearMode = !m_bNearMode;

                if (NULL != m_pNuiSensor)
                {
                    // Set near mode based on our internal state
                    m_pNuiSensor->NuiImageStreamSetImageFrameFlags(m_pDepthStreamHandle, m_bNearMode ? NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE : 0);
                }
            }
            break;
    }

    return FALSE;
}

/// <summary>
/// Create the first connected Kinect found 
/// </summary>
/// <returns>S_OK on success, otherwise failure code</returns>
HRESULT CGreenScreen::CreateFirstConnected()
{
    INuiSensor * pNuiSensor;
    HRESULT hr;

    int iSensorCount = 0;
    hr = NuiGetSensorCount(&iSensorCount);
    if (FAILED(hr))
    {
        return hr;
    }

    // Look at each Kinect sensor
    for (int i = 0; i < iSensorCount; ++i)
    {
        // Create the sensor so we can check status, if we can't create it, move on to the next
        hr = NuiCreateSensorByIndex(i, &pNuiSensor);
        if (FAILED(hr))
        {
            continue;
        }

        // Get the status of the sensor, and if connected, then we can initialize it
        hr = pNuiSensor->NuiStatus();
        if (S_OK == hr)
        {
            m_pNuiSensor = pNuiSensor;
            break;
        }

        // This sensor wasn't OK, so release it since we're not using it
        pNuiSensor->Release();
    }

    if (NULL != m_pNuiSensor)
    {
        // Initialize the Kinect and specify that we'll be using depth
        hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_COLOR
			| NUI_INITIALIZE_FLAG_USES_SKELETON); 
        if (SUCCEEDED(hr))
        {
            // Create an event that will be signaled when depth data is available
            m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            // Open a depth image stream to receive depth frames
            hr = m_pNuiSensor->NuiImageStreamOpen(
                NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX,
                cDepthResolution,
                0,
                2,
                m_hNextDepthFrameEvent,
                &m_pDepthStreamHandle);

            // Create an event that will be signaled when color data is available
            m_hNextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            // Open a color image stream to receive depth frames
            hr = m_pNuiSensor->NuiImageStreamOpen(
                NUI_IMAGE_TYPE_COLOR,
                cColorResolution,
                0,
                2,
                m_hNextColorFrameEvent,
                &m_pColorStreamHandle);

			// Create an event that will be signaled when skeleton data is available
            m_hNextSkeletonEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

            // Open a skeleton stream to receive skeleton data
            hr = m_pNuiSensor->NuiSkeletonTrackingEnable(m_hNextSkeletonEvent, 0); 
        }
    }

    if (NULL == m_pNuiSensor || FAILED(hr))
    {
        SetStatusMessage(L"No ready Kinect found!");
        return E_FAIL;
    }

    //m_pNuiSensor->NuiSkeletonTrackingDisable();

    return hr;
}

/// <summary>
/// Handle new depth data
/// </summary>
/// <returns>S_OK on success, otherwise failure code</returns>
HRESULT CGreenScreen::ProcessDepth()
{
    HRESULT hr = S_OK;
    NUI_IMAGE_FRAME imageFrame;

    // Attempt to get the depth frame
    hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pDepthStreamHandle, 0, &imageFrame);
    if (FAILED(hr))
    {
        return hr;
    }

    m_depthTimeStamp = imageFrame.liTimeStamp;

    INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;

    // Lock the frame data so the Kinect knows not to modify it while we're reading it
    pTexture->LockRect(0, &LockedRect, NULL, 0);

    // Make sure we've received valid data
    if (LockedRect.Pitch != 0)
    {
        memcpy(m_depthD16, LockedRect.pBits, LockedRect.size);
    }

    // We're done with the texture so unlock it
    pTexture->UnlockRect(0);

    // Release the frame
    m_pNuiSensor->NuiImageStreamReleaseFrame(m_pDepthStreamHandle, &imageFrame);

    // Get of x, y coordinates for color in depth space
    // This will allow us to later compensate for the differences in location, angle, etc between the depth and color cameras
    m_pNuiSensor->NuiImageGetColorPixelCoordinateFrameFromDepthPixelFrameAtResolution(
        cColorResolution,
        cDepthResolution,
        m_depthWidth*m_depthHeight,
        m_depthD16,
        m_depthWidth*m_depthHeight*2,
        m_colorCoordinates
        );

    return hr;
}

/// <summary>
/// Handle new color data
/// </summary>
/// <returns>S_OK for success or error code</returns>
HRESULT CGreenScreen::ProcessColor()
{
    HRESULT hr = S_OK;
    NUI_IMAGE_FRAME imageFrame;

    // Attempt to get the depth frame
    hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pColorStreamHandle, 0, &imageFrame);
    if (FAILED(hr))
    {
        return hr;
    }

    m_colorTimeStamp = imageFrame.liTimeStamp;

    INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;

    // Lock the frame data so the Kinect knows not to modify it while we're reading it
    pTexture->LockRect(0, &LockedRect, NULL, 0);

    // Make sure we've received valid data
    if (LockedRect.Pitch != 0)
    {
        memcpy(m_colorRGBX, LockedRect.pBits, LockedRect.size);
    }

    // We're done with the texture so unlock it
    pTexture->UnlockRect(0);

    // Release the frame
    m_pNuiSensor->NuiImageStreamReleaseFrame(m_pColorStreamHandle, &imageFrame);

    return hr;
}


/// <summary>
/// Set the status bar message
/// </summary>
/// <param name="szMessage">message to display</param>
void CGreenScreen::SetStatusMessage(WCHAR * szMessage)
{
    SendDlgItemMessageW(m_hWnd, IDC_STATUS, WM_SETTEXT, 0, (LPARAM)szMessage);
}


// SKELETON FUNCTIONS
/// <summary>
/// Handle new skeleton data
/// </summary>
void CGreenScreen::ProcessSkeleton()
{
	RECT rct;
	GetClientRect(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), &rct);
	int windowWidth = rct.right;
	int windowHeight = rct.bottom;

    NUI_SKELETON_FRAME skeletonFrame = {0};

    HRESULT hr = m_pNuiSensor->NuiSkeletonGetNextFrame(0, &skeletonFrame);
    if ( FAILED(hr) )
    {
        return;
    }

    // smooth out the skeleton data
    m_pNuiSensor->NuiTransformSmooth(&skeletonFrame, NULL);

	// ASSIGN SKELETONS SO IT CAN BE PASSED TO DRAW AND HANDLE FUNCTION
	tempSkeletonFrame = skeletonFrame;

	// Reset feet points. In case they aren't being tracked, points at (0,0) aren't drawn
	for ( int i=0; i<4; i++ )
			m_feetPoints[i] = Point2f( 0.0f, 0.0f );

	// hardcoded, two skeletons tracked
	for ( int skelIndex=0; skelIndex<2; skelIndex++ ){
		NUI_SKELETON_DATA& skel = skeletonFrame.SkeletonData[skelIndex];
		
		// calculate all of the joint points, just in case we want them
		for (int i = 0; i < NUI_SKELETON_POSITION_COUNT; ++i)
		{
			m_Points[i] = SkeletonToScreen(skel.SkeletonPositions[i], windowWidth, windowHeight);
		}

		NUI_SKELETON_POSITION_TRACKING_STATE rightFootState =
			skeletonFrame.SkeletonData[skelIndex].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_FOOT_RIGHT];
		NUI_SKELETON_POSITION_TRACKING_STATE leftFootState =
			skeletonFrame.SkeletonData[skelIndex].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_FOOT_LEFT];

		if (rightFootState == NUI_SKELETON_POSITION_TRACKED || rightFootState == NUI_SKELETON_POSITION_INFERRED)
		{
			m_feetPoints[skelIndex*2] = Point2f(
				m_Points[NUI_SKELETON_POSITION_FOOT_RIGHT].x,
				m_Points[NUI_SKELETON_POSITION_FOOT_RIGHT].y
				);
		}
		if (leftFootState == NUI_SKELETON_POSITION_TRACKED || leftFootState == NUI_SKELETON_POSITION_INFERRED)
		{
			m_feetPoints[(skelIndex*2)+1] = Point2f(
					m_Points[NUI_SKELETON_POSITION_FOOT_LEFT].x,
					m_Points[NUI_SKELETON_POSITION_FOOT_LEFT].y
					);
		}
		
	}
}

/// <summary>
/// Converts a skeleton point to screen space
/// </summary>
/// <param name="skeletonPoint">skeleton point to tranform</param>
/// <param name="width">width (in pixels) of output buffer</param>
/// <param name="height">height (in pixels) of output buffer</param>
/// <returns>point in screen-space</returns>
Point2f CGreenScreen::SkeletonToScreen(Vector4 skeletonPoint, int width, int height)
{
    LONG x, y;
    USHORT depth;
	Point2f pt;

    // Calculate the skeleton's position on the screen
    // NuiTransformSkeletonToDepthImage returns coordinates in NUI_IMAGE_RESOLUTION_320x240 space
    NuiTransformSkeletonToDepthImage(skeletonPoint, &x, &y, &depth);

    pt.x = static_cast<float>(x * width) / cScreenWidth;
    pt.y = static_cast<float>(y * height) / cScreenHeight;

    return pt;
}