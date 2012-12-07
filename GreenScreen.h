//------------------------------------------------------------------------------
// <copyright file="GreenScreen.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "resource.h"
#include "NuiApi.h"
#include "ImageRenderer.h"

#include <gl/GL.h>
#include "types.h"

class CGreenScreen
{
    static const int        cBytesPerPixel    = 4;

    static const NUI_IMAGE_RESOLUTION cDepthResolution = NUI_IMAGE_RESOLUTION_320x240;
    
    // green screen background will also be scaled to this resolution
    static const NUI_IMAGE_RESOLUTION cColorResolution = NUI_IMAGE_RESOLUTION_640x480;

    static const int        cStatusMessageMaxLen = MAX_PATH*2;

	// FROM SKELETONBASICS.H
    static const int        cScreenWidth  = 320;
    static const int        cScreenHeight = 240;

public:
    /// <summary>
    /// Constructor
    /// </summary>
    CGreenScreen();

    /// <summary>
    /// Destructor
    /// </summary>
    ~CGreenScreen();

    /// <summary>
    /// Handles window messages, passes most to the class instance to handle
    /// </summary>
    /// <param name="hWnd">window message is for</param>
    /// <param name="uMsg">message</param>
    /// <param name="wParam">message data</param>
    /// <param name="lParam">additional message data</param>
    /// <returns>result of message processing</returns>
    static LRESULT CALLBACK MessageRouter(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// <summary>
    /// Handle windows messages for a class instance
    /// </summary>
    /// <param name="hWnd">window message is for</param>
    /// <param name="uMsg">message</param>
    /// <param name="wParam">message data</param>
    /// <param name="lParam">additional message data</param>
    /// <returns>result of message processing</returns>
    LRESULT CALLBACK        DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// <summary>
    /// Creates the main window and begins processing
    /// </summary>
    /// <param name="hInstance"></param>
    /// <param name="nCmdShow"></param>
    int                     Run(HINSTANCE hInstance, int nCmdShow);

private:
    HWND                    m_hWnd;

    bool                    m_bNearMode;

	// FROM SKELETONBASICS.H
	bool                    m_bSeatedMode;
	// Skeletal drawing
    Point2f            m_Points[NUI_SKELETON_POSITION_COUNT];
	Point2f				m_feetPoints[4];

	bool					handleSkeletons;
	NUI_SKELETON_FRAME tempSkeletonFrame;

    // Current Kinect
    INuiSensor*             m_pNuiSensor;

    // Direct2D
    ImageRenderer*          m_pDrawGreenScreen;
    
    HANDLE                  m_pDepthStreamHandle;
    HANDLE                  m_hNextDepthFrameEvent;

    HANDLE                  m_pColorStreamHandle;
    HANDLE                  m_hNextColorFrameEvent;

	// FROM SKELETONBASICS.H
	HANDLE                  m_pSkeletonStreamHandle;
    HANDLE                  m_hNextSkeletonEvent;

    LONG                    m_depthWidth;
    LONG                    m_depthHeight;

    LONG                    m_colorWidth;
    LONG                    m_colorHeight;

    LONG                    m_colorToDepthDivisor;

    USHORT*                 m_depthD16;
    BYTE*                   m_colorRGBX;
    BYTE*                   m_outputRGBX;

    LONG*                   m_colorCoordinates;

    LARGE_INTEGER           m_depthTimeStamp;
    LARGE_INTEGER           m_colorTimeStamp;

    /// <summary>
    /// Load an image from a resource into a buffer
    /// </summary>
    /// <param name="resourceName">name of image resource to load</param>
    /// <param name="resourceType">type of resource to load</param>
    /// <param name="cOutputBuffer">size of output buffer, in bytes</param>
    /// <param name="outputBuffer">buffer that will hold the loaded image</param>
    /// <returns>S_OK on success, otherwise failure code</returns>
    HRESULT                 LoadResourceImage(PCWSTR resourceName, PCWSTR resourceType, DWORD cOutputBuffer, BYTE* outputBuffer);

    /// <summary>
    /// Main processing function
    /// </summary>
    void                    Update();

    /// <summary>
    /// Create the first connected Kinect found 
    /// </summary>
    /// <returns>S_OK on success, otherwise failure code</returns>
    HRESULT                 CreateFirstConnected();

    /// <summary>
    /// Handle new depth data
    /// </summary>
    /// <returns>S_OK on success, otherwise failure code</returns>
    HRESULT                 ProcessDepth();

    /// <summary>
    /// Handle new color data
    /// </summary>
    /// <returns>S_OK on success, otherwise failure code</returns>
    HRESULT                 ProcessColor();

    /// <summary>
    /// Set the status bar message
    /// </summary>
    /// <param name="szMessage">message to display</param>
    void                    SetStatusMessage(WCHAR* szMessage);

	// FROM SKELETONBASICS.H
	    /// <summary>
    /// Handle new skeleton data
    /// </summary>
    void                    ProcessSkeleton();

    /// <summary>
    /// Draws a bone line between two joints
    /// </summary>
    /// <param name="skel">skeleton to draw bones from</param>
    /// <param name="joint0">joint to start drawing from</param>
    /// <param name="joint1">joint to end drawing at</param>
    void                    DrawBone(const NUI_SKELETON_DATA & skel, NUI_SKELETON_POSITION_INDEX bone0, NUI_SKELETON_POSITION_INDEX bone1);

    /// <summary>
    /// Draws a skeleton
    /// </summary>
    /// <param name="skel">skeleton to draw</param>
    /// <param name="windowWidth">width (in pixels) of output buffer</param>
    /// <param name="windowHeight">height (in pixels) of output buffer</param>
    void                    DrawSkeleton(const NUI_SKELETON_DATA & skel, int windowWidth, int windowHeight);

    /// <summary>
    /// Converts a skeleton point to screen space
    /// </summary>
    /// <param name="skeletonPoint">skeleton point to tranform</param>
    /// <param name="width">width (in pixels) of output buffer</param>
    /// <param name="height">height (in pixels) of output buffer</param>
    /// <returns>point in screen-space</returns>
    Point2f           SkeletonToScreen(Vector4 skeletonPoint, int width, int height);
};
