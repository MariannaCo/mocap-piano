//------------------------------------------------------------------------------
// <copyright file="ImageRenderer.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "ImageRenderer.h"
#include <iostream>
#include <sstream>
#include <cwchar>
using namespace std;
#pragma comment(lib, "dwrite.lib")

/// <summary>
/// Constructor
/// </summary>
ImageRenderer::ImageRenderer() : 
    m_hWnd(0),
    m_sourceWidth(0),
    m_sourceHeight(0),
    m_sourceStride(0),
    m_pD2DFactory(NULL), 
    m_pRenderTarget(NULL),
    m_pBitmap(0)
{
}

/// <summary>
/// Destructor
/// </summary>
ImageRenderer::~ImageRenderer()
{
    DiscardResources();
    SafeRelease(m_pD2DFactory);
}

/// <summary>
/// Ensure necessary Direct2d resources are created
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT ImageRenderer::EnsureResources()
{
    HRESULT hr = S_OK;

    if (NULL == m_pRenderTarget)
    {
        D2D1_SIZE_U size = D2D1::SizeU(m_sourceWidth, m_sourceHeight);

        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
        rtProps.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
        rtProps.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

        // Create a hWnd render target, in order to render to the window set in initialize
        hr = m_pD2DFactory->CreateHwndRenderTarget(
            rtProps,
            D2D1::HwndRenderTargetProperties(m_hWnd, size),
            &m_pRenderTarget
            );

        if ( FAILED(hr) )
        {
            return hr;
        }

        // Create a bitmap that we can copy image data into and then render to the target
        hr = m_pRenderTarget->CreateBitmap(
            size, 
            D2D1::BitmapProperties( D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE) ),
            &m_pBitmap 
            );

        if ( FAILED(hr) )
        {
            SafeRelease(m_pRenderTarget);
            return hr;
        }

		// CREATE THE COLORED BRUSHES
		m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(255, 0, 0), &redBrush);
		m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 255), &blueBrush);

		// CUSTOM INITIALIZATION FOR TEXT
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&pDWriteFactory)
		);

		pDWriteFactory->CreateTextFormat(
			L"Gabriola",
	        NULL,
	        DWRITE_FONT_WEIGHT_REGULAR,
	        DWRITE_FONT_STYLE_NORMAL,
	        DWRITE_FONT_STRETCH_NORMAL,
	        30.0f,
	        L"en-us",
	        &pTextFormat
        );
		pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

		pDWriteFactory->CreateTextFormat(
	        L"Gabriola",
	        NULL,
	        DWRITE_FONT_WEIGHT_REGULAR,
	        DWRITE_FONT_STYLE_NORMAL,
	        DWRITE_FONT_STRETCH_NORMAL,
	        120.0f,
	        L"en-us",
	        &bigTextFormat
        );
		bigTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

		// SET THE TEXT RECTANGLES
		textRect = D2D1::RectF( 10.0f, 0.0f, m_sourceWidth-10.0f, 60.0f );
		bigTextRect = D2D1::RectF( 10.0f, m_sourceHeight/2 - 100.0f,
			m_sourceWidth-10.0f, m_sourceHeight/2 + 100.0f );

		// SET THE KEY RECTANGLES
		int keyWidth = m_sourceWidth/numKeys;

		for(int i = 0; i < numKeys; i++)
		{
			keyRects[i] = D2D1::RectF((float)(keyWidth*i), (float)(m_sourceHeight-40),
				(float)(keyWidth*i+keyWidth), (float)(m_sourceHeight));
		}
	}

    return hr;
}

/// <summary>
/// Dispose of Direct2d resources 
/// </summary>
void ImageRenderer::DiscardResources()
{
    SafeRelease(m_pRenderTarget);
    SafeRelease(m_pBitmap);
}

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
HRESULT ImageRenderer::Initialize(HWND hWnd, ID2D1Factory* pD2DFactory, int sourceWidth, int sourceHeight, int sourceStride)
{
    if (NULL == pD2DFactory)
    {
        return E_INVALIDARG;
    }

    m_hWnd = hWnd;

    // One factory for the entire application so save a pointer here
    m_pD2DFactory = pD2DFactory;

    m_pD2DFactory->AddRef();

    // Get the frame size
    m_sourceWidth  = sourceWidth;
    m_sourceHeight = sourceHeight;
    m_sourceStride = sourceStride;

    return S_OK;
}

