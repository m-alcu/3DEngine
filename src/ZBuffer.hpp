#pragma once

#include <limits>
#include <cassert>
#include <algorithm>

class ZBuffer
{
public:
	ZBuffer( int width, int height )
		:
		width( width ),
		height( height ),
		pBuffer( new float[width*height] )
	{}
	ZBuffer( const ZBuffer& ) = delete;
	ZBuffer& operator=( const ZBuffer& ) = delete;
	~ZBuffer()
	{
		delete[] pBuffer;
		pBuffer = nullptr;
	}
	void Clear()
	{
		std::fill_n(
			pBuffer,
			width * height,
			1.0f // Maximum depth in normalized device coordinates (NDC)
		);
	}
	void Resize( int w,int h )
	{
		if( w == width && h == height ) return;
		delete[] pBuffer;
		width = w;
		height = h;
		pBuffer = new float[width * height];
	}
	bool TestAndSet( int pos, float depth )
	{
		float& depthInBuffer = pBuffer[pos];
		if( depth < depthInBuffer )
		{
			depthInBuffer = depth;
			return true;
		}
		return false;
	}
	void Set( int pos,float depth )
	{
		pBuffer[pos] = depth;
	}
	float Get( int pos ) const
	{
		return pBuffer[pos];
	}
private:
	int width;
	int height;
	float* pBuffer = nullptr;
public:
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
};