#pragma once

#include <cstdint>
#include <functional>
#include <utility>

class Keybind {
public:
	Keybind( uint16_t key, std::function<void()> callback ) : m_Callback( std::move( callback ) ), m_Key( key ) {};
	~Keybind( ) = default;

	std::function<void()> m_Callback;
	uint16_t m_Key;
private:
};