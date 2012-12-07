//------------------------------------------------------------------------------
// <copyright file="ImageRenderer.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "ImageRenderer.h"
#include "SimpleMIDIPlayer.h"
#include <iostream>
#include <sstream>
#include <cwchar>
#include <Wincodec.h>

using namespace std;

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

/// <summary>
/// Constructor
/// </summary>
ImageRenderer::ImageRenderer()
{
	
}

/// <summary>
/// Destructor
/// </summary>
ImageRenderer::~ImageRenderer()
{

}

HRESULT ImageRenderer::Initialize( HWND hWnd, int sourceWidth, int sourceHeight, int sourceStride ){
	m_hWnd = hWnd;
	EnableOpenGL();

    m_sourceWidth  = sourceWidth;
    m_sourceHeight = sourceHeight;
    m_sourceStride = sourceStride;

	// Load the background image into a texture
	m_backgroundRGBX = new BYTE[m_sourceWidth*m_sourceHeight*sizeof(long)];
	LoadResourceImage(L"Background", L"Image", m_sourceWidth*m_sourceHeight*sizeof(long), m_backgroundRGBX);
	glGenTextures( 1, &m_bgTexture );
    glBindTexture(GL_TEXTURE_2D, m_bgTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_sourceWidth, m_sourceHeight, 0,  GL_BGRA, GL_UNSIGNED_BYTE, m_backgroundRGBX);


	// Create a texture for rendering frames to
	glGenTextures( 1, &m_frameTexture );
    glBindTexture(GL_TEXTURE_2D, m_frameTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_sourceWidth, m_sourceHeight, 0,  GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

	// Create a pixel buffer for the frame texture
	glewInit();
	glGenBuffers(0, &pboId);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboId);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, m_sourceWidth*m_sourceHeight*sizeof(long),0,GL_STREAM_DRAW);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	
    // OpenGL setup
    glClearColor(0,0,0,0);
    glClearDepth(1.0f);
    glEnable(GL_TEXTURE_2D);

    // Initial camera setup
    glViewport(0, 0, m_sourceWidth, m_sourceHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,  m_sourceWidth, m_sourceHeight, 0, 1, -1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	
	return S_OK;
}

/// <summary>
/// Draws a 32 bit per pixel image of previously specified width, height, and stride to the associated hwnd
/// </summary>
/// <param name="pImage">image data in RGBX format</param>
/// <param name="cbImage">size of image data in bytes</param>
/// <returns>indicates success or failure</returns>
HRESULT ImageRenderer::Draw(
	BYTE* pImage, Point2f feetPoints[4] 
	)
{
	RECT rct;
	GetClientRect( m_hWnd, &rct);
	int width = rct.right;
	int height = rct.bottom;

	// set the orthogonal projection
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, 1, -1);

	// clear the buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	// requires the orthogonal projection
	drawBG( width, height );

	// also requires the orthogonal projection
	drawPlayers( pImage, width, height );

	drawFootMarkers( feetPoints );
	
	// finished, swap buffers
	SwapBuffers( m_hDC );

	return S_OK;
}

void ImageRenderer::drawBG(int width, int height){
	glBindTexture(GL_TEXTURE_2D, m_bgTexture);
    glBegin(GL_QUADS);
       glTexCoord2f(0.0f, 0.0f);
        glVertex3f(0, 0, 0);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(width, 0, 0);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(width, height, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(0, height, 0.0f);
    glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void ImageRenderer::drawPlayers(BYTE* pImage, int width, int height){
	// use a pixel buffer to copy frame data using DMA
	glBindTexture(GL_TEXTURE_2D, m_frameTexture);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboId);

    // copy pixels from buffer to texture
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_sourceWidth, m_sourceHeight, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 0);

	// bind PBO to update pixel values
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboId);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, m_sourceWidth*m_sourceHeight*sizeof(long), 0, GL_STREAM_DRAW);
	
	GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if(ptr)
    {
        // update data directly on the mapped buffer
		int size = m_sourceWidth*m_sourceHeight;
        for(int i=0;i<size;i++){
			((long*)ptr)[i] = ((long*)pImage)[i];
		}
		
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer
    }

	// Use transparency to implement the 'greenscreen' 
	glEnable(GL_BLEND);
	glBlendFunc (GL_ONE_MINUS_SRC_ALPHA,GL_SRC_ALPHA);
	
    glBegin(GL_QUADS);
       glTexCoord2f(0.0f, 0.0f);
        glVertex3f(0, 0, 0);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(width, 0, 0);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(width, height, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(0, height, 0.0f);
    glEnd(); 

	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);

}

void ImageRenderer::drawFootMarkers( Point2f feetPoints[4] ){
	for ( int i=0; i<4; i++ ){
		if ( feetPoints[i].x != 0.0f && feetPoints[i].y != 0.0f ){
			circle( feetPoints[i].x, feetPoints[i].y, 10.0f, 10, 1.0f, 0.0f, 0.0f );
		}
	}
	//circle( 100.0f, 100.0f, 10.0f, 10, 1.0f, 0.0f, 0.0f );
}

