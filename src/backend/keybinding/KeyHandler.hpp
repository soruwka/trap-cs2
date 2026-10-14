#pragma once

#include "Keybind.hpp"
#include <cstddef>
#include <deque>
#include <memory>

#define ADD_KEYBIND(keybind) \
do { \
	if ( !g_KeyHandler ) g_KeyHandler = std::make_unique<KeyHandler>( ); \
	g_KeyHandler->AddKeybind( keybind ); \
} while ( false )

class KeyHandler {
public:
	explicit KeyHandler( ) = default;
	~KeyHandler( ) = default;

	auto HandleKeyInput( uint16_t key ) -> void;
	auto AddKeybind( const Keybind& keybind ) -> void { this->m_Keybinds.emplace_back( keybind ); };

private:
	std::deque<Keybind> m_Keybinds;
};

inline std::unique_ptr<KeyHandler> g_KeyHandler;