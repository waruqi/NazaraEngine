// Copyright (C) 2015 Jérôme Leclercq
// This file is part of the "Nazara Engine - Core module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Core/AbstractHash.hpp>
#include <Nazara/Core/Error.hpp>
#include <cstring>
#include <Nazara/Core/Debug.hpp>

namespace Nz
{
	inline String::String() :
	m_sharedString(GetEmptyString())
	{
	}

	inline String::String(char character)
	{
		if (character != '\0')
		{
			m_isSmallString = true;
			m_smallString.buffer[0] = character;
			m_smallString.buffer[1] = '\0';
			m_smallString.size = 1;
		}
		else
		{
			m_isSmallString = false;
			m_sharedString = GetEmptyString();
		}
	}

	inline String::String(std::size_t rep, char character)
	{
		if (rep > 0)
		{
			char* buffer;
			if (rep > s_maxSSSize)
			{
				m_isSmallString = false;
				m_sharedString = std::make_shared<SharedString>(rep);
				buffer = m_sharedString->string.get();
			}
			else
			{
				m_isSmallString = true;
				m_smallString.buffer[rep] = '\0';
				m_smallString.size = static_cast<char>(rep);
				buffer = m_smallString.buffer;
			}

			if (character != '\0')
				std::memset(buffer, character, rep);
		}
		else
			m_sharedString = GetEmptyString();
	}

	inline String::String(std::size_t rep, const char* string) :
	String(rep, string, (string) ? std::strlen(string) : 0)
	{
	}

	inline String::String(std::size_t rep, const char* string, std::size_t length)
	{
		std::size_t totalSize = rep*length;

		if (totalSize > 0)
		{
			char* buffer;
			if (totalSize > s_maxSSSize)
			{
				m_isSmallString = false;
				m_sharedString = std::make_shared<SharedString>(totalSize);
				buffer = m_sharedString->string.get();
			}
			else
			{
				m_isSmallString = true;
				m_smallString.buffer[totalSize] = '\0';
				m_smallString.size = static_cast<char>(totalSize);
				buffer = m_smallString.buffer;
			}

			for (std::size_t i = 0; i < rep; ++i)
				std::memcpy(&buffer[i*length], string, length);
		}
		else
			m_sharedString = GetEmptyString();
	}

	inline String::String(std::size_t rep, const String& string) :
	String(rep, string.GetConstBuffer(), string.GetSize())
	{
	}

	inline String::String(const char* string) :
	String(string, (string) ? std::strlen(string) : 0)
	{
	}

	inline String::String(const char* string, std::size_t length)
	{
		if (length > 0)
		{
			char* buffer;
			if (length > s_maxSSSize)
			{
				m_isSmallString = false;
				m_sharedString = std::make_shared<SharedString>(length);
				buffer = m_sharedString->string.get();
			}
			else
			{
				m_isSmallString = true;
				m_smallString.buffer[length] = '\0';
				m_smallString.size = static_cast<char>(length);
				buffer = m_smallString.buffer;
			}

			m_sharedString = std::make_shared<SharedString>(length);
			std::memcpy(buffer, string, length);
		}
		else
			m_sharedString = GetEmptyString();
	}

	inline String::String(const std::string& string) :
	String(string.c_str(), string.size())
	{
	}

	inline String::String(std::shared_ptr<SharedString>&& sharedString) :
	m_sharedString(std::move(sharedString)),
	m_isSmallString(false)
	{
	}

	inline String::String(const String& string) :
	m_isSmallString(string.m_isSmallString)
	{
		if (m_isSmallString)
			m_smallString = string.m_smallString;
		else
			m_sharedString = string.m_sharedString;
	}

	inline String::String(String&& string) noexcept :
	m_isSmallString(string.m_isSmallString)
	{
		if (m_isSmallString)
			m_smallString = string.m_smallString;
		else
			m_sharedString = std::move(string.m_sharedString);
	}

	inline String::~String()
	{
		if (!m_isSmallString)
			m_sharedString.~shared_ptr();
	}

	inline String& String::Append(char character)
	{
		return Insert(GetSize(), character);
	}

	inline String& String::Append(const char* string)
	{
		return Insert(GetSize(), string);
	}

