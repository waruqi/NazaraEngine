// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Development Kit"
// For conditions of distribution and use, see copyright notice in Prerequisites.hpp

#include <NazaraSDK/Application.hpp>
#include <Nazara/Core/Log.hpp>
#include <regex>

#ifndef NDK_SERVER
#include <Nazara/Graphics/ForwardRenderTechnique.hpp>
#include <Nazara/Utility/SimpleTextDrawer.hpp>
#include <NazaraSDK/Components/CameraComponent.hpp>
#include <NazaraSDK/Components/GraphicsComponent.hpp>
#include <NazaraSDK/Components/NodeComponent.hpp>
#include <NazaraSDK/Systems/RenderSystem.hpp>
#endif

namespace Ndk
{
	/*!
	* \ingroup NDK
	* \class Ndk::Application
	* \brief NDK class that represents the application, it offers a set of tools to ease the development
	*/

	/*!
	* \brief Constructs an Application object with command-line arguments
	*
	* Pass the argc and argv arguments from the main function.
	*
	* Command-line arguments can be retrieved by application methods
	*
	* This calls Sdk::Initialize()
	*
	* \remark Only one Application instance can exist at a time
	*/
	Application::Application(int argc, char* argv[]) :
	Application()
	{
		std::regex optionRegex(R"(-(\w+))");
		std::regex valueRegex(R"(-(\w+)\s*=\s*(.+))");

		std::smatch results;

		for (int i = 1; i < argc; ++i)
		{
			std::string argument(argv[i]);
			if (std::regex_match(argument, results, valueRegex))
			{
				Nz::String key(results[1].str());
				Nz::String value(results[2].str());

				m_parameters[key.ToLower()] = value;
				NazaraDebug("Registred parameter from command-line: " + key.ToLower() + "=" + value);
			}
			else if (std::regex_match(argument, results, optionRegex))
			{
				Nz::String option(results[1].str());

				m_options.insert(option);
				NazaraDebug("Registred option from command-line: " + option);
			}
			else
				NazaraWarning("Ignored command-line argument #" + Nz::String::Number(i) + " \"" + argument + '"');
		}

		#ifndef NDK_SERVER
		if (HasOption("console"))
			EnableConsole(true);

		if (HasOption("fpscounter"))
			EnableFPSCounter(true);
		#endif
	}

	/*!
	* \brief Runs the application by updating worlds, taking care about windows, ...
	*/
	bool Application::Run()
	{
		#ifndef NDK_SERVER
		bool hasAtLeastOneActiveWindow = false;

		auto it = m_windows.begin();
		while (it != m_windows.end())
		{
			Nz::Window& window = *it->window;

			window.ProcessEvents();

			if (!window.IsOpen(true))
			{
				it = m_windows.erase(it);
				continue;
			}

			hasAtLeastOneActiveWindow = true;

			++it;
		}

		if (m_exitOnClosedWindows && !hasAtLeastOneActiveWindow)
			return false;
		#endif

		if (m_shouldQuit)
			return false;

		m_updateTime = m_updateClock.Restart() / 1'000'000.f;

		for (World& world : m_worlds)
			world.Update(m_updateTime);

		#ifndef NDK_SERVER
		for (WindowInfo& info : m_windows)
		{
			if (!info.overlayWorld)
				continue;

			if (info.fpsCounter)
			{
				FPSCounterOverlay& fpsCounter = *info.fpsCounter;

				fpsCounter.frameCount++;

				fpsCounter.elapsedTime += m_updateTime;
				if (fpsCounter.elapsedTime >= 1.f)
				{
					fpsCounter.sprite->Update(Nz::SimpleTextDrawer::Draw("FPS: " + Nz::String::Number(fpsCounter.frameCount), 36));
					fpsCounter.frameCount = 0;
					fpsCounter.elapsedTime = 0.f;
				}
			}

			info.overlayWorld->Update(m_updateTime);
		}
		#endif

		return true;
	}

	#ifndef NDK_SERVER
	void Application::SetupConsole(WindowInfo& info)
	{
		std::unique_ptr<ConsoleOverlay> overlay = std::make_unique<ConsoleOverlay>();

		Nz::Vector2ui windowDimensions;
		if (info.window->IsValid())
			windowDimensions = info.window->GetSize();
		else
			windowDimensions.MakeZero();

		overlay->console = info.canvas->Add<Console>();

		Console& consoleRef = *overlay->console;
		consoleRef.Resize({float(windowDimensions.x), windowDimensions.y / 4.f});
		consoleRef.Show(false);

		// Redirect logs toward the console
		overlay->logSlot.Connect(Nz::Log::OnLogWrite, [&consoleRef] (const Nz::String& str)
		{
			consoleRef.AddLine(str);
		});

		// Setup a few event callback to handle the console
		Nz::EventHandler& eventHandler = info.window->GetEventHandler();

		overlay->keyPressedSlot.Connect(eventHandler.OnKeyPressed, [&consoleRef] (const Nz::EventHandler*, const Nz::WindowEvent::KeyEvent& event)
		{
			if (event.virtualKey == Nz::Keyboard::VKey::F9)
			{
				// Toggle console visibility and focus
				if (consoleRef.IsVisible())
				{
					consoleRef.ClearFocus();
					consoleRef.Show(false);
				}
				else
				{
					consoleRef.Show(true);
					consoleRef.SetFocus();
				}
			}
		});

		overlay->resizedSlot.Connect(info.renderTarget->OnRenderTargetSizeChange, [&consoleRef] (const Nz::RenderTarget* renderTarget)
		{
			Nz::Vector2ui size = renderTarget->GetSize();
			consoleRef.Resize({float(size.x), size.y / 4.f});
		});

		info.console = std::move(overlay);
	}

	void Application::SetupFPSCounter(WindowInfo& info)
	{
		std::unique_ptr<FPSCounterOverlay> fpsCounter = std::make_unique<FPSCounterOverlay>();
		fpsCounter->sprite = Nz::TextSprite::New();

		fpsCounter->entity = info.overlayWorld->CreateEntity();
		fpsCounter->entity->AddComponent<NodeComponent>();
		fpsCounter->entity->AddComponent<GraphicsComponent>().Attach(fpsCounter->sprite);

		info.fpsCounter = std::move(fpsCounter);
	}

	void Application::SetupOverlay(WindowInfo& info)
	{
		info.overlayWorld = std::make_unique<World>(false); //< No default system

		if (info.window->IsValid())
			info.canvas = std::make_unique<Canvas>(info.overlayWorld->CreateHandle(), info.window->GetEventHandler(), info.window->GetCursorController().CreateHandle());

		RenderSystem& renderSystem = info.overlayWorld->AddSystem<RenderSystem>();
		renderSystem.ChangeRenderTechnique<Nz::ForwardRenderTechnique>();
		renderSystem.SetDefaultBackground(nullptr);
		renderSystem.SetGlobalUp(Nz::Vector3f::Down());

		EntityHandle viewer = info.overlayWorld->CreateEntity();
		CameraComponent& camComponent = viewer->AddComponent<CameraComponent>();
		viewer->AddComponent<NodeComponent>();

		camComponent.SetProjectionType(Nz::ProjectionType_Orthogonal);
		camComponent.SetTarget(info.renderTarget);
	}
	#endif

	Application* Application::s_application = nullptr;
}