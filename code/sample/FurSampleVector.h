// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
#pragma once

#include "DXUT.h"

/// Simple vector type.
/// We'd might have used std::vector, but we can't because we need allocations to be 16 byte aligned.
/// Doing so means we have no dependency on stl. 
/// Interface similar to std::vector except uses lowerCamel, and convention types always begin with capital.
/// (ie push_back => pushBack)
/// (iterator => Iterator)
template <typename T>
class FurSample_Vector
{
public:
	typedef FurSample_Vector ThisType;
	
	typedef T* Iterator;
	typedef const T* ConstIterator;
	typedef size_t SizeType;

		/// Get the size
	size_t getSize() const { return m_size;  }
		/// Change the size
	void setSize(size_t size);

		/// Clear contents 
	void clear();
		/// Clear and deallocate
	void clearAndDeallocate();

		/// Add to back
	void pushBack(const T& v);

		/// []
	const T& operator[](size_t index) const { assert(index < m_size); return m_data[index]; }
	T& operator[](size_t index) { assert(index < m_size); return m_data[index]; }

		/// Access contents
	T* data() { return m_data; }
	const T* data() const { return m_data; }

		/// begin
	ConstIterator begin() const { return m_data;  }
	Iterator begin() { return m_data;  }

		/// end
	ConstIterator end() const { return m_data + m_size; }
	Iterator end() { return m_data + m_size; }

		/// Assignment
	FurSample_Vector& operator=(const ThisType& rhs);
		
		/// ==
	bool operator==(const ThisType& rhs) const;
		/// !=
	bool operator!=(const ThisType& rhs) const { return !(*this == rhs); }

	/// Ctor
	FurSample_Vector() :m_data(nullptr), m_size(0), m_capacity(0) {}
	~FurSample_Vector();

	/// Ctor with a size and a fill
	explicit FurSample_Vector(size_t initialSize, const T& fillValue);
	/// Copy Ctor
	FurSample_Vector(const ThisType& rhs);


protected:
	T* m_data;
	size_t m_size;
	size_t m_capacity;
};

// ---------------------------------------------------------------------------
template <typename T>
FurSample_Vector<T>::~FurSample_Vector()
{
	if (m_data)
	{
		// Run dtors
		for (size_t i = 0; i < m_size; i++)
		{
			(m_data + i)->~T();
		}
		// Free
		_aligned_free(m_data);
	}
}
// ---------------------------------------------------------------------------
template <typename T>
FurSample_Vector<T>::FurSample_Vector(size_t initialSize, const T& fillValue)
{
	if (initialSize > 0)
	{
		m_data = (T*)_aligned_alloc(initialSize * sizeof(T), 16);
		m_size = initialSize;
		m_capacity = initialSize;

		for (size_t i = 0; i < initialSize; ++i)
		{
			new(m_data + i) T(fillValue);
		}
	}
	else
	{
		m_data = nullptr;
		m_size = 0;
		m_capacity = 0;
	}
}
// ---------------------------------------------------------------------------
template <typename T>
void FurSample_Vector<T>::setSize(size_t size)
{
	if (size < m_size)
	{
		for (size_t i = size; i < m_size; i++)
		{
			(m_data + i)->~T();
		}
	}
	else if (size > m_size)
	{
		if (size > m_capacity)
		{
			m_data = (T*)_aligned_realloc(m_data, sizeof(T) * size, 16);
			m_capacity = size;
		}
		for (size_t i = m_size; i < size; ++i)
		{
			new(m_data + i) T;
		}
	}
	m_size = size;
}

// ---------------------------------------------------------------------------
template <typename T>
void FurSample_Vector<T>::clear()
{
	for (size_t i = 0; i < m_size; i++)
	{
		(m_data + i)->~T();
	}
	m_size = 0;
}
// ---------------------------------------------------------------------------
template <typename T>
void FurSample_Vector<T>::clearAndDeallocate()
{
	clear();
	if (m_data)
	{
		_aligned_free(m_data);
		m_capacity = 0;
	}
}

// ---------------------------------------------------------------------------
template <typename T>
FurSample_Vector<T>::FurSample_Vector(const ThisType& rhs):
	m_size(rhs.m_size),
	m_capacity(rhs.m_capacity)
{
	if (m_size > 0)
	{
		m_data = (T*)_aligned_alloc(sizeof(T) * m_size);
		for (size_t i = 0; i < m_size; i++)
		{
			new (m_data + i) T(rhs.m_data[i]);
		}
	}
}

// ---------------------------------------------------------------------------
template <typename T>
FurSample_Vector<T>& FurSample_Vector<T>::operator=(const ThisType& rhs)
{
	if (this == &rhs)
	{
		return *this;
	}
	if (rhs.m_size > m_capacity)
	{
		// We need some more space
		m_data = (T*)_aligned_realloc(m_data, sizeof(T) * rhs.m_size, 16);
		m_capacity = rhs.m_size;
	}
	if (rhs.m_size < m_size)
	{
		// Make smaller
		for (size_t i = rhs.m_size; i < m_size; i++)
		{
			(m_data + i)->~T();
		}
		m_size = rhs.m_size;
	}
	// Everything up to m_size is ctored, so just assign
	for (size_t i = 0; i < m_size; i++)
	{
		m_data[i] = rhs.m_data[i];
	}
	// Copy Ctor the rest
	for (size_t i = m_size; i < rhs.m_size; i++)
	{
		new (m_data + i) T(rhs.m_data[i]);
	}
	// Set the size
	m_size = size;
}

// ---------------------------------------------------------------------------
template <typename T>
bool FurSample_Vector<T>::operator==(const ThisType& rhs) const
{
	if (this == &rhs)
	{
		return true;
	}
	if (m_size != rhs.m_size)
	{
		return false;
	}
	for (size_t i = 0; i < m_size; i++)
	{
		// Use operator == to do test, as more common to impl
		if (!(m_data[i] == rhs.m_data[i]))
		{
			return false;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
template <typename T>
void FurSample_Vector<T>::pushBack(const T& v)
{
	if (m_size >= m_capacity)
	{
		// Going to need some more capacity. Lets go 1/2 space again
		size_t incSize = m_size / 2;
		// With a minimum of 16 elements
		incSize = (incSize < 16) ? 16 : incSize;
		// Allocate
		m_data = (T*)_aligned_realloc(m_data, sizeof(T) * (m_capacity + incSize), 16);
		m_capacity += incSize;
	}
	new (m_data + m_size) T(v);
	m_size++;
}