	inline String& String::Append(const char* string, std::size_t length)
	{
		return Insert(GetSize(), string, length);
	}

	inline String& String::Append(const String& string)
	{
		return Insert(GetSize(), string);
	}

	inline bool String::Contains(char character, std::intmax_t start, UInt32 flags) const
	{
		return Find(character, start, flags) != npos;
	}

	inline bool String::Contains(const char* string, std::intmax_t start, UInt32 flags) const
	{
		return Find(string, start, flags) != npos;
	}

	inline bool String::Contains(const String& string, std::intmax_t start, UInt32 flags) const
	{
		return Find(string, start, flags) != npos;
	}

	inline unsigned int String::Count(const String& string, std::intmax_t start, UInt32 flags) const
	{
		return Count(string.GetConstBuffer(), start, flags);
	}

	inline unsigned int String::CountAny(const String& string, std::intmax_t start, UInt32 flags) const
	{
		return CountAny(string.GetConstBuffer(), start, flags);
	}

	inline bool String::EndsWith(const char* string, UInt32 flags) const
	{
		return EndsWith(string, std::strlen(string), flags);
	}

	inline bool String::EndsWith(const String& string, UInt32 flags) const
	{
		return EndsWith(string.GetConstBuffer(), string.m_sharedString->size, flags);
	}

	inline std::size_t String::Find(const String& string, std::intmax_t start, UInt32 flags) const
	{
		return Find(string.GetConstBuffer(), start, flags);
	}

	inline std::size_t String::FindAny(const String& string, std::intmax_t start, UInt32 flags) const
	{
		return FindAny(string.GetConstBuffer(), start, flags);
	}

	inline std::size_t String::FindLastAny(const String& string, std::intmax_t start, UInt32 flags) const
	{
		return FindLastAny(string.GetConstBuffer(), start, flags);
	}

	inline std::size_t String::GetAbsolutePos(std::intmax_t pos) const
	{
		if (pos < 0)
			return std::max<std::size_t>(GetSize() - static_cast<std::size_t>(-pos), 0);
		else
			return static_cast<std::size_t>(pos);

	}

	inline char* String::GetBuffer()
	{
		if (m_isSmallString)
			return m_smallString.buffer;
		else
		{
			EnsureOwnership();

			return m_sharedString->string.get();
		}
	}

	inline std::size_t String::GetCapacity() const
	{
		return m_isSmallString ? s_maxSSSize : m_sharedString->capacity;
	}

	inline const char* String::GetConstBuffer() const
	{
		return m_isSmallString ? m_smallString.buffer : m_sharedString->string.get();
	}

	inline std::size_t String::GetSize() const
	{
		return m_isSmallString ? m_smallString.size : m_sharedString->size;
	}

	inline std::string String::GetUtf8String() const
	{
		return std::string(GetConstBuffer(), GetSize());
	}

	inline String& String::Insert(std::intmax_t pos, const String& string)
	{
		return Insert(pos, string.GetConstBuffer(), string.m_sharedString->size);
	}

	inline bool String::IsEmpty() const
	{
		return (m_isSmallString) ? m_smallString.size == 0 : m_sharedString->size == 0;
	}

	inline bool String::IsNull() const
	{
		return (!m_isSmallString) ? m_sharedString.get() == GetEmptyString().get() : false;
	}

	inline bool operator==(const String& first, const String& second)
	{
		if (first.GetSize() != second.GetSize())
			return false;

		if (!first.m_isSmallString && !second.m_isSmallString && first.m_sharedString == second.m_sharedString)
			return true;

		return std::strcmp(first.GetConstBuffer(), second.GetConstBuffer()) == 0;
	}

	inline bool operator!=(const String& first, const String& second)
	{
		return !operator==(first, second);
	}

	inline bool operator<(const String& first, const String& second)
	{
		return std::strcmp(first.GetConstBuffer(), second.GetConstBuffer()) < 0;
	}

	inline bool operator<=(const String& first, const String& second)
	{
		return !operator<(second, first);
	}

	inline bool operator>(const String& first, const String& second)
	{
		return second < first;
	}

