// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Core module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Core/Initializer.hpp>
#include <Nazara/Core/Debug.hpp>

namespace Nz
{
	namespace Detail
	{
		template<typename...> struct Initializer;

		template<typename T, typename... Rest>
		struct Initializer<T, Rest...>
		{
			static bool Init()
			{
				if (T::Initialize())
				{
					if (Initializer<Rest...>::Init())
						return true;
					else
						T::Uninitialize();
				}

				return false;
			}

			static void Uninit()
			{
				Initializer<Rest...>::Uninit();
				T::Uninitialize();
			}
		};

		template<>
		struct Initializer<>
		{
			static bool Init()
			{
				return true;
			}

			static void Uninit()
			{
			}
		};
	}

	/*!
	* \ingroup core
	* \class Nz::Initializer
	* \brief Core class that represents a module initializer
	*/

	/*!
	* \brief Constructs a Initializer object
	*
	* \param initialize Should initialization take place now
	*/
	template<typename... Args>
	Initializer<Args...>::Initializer(bool initialize) :
	m_initialized(false)
	{
		if (initialize)
			Initialize();
	}

	/*!
	* \brief Constructs a Initializer object by copy
	*
	* \param init Initializer object
	*/
	template<typename... Args>
	Nz::Initializer<Args...>::Initializer(const Initializer& init) :
	m_initialized(init.m_initialized)
	{
		if (m_initialized)
			Initialize();
	}

	/*!
	* \brief Constructs a Initializer object by movement
	*
	* \param init Initializer object
	*/
	template<typename... Args>
	Initializer<Args...>::Initializer(Initializer&& init) :
	m_initialized(init.m_initialized)
	{
		init.m_initialized = false;
	}

	/*!
	* \brief Destructs the object and call Uninitialize
	*
	* \see Uninitialize
	*/
	template<typename... Args>
	Initializer<Args...>::~Initializer()
	{
		Uninitialize();
	}

	/*!
	* \brief Initialize modules explicitly
	* \return True if all modules were successfully initialized
	*
	* \see Uninitialize
	*/
	template<typename... Args>
	bool Initializer<Args...>::Initialize()
	{
		if (!m_initialized)
			m_initialized = Detail::Initializer<Args...>::Init();

		return m_initialized;
	}

	/*!
	* \brief Checks whether the module is initialized
	* \return true if initialized
	*/
	template<typename... Args>
	bool Initializer<Args...>::IsInitialized() const
	{
		return m_initialized;
	}

	/*!
	* \brief Uninitialize the module
	*
	* \see Initialize
	*/
	template<typename... Args>
	void Initializer<Args...>::Uninitialize()
	{
		if (m_initialized)
			Detail::Initializer<Args...>::Uninit();
	}

	/*!
	* \brief Check if modules are initialized
	* \return true if initialized
	*/
	template<typename... Args>
	Initializer<Args...>::operator bool() const
	{
		return IsInitialized();
	}

	/*!
	* \brief Assign a Initializer object by copy
	*
	* \param init Initializer object
	*/
	template<typename ...Args>
	Initializer& Initializer<Args...>::operator=(const Initializer& init)
	{
		m_initialized = init.m_initialized;
		if (m_initialized)
			Initialize();

		return *this;
	}

	/*!
	* \brief Assign a Initializer object by movement
	*
	* \param init Initializer object
	*/
	template<typename ...Args>
	Initializer& Initializer<Args...>::operator=(Initializer&& init)
	{
		m_initialized = init.m_initialized;
		init.m_initialized = false;

		return *this;
	}
}

#include <Nazara/Core/DebugOff.hpp>
