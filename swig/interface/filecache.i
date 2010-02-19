/**
 * SWIG binding interface
 *
 */
%module filecache
%include "std_string.i"

namespace filecache {
class filecache {
	public:
		              filecache( bool = true );
		              filecache( const char*, bool = true );
		bool          operator==( const filecache& fc ) const;
		             ~filecache();

		std::string   cacheFile( const char* toCache );
		std::string   uncacheFile( const char*, bool = true, bool = false );
		std::string   cacheFileForWriting( const char* toCache );
		void          releaseFile( const char* );

		void          babble( bool );
		void          relocate( const char* );
		void          resize( unsigned long );

		unsigned long size() const;
		std::string   location() const;
};
}
