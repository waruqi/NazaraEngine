#include <Nazara/Utility.hpp>
#include <Nazara/Renderer/Renderer.hpp>
#include <Nazara/Renderer/RenderBuffer.hpp>
#include <Nazara/Renderer/RenderWindow.hpp>
#include <Nazara/VulkanRenderer.hpp>
#include <array>
#include <iostream>

VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (pCallbackData->messageIdNumber != 0)
		std::cerr << "#" << pCallbackData->messageIdNumber << " " << pCallbackData->messageIdNumber << ": ";

	std::cerr << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

int main()
{
	Nz::ParameterList params;
	params.SetParameter("VkInstanceInfo_EnabledExtensionCount", 1LL);
	params.SetParameter("VkInstanceInfo_EnabledExtension0", "VK_EXT_debug_report");

	params.SetParameter("VkDeviceInfo_EnabledLayerCount", 1LL);
	params.SetParameter("VkDeviceInfo_EnabledLayer0", "VK_LAYER_LUNARG_standard_validation");
	params.SetParameter("VkInstanceInfo_EnabledLayerCount", 1LL);
	params.SetParameter("VkInstanceInfo_EnabledLayer0", "VK_LAYER_LUNARG_standard_validation");

	Nz::Renderer::SetParameters(params);

	Nz::Initializer<Nz::Renderer> loader;
	if (!loader)
	{
		std::cout << "Failed to initialize Vulkan" << std::endl;;
		return __LINE__;
	}

	Nz::VulkanRenderer* rendererImpl = static_cast<Nz::VulkanRenderer*>(Nz::Renderer::GetRendererImpl());

	Nz::Vk::Instance& instance = Nz::Vulkan::GetInstance();

	VkDebugUtilsMessengerCreateInfoEXT callbackCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT  };
	callbackCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
	callbackCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	callbackCreateInfo.pfnUserCallback = &MyDebugReportCallback;

	/* Register the callback */
	VkDebugUtilsMessengerEXT callback;

	instance.vkCreateDebugUtilsMessengerEXT(instance, &callbackCreateInfo, nullptr, &callback);


	std::vector<VkLayerProperties> layerProperties;
	if (!Nz::Vk::Loader::EnumerateInstanceLayerProperties(&layerProperties))
	{
		NazaraError("Failed to enumerate instance layer properties");
		return __LINE__;
	}

	for (const VkLayerProperties& properties : layerProperties)
	{
		std::cout << properties.layerName << ": \t" << properties.description << std::endl;
	}

	Nz::RenderWindow window;

	Nz::MeshParams meshParams;
	meshParams.matrix = Nz::Matrix4f::Rotate(Nz::EulerAnglesf(0.f, 90.f, 180.f)) * Nz::Matrix4f::Scale(Nz::Vector3f(0.002f));
	meshParams.vertexDeclaration = Nz::VertexDeclaration::Get(Nz::VertexLayout_XYZ_Normal_UV);

	Nz::String windowTitle = "Vulkan Test";
	if (!window.Create(Nz::VideoMode(800, 600, 32), windowTitle))
	{
		std::cout << "Failed to create Window" << std::endl;
		return __LINE__;
	}

	std::shared_ptr<Nz::RenderDevice> device = window.GetRenderDevice();

	auto fragmentShader = device->InstantiateShaderStage(Nz::ShaderStageType::Fragment, Nz::ShaderLanguage::SpirV, "resources/shaders/triangle.frag.spv");
	if (!fragmentShader)
	{
		std::cout << "Failed to instantiate fragment shader" << std::endl;
		return __LINE__;
	}

	auto vertexShader = device->InstantiateShaderStage(Nz::ShaderStageType::Vertex, Nz::ShaderLanguage::SpirV, "resources/shaders/triangle.vert.spv");
	if (!vertexShader)
	{
		std::cout << "Failed to instantiate fragment shader" << std::endl;
		return __LINE__;
	}

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

	std::unique_ptr<Nz::Texture> texture = device->InstantiateTexture(texParams);
	if (!texture->Update(drfreakImage->GetConstPixels()))
	{
		NazaraError("Failed to update texture");
		return __LINE__;
	}

	std::unique_ptr<Nz::TextureSampler> textureSampler = device->InstantiateTextureSampler({});

	struct
	{
		Nz::Matrix4f projectionMatrix;
		Nz::Matrix4f modelMatrix;
		Nz::Matrix4f viewMatrix;
	}
	ubo;

	Nz::Vector2ui windowSize = window.GetSize();
	ubo.projectionMatrix = Nz::Matrix4f::Perspective(70.f, float(windowSize.x) / windowSize.y, 0.1f, 1000.f);
	ubo.viewMatrix = Nz::Matrix4f::Translate(Nz::Vector3f::Backward() * 1);
	ubo.modelMatrix = Nz::Matrix4f::Translate(Nz::Vector3f::Forward() * 2 + Nz::Vector3f::Right());

	Nz::UInt32 uniformSize = sizeof(ubo);

	Nz::RenderPipelineLayoutInfo pipelineLayoutInfo;
	auto& uboBinding = pipelineLayoutInfo.bindings.emplace_back();
	uboBinding.index = 0;
	uboBinding.shaderStageFlags = Nz::ShaderStageType::Vertex;
	uboBinding.type = Nz::ShaderBindingType::UniformBuffer;

	auto& textureBinding = pipelineLayoutInfo.bindings.emplace_back();
	textureBinding.index = 1;
	textureBinding.shaderStageFlags = Nz::ShaderStageType::Fragment;
	textureBinding.type = Nz::ShaderBindingType::Texture;

	std::shared_ptr<Nz::RenderPipelineLayout> renderPipelineLayout = device->InstantiateRenderPipelineLayout(pipelineLayoutInfo);

	Nz::ShaderBindingPtr shaderBinding = renderPipelineLayout->AllocateShaderBinding();

	std::unique_ptr<Nz::AbstractBuffer> uniformBuffer = device->InstantiateBuffer(Nz::BufferType_Uniform);
	if (!uniformBuffer->Initialize(uniformSize, Nz::BufferUsage_DeviceLocal))
	{
		NazaraError("Failed to create uniform buffer");
		return __LINE__;
	}

	shaderBinding->Update({
		{
			0,
			Nz::ShaderBinding::UniformBufferBinding {
				uniformBuffer.get(), 0, uniformSize
			}
		},
		{
			1,
			Nz::ShaderBinding::TextureBinding {
				texture.get(), textureSampler.get()
			}
		}
	});

	Nz::RenderPipelineInfo pipelineInfo;
	pipelineInfo.pipelineLayout = renderPipelineLayout;

	pipelineInfo.depthBuffer = true;
	pipelineInfo.shaderStages.emplace_back(fragmentShader);
	pipelineInfo.shaderStages.emplace_back(vertexShader);

	auto& vertexBuffer = pipelineInfo.vertexBuffers.emplace_back();
	vertexBuffer.binding = 0;
	vertexBuffer.declaration = drfreakVB->GetVertexDeclaration();

	std::unique_ptr<Nz::RenderPipeline> pipeline = device->InstantiateRenderPipeline(pipelineInfo);

	std::array<VkClearValue, 2> clearValues;
	clearValues[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
	clearValues[1].depthStencil = {1.f, 0};

	Nz::VkRenderWindow& vulkanWindow = *static_cast<Nz::VkRenderWindow*>(window.GetImpl());
	Nz::VulkanDevice& vulkanDevice = vulkanWindow.GetDevice();

	std::unique_ptr<Nz::CommandPool> commandPool = vulkanWindow.CreateCommandPool(Nz::QueueType::Graphics);

	Nz::UInt32 imageCount = vulkanWindow.GetFramebufferCount();
	std::vector<std::unique_ptr<Nz::CommandBuffer>> renderCmds(imageCount);

	Nz::RenderBuffer* renderBufferIB = static_cast<Nz::RenderBuffer*>(drfreakIB->GetBuffer()->GetImpl());
	Nz::RenderBuffer* renderBufferVB = static_cast<Nz::RenderBuffer*>(drfreakVB->GetBuffer()->GetImpl());

	if (!renderBufferIB->Synchronize(&vulkanDevice))
	{
		NazaraError("Failed to synchronize render buffer");
		return __LINE__;
	}

	if (!renderBufferVB->Synchronize(&vulkanDevice))
	{
		NazaraError("Failed to synchronize render buffer");
		return __LINE__;
	}

	Nz::AbstractBuffer* indexBufferImpl = renderBufferIB->GetHardwareBuffer(&vulkanDevice);
	Nz::AbstractBuffer* vertexBufferImpl = renderBufferVB->GetHardwareBuffer(&vulkanDevice);

	Nz::VulkanRenderPipeline* vkPipeline = static_cast<Nz::VulkanRenderPipeline*>(pipeline.get());

	for (Nz::UInt32 i = 0; i < imageCount; ++i)
	{
		auto& commandBufferPtr = renderCmds[i];

		VkRect2D renderArea = {
			{                                           // VkOffset2D                     offset
				0,                                          // int32_t                        x
				0                                           // int32_t                        y
			},
			{                                           // VkExtent2D                     extent
				windowSize.x,                                        // int32_t                        width
				windowSize.y,                                        // int32_t                        height
			}
		};

		VkRenderPassBeginInfo render_pass_begin_info = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,     // VkStructureType                sType
			nullptr,                                      // const void                    *pNext
			vulkanWindow.GetRenderPass(),                            // VkRenderPass                   renderPass
			vulkanWindow.GetFrameBuffer(i),                       // VkFramebuffer                  framebuffer
			renderArea,
			2U,                                            // uint32_t                       clearValueCount
			clearValues.data()                                  // const VkClearValue            *pClearValues
		};

		commandBufferPtr = commandPool->BuildCommandBuffer([&](Nz::CommandBufferBuilder& builder)
		{
			Nz::Vk::CommandBuffer& vkCommandBuffer = static_cast<Nz::VulkanCommandBufferBuilder&>(builder).GetCommandBuffer();

			builder.BeginDebugRegion("Main window rendering", Nz::Color::Green);
			{
				vkCommandBuffer.BeginRenderPass(render_pass_begin_info);
				{
					builder.BindIndexBuffer(indexBufferImpl);
					builder.BindVertexBuffer(0, vertexBufferImpl);
					builder.BindShaderBinding(*shaderBinding);

					builder.SetScissor(Nz::Recti{ 0, 0, int(windowSize.x), int(windowSize.y) });
					builder.SetViewport(Nz::Recti{ 0, 0, int(windowSize.x), int(windowSize.y) });

					vkCommandBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->Get(vulkanWindow.GetRenderPass())); //< TODO

					builder.DrawIndexed(drfreakIB->GetIndexCount());
				}
				vkCommandBuffer.EndRenderPass();
			}
			builder.EndDebugRegion();
		});
	}

	Nz::Vector3f viewerPos = Nz::Vector3f::Zero();

	Nz::EulerAnglesf camAngles(0.f, 0.f, 0.f);
	Nz::Quaternionf camQuat(camAngles);

	window.EnableEventPolling(true);

	Nz::Clock updateClock;
	Nz::Clock secondClock;
	unsigned int fps = 0;

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

					// Pour éviter que le curseur ne sorte de l'écran, nous le renvoyons au centre de la fenêtre
					// Cette fonction est codée de sorte à ne pas provoquer d'évènement MouseMoved
					Nz::Mouse::SetPosition(windowSize.x / 2, windowSize.y / 2, window);
					break;
				}
			}
		}

		if (updateClock.GetMilliseconds() > 1000 / 60)
		{
			float cameraSpeed = 2.f * updateClock.GetSeconds();
			updateClock.Restart();

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Up) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Z))
				viewerPos += camQuat * Nz::Vector3f::Forward() * cameraSpeed;

			// Si la flèche du bas ou la touche S est pressée, on recule
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Down) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::S))
				viewerPos += camQuat * Nz::Vector3f::Backward() * cameraSpeed;

			// Etc...
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Left) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Q))
				viewerPos += camQuat * Nz::Vector3f::Left() * cameraSpeed;

			// Etc...
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Right) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::D))
				viewerPos += camQuat * Nz::Vector3f::Right() * cameraSpeed;

			// Majuscule pour monter, notez l'utilisation d'une direction globale (Non-affectée par la rotation)
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::LShift) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::RShift))
				viewerPos += Nz::Vector3f::Up() * cameraSpeed;

			// Contrôle (Gauche ou droite) pour descendre dans l'espace global, etc...
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::LControl) || Nz::Keyboard::IsKeyPressed(Nz::Keyboard::RControl))
				viewerPos += Nz::Vector3f::Down() * cameraSpeed;
		}

		Nz::VulkanRenderImage& renderImage = vulkanWindow.Acquire();

		ubo.viewMatrix = Nz::Matrix4f::ViewMatrix(viewerPos, camAngles);

		auto& allocation = renderImage.GetUploadPool().Allocate(uniformSize);

		std::memcpy(allocation.mappedPtr, &ubo, sizeof(ubo));

		renderImage.Execute([&](Nz::CommandBufferBuilder& builder)
		{
			builder.BeginDebugRegion("UBO Update", Nz::Color::Yellow);
			{
				builder.PreTransferBarrier();
				builder.CopyBuffer(allocation, uniformBuffer.get());
				builder.PostTransferBarrier();
			}
			builder.EndDebugRegion();
		}, Nz::QueueType::Transfer);

		Nz::UInt32 imageIndex = renderImage.GetImageIndex();

		renderImage.SubmitCommandBuffer(renderCmds[imageIndex].get(), Nz::QueueType::Graphics);

		renderImage.Present();

		// On incrémente le compteur de FPS improvisé
		fps++;

		if (secondClock.GetMilliseconds() >= 1000) // Toutes les secondes
		{
			// Et on insère ces données dans le titre de la fenêtre
			window.SetTitle(windowTitle + " - " + Nz::String::Number(fps) + " FPS");

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

	instance.vkDestroyDebugUtilsMessengerEXT(instance, callback, nullptr);

	return EXIT_SUCCESS;
}
