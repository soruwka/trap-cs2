#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Feature {
public:
	virtual ~Feature( ) = default;
	void virtual OnTick( ) = 0;
	std::string virtual GetName( ) = 0;
	bool virtual IsActive( ) = 0;
	int virtual GetHotkey( ) = 0;
};

class FeatureManager {
public:
	void TickAllFeatures( ) {
		const auto count = this->m_Features.size( );
		for ( std::size_t i = 0; i < count; ++i ) {
			auto* feature = this->m_Features[ i ].get( );
			if ( feature->IsActive( ) )
				feature->OnTick( );
		}
	}
		
	void AddFeature( std::unique_ptr<Feature> feature ) {
		if ( feature ) this->m_Features.emplace_back( std::move( feature ) );
	}

private:
	std::vector<std::unique_ptr<Feature>> m_Features;
};

inline std::unique_ptr<FeatureManager> g_FeatureManager = std::make_unique<FeatureManager>( );