	inline bool operator>=(const String& first, const String& second)
	{
		return !operator<(first, second);
	}

	inline bool operator==(char character, const String& nstring)
	{
		return nstring == character;
	}

	inline bool operator==(const char* string, const String& nstring)
	{
		return nstring == string;
	}

	inline bool operator==(const std::string& string, const String& nstring)
	{
		return nstring == string;
	}

	inline bool operator!=(char character, const String& nstring)
	{
		return !operator==(character, nstring);
	}

	inline bool operator!=(const char* string, const String& nstring)
	{
		return !operator==(string, nstring);
	}

	inline bool operator!=(const std::string& string, const String& nstring)
	{
		return !operator==(string, nstring);
	}

	inline bool operator<(char character, const String& nstring)
	{
		return nstring > character;
	}

	inline bool operator<(const char* string, const String& nstring)
	{
		return nstring > string;
	}

	inline bool operator<(const std::string& string, const String& nstring)
	{
		return nstring > string;
	}

	inline bool operator<=(char character, const String& nstring)
	{
		return !operator<(nstring, String(character));
	}

	inline bool operator<=(const char* string, const String& nstring)
	{
		return !operator<(nstring, string);
	}

	inline bool operator<=(const std::string& string, const String& nstring)
	{
		return !operator<(nstring, string);
	}

	inline bool operator>(char character, const String& nstring)
	{
		return nstring < character;
	}

	inline bool operator>(const char* string, const String& nstring)
	{
		return nstring < string;
	}

	inline bool operator>(const std::string& string, const String& nstring)
	{
		return nstring < string;
	}

	inline bool operator>=(char character, const String& nstring)
	{
		return !operator<(character, nstring);
	}

	inline bool operator>=(const char* string, const String& nstring)
	{
		return !operator<(string, nstring);
	}

	inline bool operator>=(const std::string& string, const String& nstring)
	{
		return !operator<(string, nstring);
	}

	/*!
	* \brief Releases the content to the string
	*/
	inline void String::ReleaseString()
	{
		m_sharedString = std::move(GetEmptyString());
	}

	inline String::SmallString::SmallString(const String::SmallString& smallStr) :
	size(smallStr.size)
	{
		std::memcpy(buffer, smallStr.buffer, size);
	}

	inline String::SmallString& String::SmallString::operator=(const String::SmallString& smallStr)
	{
		size = smallStr.size;
		std::memcpy(buffer, smallStr.buffer, size);

		return *this;
	}

	/*!
	* \brief Constructs a SharedString object with a size
	*
	* \param strSize Number of characters in the string
	*/
	inline String::SharedString::SharedString(std::size_t strSize) :
	capacity(strSize), 
	size(strSize),
	string(new char[strSize + 1])
	{
		string[strSize] = '\0';
	}

	/*!
	* \brief Constructs a SharedString object with a size and a capacity
	*
	* \param strSize Number of characters in the string
	* \param strCapacity Capacity in characters in the string
	*/
	inline String::SharedString::SharedString(std::size_t strSize, std::size_t strCapacity) :
	capacity(strCapacity),
	size(strSize),
	string(new char[strCapacity + 1])
	{
		string[strSize] = '\0';
	}

	/*!
	* \brief Appends the string to the hash
	* \return true if hash is successful
	*
	* \param hash Hash to append data of the file
	* \param string String to hash
	*/
	inline bool HashAppend(AbstractHash* hash, const String& string)
	{
		hash->Append(reinterpret_cast<const UInt8*>(string.GetConstBuffer()), string.GetSize());
		return true;
	}
}

namespace std
{
	/*!
	* \brief Specialisation of std to hash
	* \return Result of the hash
	*
	* \param str String to hash
	*/
	template<>
	struct hash<Nz::String>
	{
		size_t operator()(const Nz::String& str) const
		{
			// Algorithme DJB2
			// http://www.cse.yorku.ca/~oz/hash.html

			size_t h = 5381;
			if (!str.IsEmpty())
			{
				const char* ptr = str.GetConstBuffer();

				do
					h = ((h << 5) + h) + *ptr;
				while (*++ptr);
			}

			return h;
		}
	};
}

#include <Nazara/Core/DebugOff.hpp>