/// <summary>
/// Draws a 32 bit per pixel image of previously specified width, height, and stride to the associated hwnd
/// </summary>
/// <param name="pImage">image data in RGBX format</param>
/// <param name="cbImage">size of image data in bytes</param>
/// <returns>indicates success or failure</returns>
HRESULT ImageRenderer::Draw(BYTE* pImage, unsigned long cbImage, NUI_SKELETON_FRAME tempSkeletonFrame, bool handleSkeletons)
{
    // incorrectly sized image data passed in
    if ( cbImage < ((m_sourceHeight - 1) * m_sourceStride) + (m_sourceWidth * 4) )
    {
        return E_INVALIDARG;
    }

    // create the resources for this draw device
    // they will be recreated if previously lost
    HRESULT hr = EnsureResources();

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    // Copy the image that was passed in into the direct2d bitmap
    hr = m_pBitmap->CopyFromMemory(NULL, pImage, m_sourceStride);

    if ( FAILED(hr) )
    {
        return hr;
    }
       
    m_pRenderTarget->BeginDraw();

    // Draw the bitmap stretched to the size of the window
    m_pRenderTarget->DrawBitmap(m_pBitmap);

	// Draw the key outlines
	for(int i = 0; i < numKeys; i++)
	{
		m_pRenderTarget->DrawRectangle(keyRects[i], blueBrush);
	}

	// SKELETON HANDLING ACTIVITY
	//if(handleSkeletons)
	{
		RECT rct = {0, 0, m_sourceWidth, m_sourceHeight};
		//GetClientRect( GetDlgItem( m_hWnd, IDC_VIDEOVIEW ), &rct);
		int width = rct.right;
		int height = rct.bottom;
		
		for (int i = 0 ; i < NUI_SKELETON_COUNT; ++i)
		{
			NUI_SKELETON_TRACKING_STATE trackingState = tempSkeletonFrame.SkeletonData[i].eTrackingState;
			
			if (NUI_SKELETON_TRACKED == trackingState)
			{
				// INSTEAD OF DRAWING THE SKELETON, HANDLE FOR FLOOR PIANO

				// UPDATE THE SKELETON POINTS // MAY ONLY NEED TO DO THE FEET FOR OUR PURPOSES
				for (int p = 0; p < NUI_SKELETON_POSITION_COUNT; ++p)
				{
					m_Points[p] = SkeletonToScreen(tempSkeletonFrame.SkeletonData[i].SkeletonPositions[p], width, height);
				}

				// GET THE JOINT STATES FOR EACH FEET
				NUI_SKELETON_POSITION_TRACKING_STATE rightFootState =
					tempSkeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_FOOT_RIGHT];
				NUI_SKELETON_POSITION_TRACKING_STATE leftFootState =
					tempSkeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_FOOT_LEFT];

				// IF EACH FOOT IS TRACKED OR INFERRED, DO STUFF WITH THE FOOT COORDINATES
				if (rightFootState == NUI_SKELETON_POSITION_TRACKED)// || rightFootState == NUI_SKELETON_POSITION_INFERRED)
				{
					D2D1_ELLIPSE ellipse = D2D1::Ellipse( m_Points[NUI_SKELETON_POSITION_FOOT_RIGHT], 10, 10 );
					m_pRenderTarget->FillEllipse(ellipse, redBrush);
				}
				if (leftFootState == NUI_SKELETON_POSITION_TRACKED)// || leftFootState == NUI_SKELETON_POSITION_INFERRED)
				{
					D2D1_ELLIPSE ellipse = D2D1::Ellipse( m_Points[NUI_SKELETON_POSITION_FOOT_LEFT], 10, 10 );
					m_pRenderTarget->FillEllipse(ellipse, redBrush);
				}
			}
		}
	}

/*
	// DRAW HOW MANY SKELETONS
	stringstream ss;
	ss << "NUI_SKELETON_COUNT: " << NUI_SKELETON_COUNT;

	string s = ss.str();
	WCHAR* text = new WCHAR[s.size()];
	for(string::size_type i=0; i<s.size(); ++i)
		text[i]=s[i];

	// DRAW THE SCORE AND LIVES TEXT
	m_pRenderTarget->DrawTextW(text, s.length(), pTextFormat, textRect, redBrush,
		D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
*/
            
    hr = m_pRenderTarget->EndDraw();

    // Device lost, need to recreate the render target
    // We'll dispose it now and retry drawing
    if (hr == D2DERR_RECREATE_TARGET)
    {
        hr = S_OK;
        DiscardResources();
    }

    return hr;
}

/// <summary>
/// Converts a skeleton point to screen space
/// </summary>
/// <param name="skeletonPoint">skeleton point to tranform</param>
/// <param name="width">width (in pixels) of output buffer</param>
/// <param name="height">height (in pixels) of output buffer</param>
/// <returns>point in screen-space</returns>
D2D1_POINT_2F ImageRenderer::SkeletonToScreen(Vector4 skeletonPoint, int width, int height)
{
    LONG x, y;
    USHORT depth;

    // Calculate the skeleton's position on the screen
    // NuiTransformSkeletonToDepthImage returns coordinates in NUI_IMAGE_RESOLUTION_320x240 space
    NuiTransformSkeletonToDepthImage(skeletonPoint, &x, &y, &depth);

    float screenPointX = static_cast<float>(x * width) / cScreenWidth;
    float screenPointY = static_cast<float>(y * height) / cScreenHeight;

    return D2D1::Point2F(screenPointX, screenPointY);
}
