#include <filecache.hpp>
#include <iostream>

using namespace std;

int main( int argc, char* argv[] ) {

	filecache::filecache cache( "/var/tmp/filecache", true );
	filecache::filecache cache2( "/var/tmp/filecache", true );

	char* name( "/u/mnm/fileCacheTest/badInterpolation.exr" );

	cache.resize( 6 ); // 6 MB cache
	// This will fail (the file is 7 MB big, the cache is only 6 MB)
	cerr << "'" << name << "' was cached as '" << cache.cacheFile( name ) << "'." << endl;

	cache.resize( 10 ); // 10 MB cache
	// This will work
	cerr << "'" << name << "' was cached as '" << cache.cacheFile( name ) << "'." << endl;

	return 0;
}
