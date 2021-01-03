#include <Nazara/Core.hpp>
#include <Nazara/Platform.hpp>
#include <Nazara/Graphics.hpp>
#include <Nazara/Renderer.hpp>
#include <Nazara/Shader.hpp>
#include <Nazara/Shader/SpirvConstantCache.hpp>
#include <Nazara/Shader/SpirvPrinter.hpp>
#include <Nazara/Utility.hpp>
#include <array>
#include <iostream>

int main()
{
	Nz::Renderer::Config rendererConfig;
	std::cout << "Run using Vulkan? (y/n)" << std::endl;
	if (std::getchar() == 'y')
		rendererConfig.preferredAPI = Nz::RenderAPI::Vulkan;
	else
		rendererConfig.preferredAPI = Nz::RenderAPI::OpenGL;

	Nz::Modules<Nz::Graphics> nazara(rendererConfig);

	Nz::RenderWindow window;

	Nz::MeshParams meshParams;
	meshParams.matrix = Nz::Matrix4f::Rotate(Nz::EulerAnglesf(0.f, 90.f, 180.f)) * Nz::Matrix4f::Scale(Nz::Vector3f(0.002f));
	meshParams.vertexDeclaration = Nz::VertexDeclaration::Get(Nz::VertexLayout_XYZ_Normal_UV);

	std::string windowTitle = "Graphics Test";
	if (!window.Create(Nz::VideoMode(800, 600, 32), windowTitle))
	{
		std::cout << "Failed to create Window" << std::endl;
		return __LINE__;
	}

	std::shared_ptr<Nz::RenderDevice> device = window.GetRenderDevice();

	Nz::MeshRef drfreak = Nz::Mesh::LoadFromFile("resources/Spaceship/spaceship.obj", meshParams);
	if (!drfreak)
	{
		NazaraError("Failed to load model");
		return __LINE__;
	}

	Nz::StaticMesh* drfreakMesh = static_cast<Nz::StaticMesh*>(drfreak->GetSubMesh(0));

	const Nz::VertexBuffer* drfreakVB = drfreakMesh->GetVertexBuffer();
	const Nz::IndexBuffer* drfreakIB = drfreakMesh->GetIndexBuffer();

	// Index buffer
	std::cout << "Index count: " << drfreakIB->GetIndexCount() << std::endl;

	// Vertex buffer
	std::cout << "Vertex count: " << drfreakVB->GetVertexCount() << std::endl;

	// Texture
	Nz::ImageRef drfreakImage = Nz::Image::LoadFromFile("resources/Spaceship/Texture/diffuse.png");
	if (!drfreakImage || !drfreakImage->Convert(Nz::PixelFormat_RGBA8))
	{
		NazaraError("Failed to load image");
		return __LINE__;
	}

	Nz::TextureInfo texParams;
	texParams.pixelFormat = drfreakImage->GetFormat();
	texParams.type = drfreakImage->GetType();
	texParams.width = drfreakImage->GetWidth();
	texParams.height = drfreakImage->GetHeight();
	texParams.depth = drfreakImage->GetDepth();

	std::shared_ptr<Nz::Texture> texture = device->InstantiateTexture(texParams);
	if (!texture->Update(drfreakImage->GetConstPixels()))
	{
		NazaraError("Failed to update texture");
		return __LINE__;
	}

	// Texture (alpha-map)
	Nz::ImageRef alphaImage = Nz::Image::LoadFromFile("alphatile.png");
	if (!alphaImage || !alphaImage->Convert(Nz::PixelFormat_RGBA8))
	{
		NazaraError("Failed to load image");
		return __LINE__;
	}

	Nz::TextureInfo alphaTexParams;
	alphaTexParams.pixelFormat = alphaImage->GetFormat();
	alphaTexParams.type = alphaImage->GetType();
	alphaTexParams.width = alphaImage->GetWidth();
	alphaTexParams.height = alphaImage->GetHeight();
	alphaTexParams.depth = alphaImage->GetDepth();

	std::shared_ptr<Nz::Texture> alphaTexture = device->InstantiateTexture(alphaTexParams);
	if (!alphaTexture->Update(alphaImage->GetConstPixels()))
	{
		NazaraError("Failed to update texture");
		return __LINE__;
	}



	std::shared_ptr<Nz::TextureSampler> textureSampler = device->InstantiateTextureSampler({});

	Nz::Material material(Nz::BasicMaterial::GetSettings());
	material.EnableDepthBuffer(true);

	Nz::BasicMaterial basicMat(material);
	basicMat.EnableAlphaTest(true);
	basicMat.SetAlphaMap(alphaTexture);
	basicMat.SetAlphaSampler(textureSampler);
	basicMat.SetDiffuseMap(alphaTexture);
	basicMat.SetDiffuseSampler(textureSampler);

	Nz::PredefinedInstanceData instanceUboOffsets = Nz::PredefinedInstanceData::GetOffsets();
	Nz::PredefinedViewerData viewerUboOffsets = Nz::PredefinedViewerData::GetOffsets();
	const Nz::BasicMaterial::UniformOffsets& materialSettingOffsets = Nz::BasicMaterial::GetOffsets();

	std::vector<std::uint8_t> instanceDataBuffer(instanceUboOffsets.totalSize);
	std::vector<std::uint8_t> viewerDataBuffer(viewerUboOffsets.totalSize);
	std::vector<std::uint8_t> materialSettings(materialSettingOffsets.totalSize);

	Nz::Vector2ui windowSize = window.GetSize();

	Nz::AccessByOffset<Nz::Matrix4f&>(instanceDataBuffer.data(), instanceUboOffsets.worldMatrixOffset) = Nz::Matrix4f::Translate(Nz::Vector3f::Forward() * 2 + Nz::Vector3f::Right());
	Nz::AccessByOffset<Nz::Matrix4f&>(viewerDataBuffer.data(), viewerUboOffsets.viewMatrixOffset) = Nz::Matrix4f::Translate(Nz::Vector3f::Backward() * 1);
	Nz::AccessByOffset<Nz::Matrix4f&>(viewerDataBuffer.data(), viewerUboOffsets.projMatrixOffset) = Nz::Matrix4f::Perspective(70.f, float(windowSize.x) / windowSize.y, 0.1f, 1000.f);

	Nz::AccessByOffset<float&>(materialSettings.data(), materialSettingOffsets.alphaThreshold) = 0.5f;
	Nz::AccessByOffset<Nz::Vector4f&>(materialSettings.data(), materialSettingOffsets.diffuseColor) = Nz::Vector4f(1.f, 1.f, 1.f, 1.f);

	std::shared_ptr<Nz::RenderPipelineLayout> renderPipelineLayout = material.GetSettings()->GetRenderPipelineLayout();

	std::shared_ptr<Nz::AbstractBuffer> instanceDataUbo = device->InstantiateBuffer(Nz::BufferType_Uniform);
	if (!instanceDataUbo->Initialize(instanceDataBuffer.size(), Nz::BufferUsage_DeviceLocal | Nz::BufferUsage_Dynamic))
	{
		NazaraError("Failed to create instance UBO");
		return __LINE__;
	}

	instanceDataUbo->Fill(instanceDataBuffer.data(), 0, instanceDataBuffer.size());

	std::shared_ptr<Nz::AbstractBuffer> viewerDataUbo = device->InstantiateBuffer(Nz::BufferType_Uniform);
	if (!viewerDataUbo->Initialize(viewerDataBuffer.size(), Nz::BufferUsage_DeviceLocal | Nz::BufferUsage_Dynamic))
	{
		NazaraError("Failed to create viewer UBO");
		return __LINE__;
	}

	std::shared_ptr<Nz::AbstractBuffer> matSettingUBO = device->InstantiateBuffer(Nz::BufferType_Uniform);
	if (!matSettingUBO->Initialize(materialSettings.size(), Nz::BufferUsage_DeviceLocal | Nz::BufferUsage_Dynamic))
	{
		NazaraError("Failed to create mat setting UBO");
		return __LINE__;
	}

	matSettingUBO->Fill(materialSettings.data(), 0, materialSettings.size());

	Nz::ShaderBindingPtr shaderBinding = renderPipelineLayout->AllocateShaderBinding();
	shaderBinding->Update({
		{
			material.GetSettings()->GetPredefinedBindingIndex(Nz::PredefinedShaderBinding::UboViewerData),
			Nz::ShaderBinding::UniformBufferBinding {
				viewerDataUbo.get(), 0, viewerDataBuffer.size()
			}
		},
		{
			material.GetSettings()->GetPredefinedBindingIndex(Nz::PredefinedShaderBinding::UboInstanceData),
			Nz::ShaderBinding::UniformBufferBinding {
				instanceDataUbo.get(), 0, instanceDataBuffer.size()
			}
		},
		{
			3,
			Nz::ShaderBinding::UniformBufferBinding {
				matSettingUBO.get(), 0, materialSettings.size()
			}
		},
		{
			0,
			Nz::ShaderBinding::TextureBinding {
				alphaTexture.get(), textureSampler.get()
			}
		},
		{
			1,
			Nz::ShaderBinding::TextureBinding {
				texture.get(), textureSampler.get()
			}
		}
	});

	std::vector<Nz::RenderPipelineInfo::VertexBufferData> vertexBuffers = {
		{
			0,
			drfreakVB->GetVertexDeclaration()
		}
	};

	std::shared_ptr<Nz::RenderPipeline> pipeline = material.GetPipeline()->GetRenderPipeline(vertexBuffers);

	Nz::RenderDevice* renderDevice = window.GetRenderDevice().get();

	Nz::RenderWindowImpl* windowImpl = window.GetImpl();
	std::shared_ptr<Nz::CommandPool> commandPool = windowImpl->CreateCommandPool(Nz::QueueType::Graphics);

	Nz::RenderBuffer* renderBufferIB = static_cast<Nz::RenderBuffer*>(drfreakIB->GetBuffer()->GetImpl());
	Nz::RenderBuffer* renderBufferVB = static_cast<Nz::RenderBuffer*>(drfreakVB->GetBuffer()->GetImpl());

	if (!renderBufferIB->Synchronize(renderDevice))
	{
		NazaraError("Failed to synchronize render buffer");
		return __LINE__;
	}

	if (!renderBufferVB->Synchronize(renderDevice))
	{
		NazaraError("Failed to synchronize render buffer");
		return __LINE__;
	}

	Nz::AbstractBuffer* indexBufferImpl = renderBufferIB->GetHardwareBuffer(renderDevice);
	Nz::AbstractBuffer* vertexBufferImpl = renderBufferVB->GetHardwareBuffer(renderDevice);

	Nz::CommandBufferPtr drawCommandBuffer;
	auto RebuildCommandBuffer = [&]
	{
		Nz::Vector2ui windowSize = window.GetSize();

		drawCommandBuffer = commandPool->BuildCommandBuffer([&](Nz::CommandBufferBuilder& builder)
		{
			Nz::Recti renderRect(0, 0, window.GetSize().x, window.GetSize().y);

			Nz::CommandBufferBuilder::ClearValues clearValues[2];
			clearValues[0].color = Nz::Color::Black;
			clearValues[1].depth = 1.f;
			clearValues[1].stencil = 0;

			builder.BeginDebugRegion("Main window rendering", Nz::Color::Green);
			{
				builder.BeginRenderPass(windowImpl->GetFramebuffer(), windowImpl->GetRenderPass(), renderRect, { clearValues[0], clearValues[1] });
				{
					builder.BindIndexBuffer(indexBufferImpl);
					builder.BindPipeline(*pipeline);
					builder.BindVertexBuffer(0, vertexBufferImpl);
					builder.BindShaderBinding(*shaderBinding);

					builder.SetScissor(Nz::Recti{ 0, 0, int(windowSize.x), int(windowSize.y) });
					builder.SetViewport(Nz::Recti{ 0, 0, int(windowSize.x), int(windowSize.y) });

					builder.DrawIndexed(drfreakIB->GetIndexCount());
				}
				builder.EndRenderPass();
			}
			builder.EndDebugRegion();
		});
	};
	RebuildCommandBuffer();


	Nz::Vector3f viewerPos = Nz::Vector3f::Zero();

	Nz::EulerAnglesf camAngles(0.f, 0.f, 0.f);
	Nz::Quaternionf camQuat(camAngles);

	window.EnableEventPolling(true);

	Nz::Clock updateClock;
	Nz::Clock secondClock;
	unsigned int fps = 0;
	bool viewerUboUpdate = true;

	Nz::Mouse::SetRelativeMouseMode(true);

	while (window.IsOpen())
	{
		Nz::WindowEvent event;
		while (window.PollEvent(&event))
		{
			switch (event.type)
			{
				case Nz::WindowEventType_Quit:
					window.Close();
					break;

				case Nz::WindowEventType_MouseMoved: // La souris a bougé
				{
					// Gestion de la caméra free-fly (Rotation)
					float sensitivity = 0.3f; // Sensibilité de la souris

					// On modifie l'angle de la caméra grâce au déplacement relatif sur X de la souris
					camAngles.yaw = Nz::NormalizeAngle(camAngles.yaw - event.mouseMove.deltaX*sensitivity);

					// Idem, mais pour éviter les problèmes de calcul de la matrice de vue, on restreint les angles
					camAngles.pitch = Nz::Clamp(camAngles.pitch + event.mouseMove.deltaY*sensitivity, -89.f, 89.f);

					camQuat = camAngles;
					
					viewerUboUpdate = true;
					break;
				}

				case Nz::WindowEventType_Resized:
				{
					Nz::Vector2ui windowSize = window.GetSize();
					Nz::AccessByOffset<Nz::Matrix4f&>(viewerDataBuffer.data(), viewerUboOffsets.projMatrixOffset) = Nz::Matrix4f::Perspective(70.f, float(windowSize.x) / windowSize.y, 0.1f, 1000.f);
					viewerUboUpdate = true;
					break;
				}

				default:
					break;
			}
		}

		if (updateClock.GetMilliseconds() > 1000 / 60)
		{
			float cameraSpeed = 2.f * updateClock.GetSeconds();
			updateClock.Restart();

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Up) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Z))
				viewerPos += camQuat * Nz::Vector3f::Forward() * cameraSpeed;

			// Si la flèche du bas ou la touche S est pressée, on recule
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Down) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::S))
				viewerPos += camQuat * Nz::Vector3f::Backward() * cameraSpeed;

			// Etc...
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Left) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Q))
				viewerPos += camQuat * Nz::Vector3f::Left() * cameraSpeed;

			// Etc...
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Right) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::D))
				viewerPos += camQuat * Nz::Vector3f::Right() * cameraSpeed;

			// Majuscule pour monter, notez l'utilisation d'une direction globale (Non-affectée par la rotation)
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::RShift))
				viewerPos += Nz::Vector3f::Up() * cameraSpeed;

			// Contrôle (Gauche ou droite) pour descendre dans l'espace global, etc...
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LControl) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::RControl))
				viewerPos += Nz::Vector3f::Down() * cameraSpeed;

			viewerUboUpdate = true;
		}

		Nz::RenderFrame frame = windowImpl->Acquire();
		if (!frame)
			continue;

		if (frame.IsFramebufferInvalidated())
			RebuildCommandBuffer();

		Nz::AccessByOffset<Nz::Matrix4f&>(viewerDataBuffer.data(), viewerUboOffsets.viewMatrixOffset) = Nz::Matrix4f::ViewMatrix(viewerPos, camAngles);

		if (viewerUboUpdate)
		{
			auto& allocation = frame.GetUploadPool().Allocate(viewerDataBuffer.size());

			std::memcpy(allocation.mappedPtr, viewerDataBuffer.data(), viewerDataBuffer.size());

			frame.Execute([&](Nz::CommandBufferBuilder& builder)
			{
				builder.BeginDebugRegion("UBO Update", Nz::Color::Yellow);
				{
					builder.PreTransferBarrier();
					builder.CopyBuffer(allocation, viewerDataUbo.get());
					builder.PostTransferBarrier();
				}
				builder.EndDebugRegion();
			}, Nz::QueueType::Transfer);

			viewerUboUpdate = false;
		}

		frame.SubmitCommandBuffer(drawCommandBuffer.get(), Nz::QueueType::Graphics);

		frame.Present();

		window.Display();

		// On incrémente le compteur de FPS improvisé
		fps++;

		if (secondClock.GetMilliseconds() >= 1000) // Toutes les secondes
		{
			// Et on insère ces données dans le titre de la fenêtre
			window.SetTitle(windowTitle + " - " + Nz::NumberToString(fps) + " FPS");

			/*
			Note: En C++11 il est possible d'insérer de l'Unicode de façon standard, quel que soit l'encodage du fichier,
			via quelque chose de similaire à u8"Cha\u00CEne de caract\u00E8res".
			Cependant, si le code source est encodé en UTF-8 (Comme c'est le cas dans ce fichier),
			cela fonctionnera aussi comme ceci : "Chaîne de caractères".
			*/

			// Et on réinitialise le compteur de FPS
			fps = 0;

			// Et on relance l'horloge pour refaire ça dans une seconde
			secondClock.Restart();
		}
	}

	return EXIT_SUCCESS;
}