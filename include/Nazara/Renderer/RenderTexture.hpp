// Copyright (C) 2015 Jérôme Leclercq
// This file is part of the "Nazara Engine - Renderer module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_RENDERTEXTURE_HPP
#define NAZARA_RENDERTEXTURE_HPP

#include <Nazara/Prerequesites.hpp>
#include <Nazara/Core/NonCopyable.hpp>
#include <Nazara/Math/Rect.hpp>
#include <Nazara/Renderer/Config.hpp>
#include <Nazara/Renderer/Enums.hpp>
#include <Nazara/Renderer/RenderTarget.hpp>
#include <Nazara/Utility/PixelFormat.hpp>

///TODO: Faire fonctionner les RenderTexture indépendamment du contexte (un FBO par instance et par contexte l'utilisant)

class NzContext;
class NzRenderBuffer;
class NzTexture;

struct NzRenderTextureImpl;

class NAZARA_RENDERER_API NzRenderTexture : public NzRenderTarget, NzNonCopyable
{
	public:
		inline NzRenderTexture();
		inline ~NzRenderTexture();

		bool AttachBuffer(nzAttachmentPoint attachmentPoint, nzUInt8 index, NzRenderBuffer* buffer);
		bool AttachBuffer(nzAttachmentPoint attachmentPoint, nzUInt8 index, nzPixelFormat format, unsigned int width, unsigned int height);
		bool AttachTexture(nzAttachmentPoint attachmentPoint, nzUInt8 index, NzTexture* texture, unsigned int z = 0);

		bool Create(bool lock = false);
		void Destroy();

		void Detach(nzAttachmentPoint attachmentPoint, nzUInt8 index);

		unsigned int GetHeight() const override;
		NzRenderTargetParameters GetParameters() const;
		NzVector2ui GetSize() const;
		unsigned int GetWidth() const override;

		bool IsComplete() const;
		bool IsRenderable() const;
		inline bool IsValid() const;

		bool Lock() const;

		inline void SetColorTarget(nzUInt8 target) const;
		void SetColorTargets(const nzUInt8* targets, unsigned int targetCount) const;
		void SetColorTargets(const std::initializer_list<nzUInt8>& targets) const;

		void Unlock() const;

		// Fonctions OpenGL
		unsigned int GetOpenGLID() const;
		bool HasContext() const override;

		static inline void Blit(NzRenderTexture* src, NzRenderTexture* dst, nzUInt32 buffers = nzRendererBuffer_Color | nzRendererBuffer_Depth | nzRendererBuffer_Stencil, bool bilinearFilter = false);
		static void Blit(NzRenderTexture* src, NzRectui srcRect, NzRenderTexture* dst, NzRectui dstRect, nzUInt32 buffers = nzRendererBuffer_Color | nzRendererBuffer_Depth | nzRendererBuffer_Stencil, bool bilinearFilter = false);

	protected:
		bool Activate() const override;
		void Desactivate() const override;
		void EnsureTargetUpdated() const override;

	private:
		inline void InvalidateDrawBuffers() const;
		inline void InvalidateSize() const;
		inline void InvalidateTargets() const;
		void OnContextDestroy(const NzContext* context);
		void OnRenderBufferDestroy(const NzRenderBuffer* renderBuffer, unsigned int attachmentIndex);
		void OnTextureDestroy(const NzTexture* texture, unsigned int attachmentIndex);
		void UpdateDrawBuffers() const;
		void UpdateSize() const;
		void UpdateTargets() const;

		NzRenderTextureImpl* m_impl;
		mutable bool m_checked ;
		mutable bool m_drawBuffersUpdated;
		mutable bool m_sizeUpdated;
		mutable bool m_targetsUpdated;
};

#include <Nazara/Renderer/RenderTexture.inl>

#endif // NAZARA_RENDERTEXTURE_HPP
