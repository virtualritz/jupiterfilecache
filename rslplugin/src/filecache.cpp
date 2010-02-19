#include <RslPlugin.h>
#include <rx.h>
#include <filecache.hpp>
#include <iostream>


typedef struct {
	filecache::filecache cache;
	string name;
} cacheData;



void destructor( RixContext* ctx, void* data ) {
	// This will release all files cached by this instance
	delete ( cacheData* )data;
}


inline cacheData* constructor( RslContext* rslContext  ) {
	cacheData * tempData( ( cacheData* )rslContext->GetLocalData() );
	if( !tempData ) {
		tempData = new cacheData;
		rslContext->SetLocalData( ( void* )tempData, destructor );

		RxInfoType_t type;
		int count;
		char* location;
		char** locationPtr( &location );

		if( !RxAttribute( "user:filecachelocation", ( void* )locationPtr, sizeof( char* ), &type, &count ) ) {
			if( type == RxInfoStringV ) {
				tempData->cache.relocate( location );
			}
		}
		
		RtInt size;
		if( !RxAttribute( "user:filecachesize", &size, sizeof( RtInt ), &type, &count ) ) {
			if( ( type == RxInfoInteger ) && ( 1 == count ) ) {
				tempData->cache.resize( size );
			}
		}

	}
	return tempData;
}


RSLEXPORT int rslCacheFile( RslContext* rslContext, int argc, const RslArg* argv[] ) {
	RslStringIter result( argv[ 0 ] );
	RslStringIter toCache( argv[ 1 ] );

	cacheData* tempData( constructor( rslContext ) );

	tempData->name = tempData->cache.cacheFile( *toCache );

	*result = const_cast< char* >( tempData->name.c_str() );

	return 0;
}


RSLEXPORT int rslCacheFileForWriting( RslContext* rslContext, int argc, const RslArg* argv[] ) {
	RslStringIter result( argv[ 0 ] );
	RslStringIter toCache( argv[ 1 ] );

	cacheData* tempData( constructor( rslContext ) );

	tempData->name = tempData->cache.cacheFileForWriting( *toCache );

	*result = const_cast< char* >( tempData->name.c_str() );

	return 0;
}


RSLEXPORT int rslUnCacheFile( RslContext* rslContext, int argc, const RslArg* argv[] ) {
	RslStringIter result( argv[ 0 ] );
	RslStringIter toCache( argv[ 1 ] );

	cacheData* tempData( constructor( rslContext ) );

	tempData->name = tempData->cache.uncacheFile( *toCache );

	*result = const_cast< char* >( tempData->name.c_str() );

	return 0;
}


static RslFunction cacheFunctions[] =
{
    { "string cacheFile(string)", rslCacheFile, NULL, NULL },
    { "void cacheFileForWriting(string)", rslCacheFileForWriting, NULL, NULL },
    { "void unCacheFile(string)", rslUnCacheFile, NULL, NULL },
    NULL
};
RSLEXPORT RslFunctionTable RslPublicFunctions( cacheFunctions );


