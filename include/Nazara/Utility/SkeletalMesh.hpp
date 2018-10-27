// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Utility module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_SKELETALMESH_HPP
#define NAZARA_SKELETALMESH_HPP

#include <Nazara/Prerequisites.hpp>
#include <Nazara/Core/Signal.hpp>
#include <Nazara/Utility/IndexBuffer.hpp>
#include <Nazara/Utility/SubMesh.hpp>
#include <Nazara/Utility/VertexBuffer.hpp>

namespace Nz
{
	class SkeletalMesh;

	using SkeletalMeshConstRef = std::shared_ptr<const SkeletalMesh>;
	using SkeletalMeshRef = std::shared_ptr<SkeletalMesh>;

	class NAZARA_UTILITY_API SkeletalMesh final : public SubMesh
	{
		public:
			SkeletalMesh(VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer);
			~SkeletalMesh();

			const Boxf& GetAABB() const override;
			AnimationType GetAnimationType() const final override;
			const IndexBufferConstRef& GetIndexBuffer() const override;
			const VertexBufferRef& GetVertexBuffer();
			const VertexBufferConstRef& GetVertexBuffer() const;
			unsigned int GetVertexCount() const override;

			bool IsAnimated() const final override;
			bool IsValid() const;

			void SetAABB(const Boxf& aabb);
			void SetIndexBuffer(IndexBufferConstRef indexBuffer);

			template<typename... Args> static SkeletalMeshRef New(Args&&... args);

			// Signals:
			NazaraSignal(OnSkeletalMeshRelease, const SkeletalMesh* /*skeletalMesh*/);

		private:
			Boxf m_aabb;
			IndexBufferConstRef m_indexBuffer;
			VertexBufferRef m_vertexBuffer;
	};
}

#include <Nazara/Utility/SkeletalMesh.inl>

#endif // NAZARA_SKELETALMESH_HPP
