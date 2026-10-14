#include "KeyHandler.hpp"

auto KeyHandler::HandleKeyInput( uint16_t key ) -> void
{
	// New bindings registered by callbacks take effect on the next dispatch.
	const auto count = this->m_Keybinds.size( );
	for ( std::size_t i = 0; i < count; ++i )
	{
		const auto& keybind = this->m_Keybinds[ i ];
		if ( keybind.m_Key == key && keybind.m_Callback )
			keybind.m_Callback( );
	}
}
