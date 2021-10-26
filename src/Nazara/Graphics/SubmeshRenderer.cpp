// Copyright (C) 2021 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "Nazara Engine - Graphics module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Graphics/SubmeshRenderer.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/RenderSubmesh.hpp>
#include <Nazara/Renderer/CommandBufferBuilder.hpp>
#include <Nazara/Graphics/Debug.hpp>

namespace Nz
{
	SubmeshRenderer::SubmeshRenderer()
	{
		Graphics* graphics = Graphics::Instance();
		const auto& whiteTexture = graphics->GetDefaultTextures().whiteTexture2d;
		const auto& defaultSampler = graphics->GetSamplerCache().Get({});

		m_renderDataBinding = graphics->GetReferencePipelineLayout()->AllocateShaderBinding(Graphics::DrawDataBindingSet);
		m_renderDataBinding->Update({
			{
				0,
				ShaderBinding::TextureBinding {
					whiteTexture.get(), defaultSampler.get()
				}
			}
		});
	}

	std::unique_ptr<ElementRendererData> SubmeshRenderer::InstanciateData()
	{
		return {};
	}

	void SubmeshRenderer::Render(ElementRendererData& /*rendererData*/, CommandBufferBuilder& commandBuffer, const Pointer<const RenderElement>* elements, std::size_t elementCount)
	{
		const AbstractBuffer* currentIndexBuffer = nullptr;
		const AbstractBuffer* currentVertexBuffer = nullptr;
		const RenderPipeline* currentPipeline = nullptr;
		const ShaderBinding* currentMaterialBinding = nullptr;

		commandBuffer.BindShaderBinding(Graphics::DrawDataBindingSet, *m_renderDataBinding);

		for (std::size_t i = 0; i < elementCount; ++i)
		{
			assert(elements[i]->GetElementType() == UnderlyingCast(BasicRenderElement::Submesh));
			const RenderSubmesh& submesh = static_cast<const RenderSubmesh&>(*elements[i]);

			if (const RenderPipeline* pipeline = submesh.GetRenderPipeline(); currentPipeline != pipeline)
			{
				commandBuffer.BindPipeline(*pipeline);
				currentPipeline = pipeline;
			}

			if (const ShaderBinding* materialBinding = &submesh.GetMaterialBinding(); currentMaterialBinding != materialBinding)
			{
				commandBuffer.BindShaderBinding(Graphics::MaterialBindingSet, *materialBinding);
				currentMaterialBinding = materialBinding;
			}

			if (const AbstractBuffer* indexBuffer = submesh.GetIndexBuffer(); currentIndexBuffer != indexBuffer)
			{
				commandBuffer.BindIndexBuffer(*indexBuffer);
				currentIndexBuffer = indexBuffer;
			}

			if (const AbstractBuffer* vertexBuffer = submesh.GetVertexBuffer(); currentVertexBuffer != vertexBuffer)
			{
				commandBuffer.BindVertexBuffer(0, *vertexBuffer);
				currentVertexBuffer = vertexBuffer;
			}

			commandBuffer.BindShaderBinding(Graphics::WorldBindingSet, submesh.GetInstanceBinding());

			if (currentIndexBuffer)
				commandBuffer.DrawIndexed(submesh.GetIndexCount());
			else
				commandBuffer.Draw(submesh.GetIndexCount());
		}
	}
}
