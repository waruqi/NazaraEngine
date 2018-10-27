// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Utility module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Utility/SkeletalMesh.hpp>
#include <Nazara/Utility/Config.hpp>
#include <Nazara/Utility/Debug.hpp>

namespace Nz
{
	SkeletalMesh::SkeletalMesh(VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer) :
	m_aabb(Nz::Boxf::Zero()),
	m_indexBuffer(indexBuffer),
	m_vertexBuffer(vertexBuffer)
	{
		NazaraAssert(m_vertexBuffer, "Invalid vertex buffer");
	}

	SkeletalMesh::~SkeletalMesh()
	{
		OnSkeletalMeshRelease(this);
	}

	const Boxf& SkeletalMesh::GetAABB() const
	{
		return m_aabb;
	}

	AnimationType SkeletalMesh::GetAnimationType() const
	{
		return AnimationType_Skeletal;
	}

	const IndexBufferConstRef& SkeletalMesh::GetIndexBuffer() const
	{
		return m_indexBuffer;
	}

	const VertexBufferRef& SkeletalMesh::GetVertexBuffer()
	{
		return m_vertexBuffer;
	}

	const VertexBufferConstRef& SkeletalMesh::GetVertexBuffer() const
	{
		return m_vertexBuffer;
	}

	unsigned int SkeletalMesh::GetVertexCount() const
	{
		return m_vertexBuffer->GetVertexCount();
	}

	bool SkeletalMesh::IsAnimated() const
	{
		return true;
	}

	bool SkeletalMesh::IsValid() const
	{
		return m_vertexBuffer != nullptr;
	}

	void SkeletalMesh::SetAABB(const Boxf& aabb)
	{
		m_aabb = aabb;

		OnSubMeshInvalidateAABB(this);
	}

	void SkeletalMesh::SetIndexBuffer(IndexBufferConstRef indexBuffer)
	{
		m_indexBuffer = std::move(indexBuffer);
	}
}