// Enable OpenGL

void ImageRenderer::EnableOpenGL()
{
	PIXELFORMATDESCRIPTOR pfd;
	int format;
	
	// get the device context (DC)
	m_hDC = GetDC( m_hWnd );
	
	// set the pixel format for the DC
	ZeroMemory( &pfd, sizeof( pfd ) );
	pfd.nSize = sizeof( pfd );
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;
	format = ChoosePixelFormat( m_hDC, &pfd );
	SetPixelFormat( m_hDC, format, &pfd );
	
	// create and enable the render context (RC)
	m_hRC = wglCreateContext( m_hDC );
	wglMakeCurrent( m_hDC, m_hRC );
	
}

// Disable OpenGL

void ImageRenderer::DisableOpenGL()
{
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( m_hRC );
	ReleaseDC( m_hWnd, m_hDC );
}


HRESULT ImageRenderer::LoadResourceImage(
    PCWSTR resourceName,
    PCWSTR resourceType,
    DWORD cOutputBuffer,
    BYTE* outputBuffer
    )
{
    HRESULT hr = S_OK;

    IWICImagingFactory* pIWICFactory = NULL;
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pSource = NULL;
    IWICStream* pStream = NULL;
    IWICFormatConverter* pConverter = NULL;
    IWICBitmapScaler* pScaler = NULL;

    HRSRC imageResHandle = NULL;
    HGLOBAL imageResDataHandle = NULL;
    void *pImageFile = NULL;
    DWORD imageFileSize = 0;

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)&pIWICFactory);
    if ( FAILED(hr) ) return hr;

    // Locate the resource.
    imageResHandle = FindResourceW(HINST_THISCOMPONENT, resourceName, resourceType);
    hr = imageResHandle ? S_OK : E_FAIL;

    if (SUCCEEDED(hr))
    {
        // Load the resource.
        imageResDataHandle = LoadResource(HINST_THISCOMPONENT, imageResHandle);
        hr = imageResDataHandle ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // Lock it to get a system memory pointer.
        pImageFile = LockResource(imageResDataHandle);
        hr = pImageFile ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // Calculate the size.
        imageFileSize = SizeofResource(HINST_THISCOMPONENT, imageResHandle);
        hr = imageFileSize ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // Create a WIC stream to map onto the memory.
        hr = pIWICFactory->CreateStream(&pStream);
    }

    if (SUCCEEDED(hr))
    {
        // Initialize the stream with the memory pointer and size.
        hr = pStream->InitializeFromMemory(
            reinterpret_cast<BYTE*>(pImageFile),
            imageFileSize
            );
    }

    if (SUCCEEDED(hr))
    {
        // Create a decoder for the stream.
        hr = pIWICFactory->CreateDecoderFromStream(
            pStream,
            NULL,
            WICDecodeMetadataCacheOnLoad,
            &pDecoder
            );
    }

    if (SUCCEEDED(hr))
    {
        // Create the initial frame.
        hr = pDecoder->GetFrame(0, &pSource);
    }

    if (SUCCEEDED(hr))
    {
        // Convert the image format to 32bppPBGRA
        // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }

    if (SUCCEEDED(hr))
    {
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
    }

    if (SUCCEEDED(hr))
    {
        hr = pScaler->Initialize(
            pSource,
            m_sourceWidth,
            m_sourceHeight,
            WICBitmapInterpolationModeCubic
            );
    }

    if (SUCCEEDED(hr))
    {
        hr = pConverter->Initialize(
            pScaler,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.f,
            WICBitmapPaletteTypeMedianCut
            );
    }

    UINT width = 0;
    UINT height = 0;
    if (SUCCEEDED(hr))
    {
        hr = pConverter->GetSize(&width, &height);
    }

    // make sure the output buffer is large enough
    if (SUCCEEDED(hr))
    {
        if ( width*height*sizeof(long) > cOutputBuffer )
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pConverter->CopyPixels(NULL, width*sizeof(long), cOutputBuffer, outputBuffer);
    }

    SafeRelease(pScaler);
    SafeRelease(pConverter);
    SafeRelease(pSource);
    SafeRelease(pDecoder);
    SafeRelease(pStream);
    SafeRelease(pIWICFactory);

    return hr;
}

void ImageRenderer::circle(float x, float y, float r, int segments, float red, float green, float blue)
{
    glBegin( GL_TRIANGLE_FAN );
		glColor4f(red,green,blue, 1.0f);
        glVertex2f(x, y);
        for( int n = 0; n <= segments ; ++n ) {
            float const t = 2*3.14159*(float)n/(float)segments;
            glVertex2f(x + sin(t)*r, y + cos(t)*r);
        }
    glEnd();
}