#include <backend/keybinding/KeyHandler.hpp>
#include <classes/Vector.hpp>
#include <feature/Feature.hpp>

#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

void TestVectors( ) {
    const Vector3 a( 1.f, -2.f, 3.f );
    const Vector3 b( 4.f, 5.f, -6.f );
    assert( a + b == Vector3( 5.f, 3.f, -3.f ) );
    assert( a - b == Vector3( -3.f, -7.f, 9.f ) );
    assert( a.Sub( b ) == a - b );
    assert( a * b == Vector3( 4.f, -10.f, -18.f ) );
    assert( a.Multiply( 2.f ) == Vector3( 2.f, -4.f, 6.f ) );
    assert( a.Multiply( -1.f ) == Vector3( -1.f, 2.f, -3.f ) );
    assert( a.Multiply( 0.f ) == Vector3( ) );
    assert( a.Scale( 2.f ) == a * 2.f );
    assert( a != b );
    float scale = 2.f;
    assert( a * scale == a.Multiply( scale ) );
    const Vector3 origin;
    const Vector3 point( 2.f, 3.f, 6.f );
    assert( std::abs( origin.DistanceTo( point ) - 7.f ) < 0.0001f );
    assert( origin.DistanceTo( point ) == point.DistanceTo( origin ) );
    assert( a.DistanceTo( a ) == 0.f );
    const Vector2 origin2;
    const Vector2 point2( -3.f, 4.f );
    assert( origin2.DistanceTo( point2 ) == 5.f );
    assert( a.ToString( ) == "(1, -2, 3)" );
}

void TestKeybindings( ) {
    KeyHandler handler;
    handler.HandleKeyInput( 0 );
    std::vector<int> calls;
    handler.AddKeybind( Keybind( 0x1234, [ & ] { calls.push_back( 1 ); } ) );
    handler.AddKeybind( Keybind( 0x34, [ & ] { calls.push_back( 2 ); } ) );
    handler.AddKeybind( Keybind( 0x1234, { } ) );
    handler.AddKeybind( Keybind( 0x1234, [ & ] { calls.push_back( 3 ); } ) );
    handler.HandleKeyInput( 0x4321 );
    assert( calls.empty( ) );
    handler.HandleKeyInput( 0x1234 );
    assert( calls == std::vector<int>( { 1, 3 } ) );
    handler.HandleKeyInput( 0x34 );
    assert( calls == std::vector<int>( { 1, 3, 2 } ) );

    std::vector<int> states;
    handler.AddKeybind( Keybind( 7, [ &, state = 0 ]( ) mutable { states.push_back( ++state ); } ) );
    handler.HandleKeyInput( 7 );
    handler.HandleKeyInput( 7 );
    assert( states == std::vector<int>( { 1, 2 } ) );

    g_KeyHandler.reset( );
    int macroCalls = 0;
    if ( true )
        ADD_KEYBIND( Keybind( 9, [ & ] { ++macroCalls; } ) );
    else
        assert( false );
    g_KeyHandler->HandleKeyInput( 9 );
    assert( macroCalls == 1 );
    g_KeyHandler.reset( );
}

void TestKeybindingRegistration( ) {
    KeyHandler handler;
    bool registered = false;
    int firstCalls = 0;
    int addedCalls = 0;
    handler.AddKeybind( Keybind( 1, [ & ] {
        ++firstCalls;
        if ( registered ) return;
        registered = true;
        for ( int i = 0; i < 4096; ++i )
            handler.AddKeybind( Keybind( 1, [ & ] { ++addedCalls; } ) );
    } ) );
    handler.HandleKeyInput( 1 );
    assert( firstCalls == 1 && addedCalls == 0 );
    handler.HandleKeyInput( 1 );
    assert( firstCalls == 2 && addedCalls == 4096 );
}

class TestFeature : public Feature {
public:
    TestFeature( bool active, int& ticks, int& destroyed, std::function<void( )> onTick = { } )
        : m_Active( active ), m_Ticks( ticks ), m_Destroyed( destroyed ), m_OnTick( std::move( onTick ) ) { }
    ~TestFeature( ) override { ++m_Destroyed; }
    void OnTick( ) override {
        ++m_Ticks;
        if ( m_OnTick ) m_OnTick( );
    }
    std::string GetName( ) override { return "test"; }
    bool IsActive( ) override { return m_Active; }
    int GetHotkey( ) override { return 0; }

private:
    bool m_Active;
    int& m_Ticks;
    int& m_Destroyed;
    std::function<void( )> m_OnTick;
};

void TestFeatures( ) {
    static_assert( std::has_virtual_destructor_v<Feature> );
    int activeTicks = 0;
    int inactiveTicks = 0;
    int destroyed = 0;
    {
        FeatureManager manager;
        manager.TickAllFeatures( );
        manager.AddFeature( nullptr );
        manager.AddFeature( std::make_unique<TestFeature>( true, activeTicks, destroyed ) );
        manager.AddFeature( std::make_unique<TestFeature>( false, inactiveTicks, destroyed ) );
        manager.TickAllFeatures( );
        manager.TickAllFeatures( );
        assert( activeTicks == 2 && inactiveTicks == 0 && destroyed == 0 );
    }
    assert( destroyed == 2 );
}

void TestFeatureRegistration( ) {
    int initialTicks = 0;
    int addedTicks = 0;
    int destroyed = 0;
    bool registered = false;
    {
        FeatureManager manager;
        manager.AddFeature( std::make_unique<TestFeature>( true, initialTicks, destroyed, [ & ] {
            if ( registered ) return;
            registered = true;
            for ( int i = 0; i < 256; ++i )
                manager.AddFeature( std::make_unique<TestFeature>( true, addedTicks, destroyed ) );
        } ) );
        manager.TickAllFeatures( );
        assert( initialTicks == 1 && addedTicks == 0 );
        manager.TickAllFeatures( );
        assert( initialTicks == 2 && addedTicks == 256 );
    }
    assert( destroyed == 257 );
}

int main( int argc, char** argv ) {
    if ( argc != 2 ) return 1;
    const std::string test = argv[ 1 ];
    if ( test == "vectors" ) TestVectors( );
    else if ( test == "keybindings" ) TestKeybindings( );
    else if ( test == "keybinding_registration" ) TestKeybindingRegistration( );
    else if ( test == "features" ) TestFeatures( );
    else if ( test == "feature_registration" ) TestFeatureRegistration( );
    else return 1;
    std::cout << test << " passed\n";
}
