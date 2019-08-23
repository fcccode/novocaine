/*
 * novocaine - hooking (redirection) library for Windows x86
 */

#pragma once
#ifndef NOVOCAINE_H
#define NOVOCAINE_H

#include <Windows.h>
#include <memory>

namespace novocaine
{
	constexpr unsigned char x86_far_jmp = 0xE9;

	class redirect
	{
		bool _has_trampoline = false;
		void* _jump_ptr = nullptr;
		void* _redirect_ptr = nullptr;
		void* _original_ptr = nullptr;
		void* _trampoline_ptr = nullptr;
		unsigned long _code_size = 0;

		void* _build_trampoline() const noexcept
		{
			if (_original_ptr == nullptr)
				return nullptr;
			unsigned long code_address = reinterpret_cast<unsigned long>(_redirect_ptr);
			unsigned long after_address = code_address + _code_size;
			unsigned char* trampoline = new (std::nothrow) unsigned char[_code_size + 5];
			if (trampoline == nullptr)
				return nullptr;
			std::memmove(trampoline, _original_ptr, _code_size);
			*(trampoline + _code_size) = x86_far_jmp;
			*reinterpret_cast<unsigned long*>(trampoline + _code_size + 1) =
				(after_address - reinterpret_cast<unsigned long>(trampoline + _code_size) - 5);
			return trampoline;
		}

		unsigned long _set_page_permissions(void* page, unsigned long permissions) const
		{
#pragma region winapi (VirtualProtect)
			DWORD nOldProtect = 0;
			if (!VirtualProtect(page, _code_size, permissions, &nOldProtect))
				return static_cast<unsigned long>(false);
			return nOldProtect;
#pragma endregion
		}
	public:
		template <typename T>
		T trampoline() noexcept
		{
			if (_has_trampoline)
				goto return_trampoline;
			_trampoline_ptr = _build_trampoline();
			if (_trampoline_ptr == nullptr)
				return nullptr;
			_has_trampoline = true;
		return_trampoline:
			return reinterpret_cast<T>(_trampoline_ptr);
		}

		bool fast_transact() const noexcept
		{
			unsigned long previous_permissions = _set_page_permissions(_redirect_ptr, PAGE_EXECUTE_READWRITE);
			if (previous_permissions == 0)
				return false;
			unsigned long redirect_addr = reinterpret_cast<unsigned long>(_redirect_ptr);
			unsigned long jmp_addr = reinterpret_cast<unsigned long>(_jump_ptr);
			*reinterpret_cast<unsigned char*>(redirect_addr) = x86_far_jmp;
			*reinterpret_cast<unsigned long*>(redirect_addr + 1) = (jmp_addr - redirect_addr - 5);
			_set_page_permissions(_redirect_ptr, previous_permissions);
			return true;
		}

		bool transact() noexcept
		{
			if (_original_ptr == nullptr)
			{
				_original_ptr = new (std::nothrow) unsigned char[_code_size];
				if (_original_ptr == nullptr)
					return false;
				std::memmove(_original_ptr, _redirect_ptr, _code_size);
			}
			return fast_transact();
		}

		bool rewind() const noexcept
		{
			unsigned long previous_permissions = _set_page_permissions(_redirect_ptr, PAGE_EXECUTE_READWRITE);
			if (previous_permissions == 0)
				return false;
			std::memmove(_redirect_ptr, _original_ptr, _code_size);
			_set_page_permissions(_redirect_ptr, previous_permissions);
			return true;
		}

		bool forward(void* jump_to) noexcept
		{
			_jump_ptr = jump_to;
			return fast_transact();
		}

		bool flush() noexcept
		{
			delete[] reinterpret_cast<unsigned char*>(_original_ptr);
			_original_ptr = nullptr;
			return true;
		}

		template <typename T1, typename T2>
		redirect(T1 function, T2 jump_to, unsigned long code_size = 5)
		{
			_redirect_ptr = reinterpret_cast<void*>(function);
			_jump_ptr = reinterpret_cast<void*>(jump_to);
			_code_size = code_size;
		}

		~redirect()
		{
			delete[] reinterpret_cast<unsigned char*>(_trampoline_ptr);
			delete[] reinterpret_cast<unsigned char*>(_original_ptr);
		}
	};
}

#endif