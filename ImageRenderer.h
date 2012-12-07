//------------------------------------------------------------------------------
// <copyright file="ImageRenderer.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// Manages the drawing of image data

#pragma once

#include "resource.h"
#include "NuiApi.h"
#include "gl/glew.h"
#include "gl/GLU.h"
#include "gl/glut.h"

#include "gl/GL.h"

#include "types.h"

#define TRANSPARENCY	0x00000000ff000000

class ImageRenderer
{

public:
    /// <summary>
    /// Constructor
    /// </summary>
    ImageRenderer();

	HRESULT Initialize( HWND hWnd, int sourceWidth, int sourceHeight, int sourceStride );

	HRESULT Draw(BYTE* pImage, 
		Point2f feetPoints[4]);

    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~ImageRenderer();	

private:
	HWND		m_hWnd;
	HDC			m_hDC;
	HGLRC		m_hRC;

	int m_sourceWidth;
	int m_sourceHeight;
	int m_sourceStride;

	GLuint m_frameTexture;
	GLuint m_bgTexture;

	// Use a pixel buffer for DMA transfer of video frames
	GLuint pboId;
	GLubyte* m_pPixelBuffer;

	BYTE* m_backgroundRGBX;

	// OpenGL initialization & cleanup
	void EnableOpenGL();

	void DisableOpenGL();

	// Drawing components
	void drawBG(int width, int height);
	void drawPlayers(BYTE* pImage, int width, int height);
	void drawFootMarkers(Point2f feetPoints[4]);

	// Draw basic stuff
	void circle(float x, float y, float r, int segments, float red, float green, float blue);

	// Loading the background...
	HRESULT LoadResourceImage( PCWSTR resourceName, PCWSTR resourceType, DWORD cOutputBuffer, BYTE* outputBuffer );
};