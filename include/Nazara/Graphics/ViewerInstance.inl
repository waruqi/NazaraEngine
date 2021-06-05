// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Graphics module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Graphics/ViewerInstance.hpp>
#include <Nazara/Core/Error.hpp>
#include <Nazara/Graphics/Debug.hpp>

namespace Nz
{
	inline void ViewerInstance::UpdateProjectionMatrix(const Matrix4f& projectionMatrix)
	{
		m_projectionMatrix = projectionMatrix;
		if (!m_projectionMatrix.GetInverse(&m_invProjectionMatrix))
			NazaraError("failed to inverse projection matrix");
	}

	inline void ViewerInstance::UpdateProjectionMatrix(const Matrix4f& projectionMatrix, const Matrix4f& invProjectionMatrix)
	{
		m_projectionMatrix = projectionMatrix;
		m_invProjectionMatrix = invProjectionMatrix;
	}

	inline void ViewerInstance::UpdateProjViewMatrices(const Matrix4f& projectionMatrix, const Matrix4f& viewMatrix)
	{
		m_projectionMatrix = projectionMatrix;
		if (!m_projectionMatrix.GetInverse(&m_invProjectionMatrix))
			NazaraError("failed to inverse projection matrix");

		m_viewMatrix = viewMatrix;
		if (!m_viewMatrix.GetInverseAffine(&m_invViewMatrix))
			NazaraError("failed to inverse view matrix");
	}

	inline void ViewerInstance::UpdateProjViewMatrices(const Matrix4f& projectionMatrix, const Matrix4f& invProjectionMatrix, const Matrix4f& viewMatrix, const Matrix4f& invViewMatrix)
	{
		m_projectionMatrix = projectionMatrix;
		m_viewMatrix = viewMatrix;
		m_invProjectionMatrix = invProjectionMatrix;
		m_invViewMatrix = invViewMatrix;

		m_viewProjMatrix = m_viewMatrix * m_projectionMatrix;
		m_invViewProjMatrix = m_invProjectionMatrix * m_invViewMatrix;
	}

	inline void ViewerInstance::UpdateProjViewMatrices(const Matrix4f& projectionMatrix, const Matrix4f& invProjectionMatrix, const Matrix4f& viewMatrix, const Matrix4f& invViewMatrix, const Matrix4f& viewProjMatrix, const Matrix4f& invViewProjMatrix)
	{
		m_projectionMatrix = projectionMatrix;
		m_viewMatrix = viewMatrix;
		m_invProjectionMatrix = invProjectionMatrix;
		m_invViewMatrix = invViewMatrix;

		m_viewProjMatrix = viewProjMatrix;
		m_invViewProjMatrix = invViewProjMatrix;
	}

	inline void ViewerInstance::UpdateTargetSize(const Vector2f& targetSize)
	{
		m_targetSize = targetSize;
	}

	inline void ViewerInstance::UpdateViewMatrix(const Matrix4f& viewMatrix)
	{
		m_viewMatrix = viewMatrix;
		if (!m_viewMatrix.GetInverseAffine(&m_invViewMatrix))
			NazaraError("failed to inverse view matrix");

		m_viewProjMatrix = m_viewMatrix * m_projectionMatrix;
		m_invViewProjMatrix = m_invProjectionMatrix * m_invViewMatrix;
	}

	inline void ViewerInstance::UpdateViewMatrix(const Matrix4f& viewMatrix, const Matrix4f& invViewMatrix)
	{
		m_viewMatrix = viewMatrix;
		m_invViewMatrix = invViewMatrix;

		m_viewProjMatrix = m_viewMatrix * m_projectionMatrix;
		m_invViewProjMatrix = m_invProjectionMatrix * m_invViewMatrix;
	}
}

#include <Nazara/Graphics/DebugOff.hpp>
