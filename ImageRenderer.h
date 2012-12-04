﻿//------------------------------------------------------------------------------
// <copyright file="ImageRenderer.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// Manages the drawing of image data

#pragma once

#include <d2d1.h>
#include <dwrite.h>
// FOR SKELETON DATA
#include "NuiApi.h"

class ImageRenderer
{
	static const int numKeys = 10;

public:
    /// <summary>
    /// Constructor
    /// </summary>
    ImageRenderer();

    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~ImageRenderer();

    /// <summary>
    /// Set the window to draw to as well as the video format
    /// Implied bits per pixel is 32
    /// </summary>
    /// <param name="hWnd">window to draw to</param>
    /// <param name="pD2DFactory">already created D2D factory object</param>
    /// <param name="sourceWidth">width (in pixels) of image data to be drawn</param>
    /// <param name="sourceHeight">height (in pixels) of image data to be drawn</param>
    /// <param name="sourceStride">length (in bytes) of a single scanline</param>
    /// <returns>indicates success or failure</returns>
    HRESULT Initialize(HWND hwnd, ID2D1Factory* pD2DFactory, int sourceWidth, int sourceHeight, int sourceStride);

    /// <summary>
    /// Draws a 32 bit per pixel image of previously specified width, height, and stride to the associated hwnd
    /// </summary>
    /// <param name="pImage">image data in RGBX format</param>
    /// <param name="cbImage">size of image data in bytes</param>
    /// <returns>indicates success or failure</returns>
    HRESULT Draw(BYTE* pImage, unsigned long cbImage, NUI_SKELETON_FRAME tempSkeletonFrame, bool handleSkeletons);

	D2D1_POINT_2F SkeletonToScreen(Vector4 skeletonPoint, int width, int height);

	// PIANO KEYS
	D2D1_RECT_F keyRects[numKeys];
	bool keysPressed[numKeys];
	bool footOnKey[numKeys];

private:
    HWND                     m_hWnd;

    // Format information
    UINT                     m_sourceHeight;
    UINT                     m_sourceWidth;
    LONG                     m_sourceStride;

     // Direct2D 
    ID2D1Factory*            m_pD2DFactory;
    ID2D1HwndRenderTarget*   m_pRenderTarget;
    ID2D1Bitmap*             m_pBitmap;

	ID2D1SolidColorBrush*    redBrush;
	ID2D1SolidColorBrush*	 blueBrush;

	// SKELETON USE
	D2D1_POINT_2F            m_Points[NUI_SKELETON_POSITION_COUNT];
	// FROM SKELETONBASICS.H
    static const int        cScreenWidth  = 320;
    static const int        cScreenHeight = 240;

	// VARIABLES FOR TEXT USE
	IDWriteFactory*			pDWriteFactory;
	IDWriteTextFormat*		pTextFormat;
	IDWriteTextFormat*		bigTextFormat;
	D2D1_RECT_F				textRect, bigTextRect;

    /// <summary>
    /// Ensure necessary Direct2d resources are created
    /// </summary>
    /// <returns>indicates success or failure</returns>
    HRESULT EnsureResources( );

    /// <summary>
    /// Dispose of Direct2d resources 
    /// </summary>
    void DiscardResources( );
};