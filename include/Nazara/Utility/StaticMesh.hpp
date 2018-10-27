// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Utility module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_STATICMESH_HPP
#define NAZARA_STATICMESH_HPP

#include <Nazara/Prerequisites.hpp>
#include <Nazara/Core/Signal.hpp>
#include <Nazara/Utility/SubMesh.hpp>

namespace Nz
{
	class StaticMesh;

	using StaticMeshConstRef = std::shared_ptr<const StaticMesh>;
	using StaticMeshRef = std::shared_ptr<StaticMesh>;

	class NAZARA_UTILITY_API StaticMesh final : public SubMesh
	{
		public:
			StaticMesh(VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer);
			~StaticMesh();

			void Center();

			bool GenerateAABB();

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

			template<typename... Args> static StaticMeshRef New(Args&&... args);

			// Signals:
			NazaraSignal(OnStaticMeshRelease, const StaticMesh* /*staticMesh*/);

		private:
			Boxf m_aabb;
			IndexBufferConstRef m_indexBuffer;
			VertexBufferRef m_vertexBuffer;
	};
}

#include <Nazara/Utility/StaticMesh.inl>

#endif // NAZARA_STATICMESH_HPP
