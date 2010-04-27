/**@file
 *
 * Multi loaction, multi-process, thread-safe file cache library.
 *
 * @par License:
 * Copyright (C) 2007, 2010  Moritz Moeller
 * @par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 * @par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * @par
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA or point your web browser to
 * http://www.gnu.org/licenses/lgpl.txt
 *
 * @author Moritz Moeller (realritz@virtualritz.com)
 *
 * @par Disclaimer:
 * Moritz Moeller hereby disclaims all copyright interest in the
 * library FileCache (a program to cache files located on remote
 * loactions into a local directory) written by Moritz Moeller.
 * @par
 * Any one who uses this code does so completely at their own risk.
 * The author doesn't warrant that this code does anything at all
 * but if it does something and you don't like it, then he is not
 * responsible.
 * @par
 * Have a nice day!
 *
 */
// Own headers
#include <filecache.hpp>

// Standard headers
#include <iostream> // cerr
#include <sstream> // istringstream

// System headers
#include <errno.h> // errno
// superblock magic number for NFS -- this should better come from
// linux/nfs_fs.h!!!
// Need to investigate why we have missing include dependecies with
// linux/nfs_fs.h on Fedora Bore.
// Works fine on Gentoo. :)


//#include <linux/nfs_fs.h> // NFS_SUPER_MAGIC
#ifndef NFS_SUPER_MAGIC
#define NFS_SUPER_MAGIC       0x00006969
#endif


#include <signal.h> // kill()
#include <sys/stat.h> // stat()
#include <sys/statvfs.h> // statvfs()
#if defined( LINUX ) && defined( USEPROC )
// proc/readproc.h is yet another header missing from Fedora Bore, it seems. :(
# include <proc/readproc.h>
#endif


// Boost headers
#include <boost/algorithm/string/replace.hpp> // replace_all()
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_array.hpp>


#ifdef DEBUG
# define DEBUGMSG(a) cerr << "[FileCache] DEBUG: " << (a) << endl;
#else
# define DEBUGMSG(a)
#endif


namespace Jupiter {


    FileCache::Inventory FileCache::cacheInventory_;
    FileCache::PathSizeMap FileCache::cacheSize_;
    FileCache::ProcessCounterInventory FileCache::instanceCounter_;

    /**
     * Creates a new cache instance.
     *
     * Cache location can be shared bewteen instances. Note that cache sizes are
     * also shared between instances. I.e. setting the cache size for one instance
     * sets the cache size for all otehr instances sharing the cache location with
     * this instance.
     * @par
     *
     * @param  where The location for the cache. If this is empty, the cache will
     *                look for an environment variable called FILECACHE_LOCATION
     *                and use its value.
     *                If this variable is not found or its contens is not a valid
     *                path the default location "/var/tmp/_cache" ist used.
     *
     */
    FileCache::FileCache( bool activate )
    {
        WriteGuard guard( mutex_ );

        init_cache( fs::path(), activate );
    }

    FileCache::FileCache( const fs::path& where, bool activate )
    {
        WriteGuard guard( mutex_ );

        init_cache( where, activate );
    }

    FileCache::FileCache( const std::string& where, bool activate )
    {
        WriteGuard guard( mutex_ );

        if ( !where.empty() ) {
            init_cache( where, activate );
        } else {
            init_cache( fs::path(), activate );
        }
    }


    /**
     * Copy constructor.
     *
     * Rule of three. :)
     *
     */
    FileCache::FileCache( const FileCache& fc )
    {
        WriteGuard guard( mutex_ );

        cwd_ = fc.cwd_;
        cacheLocation_ = fc.cacheLocation_;
        cacheSize_[ cacheLocation_ ] = fc.cacheSize_[ fc.cacheLocation_ ];
        cache_ = fc.cache_;
        log_ = fc.log_;
    }


    /**
     * Assignment operator.
     *
     * Rule of three. :)
     *
     */
    FileCache& FileCache::operator=( const FileCache& fc )
    {
        ReadGuard guard( mutex_ );

        if ( this != &fc ) {
            WriteGuard guard( mutex_, boost::adopt_lock );

            cwd_ = fc.cwd_;
            cacheLocation_ = fc.cacheLocation_;
            cacheSize_[ cacheLocation_ ] = fc.cacheSize_[ fc.cacheLocation_ ];
            cache_ = fc.cache_;
            log_ = fc.log_;
        }

        return *this;
    }


    bool FileCache::operator==( const FileCache& fc ) const
    {
        ReadGuard guard( mutex_ );
        return cacheLocation_ == fc.cacheLocation_;
    }


    /**
     * Destructor.
     *
     * Releases all files used by the current process.
     *
     */
    FileCache::~FileCache()
    {
        WriteGuard guard( mutex_ );

        ipd::OS_process_id_t id( ipd::get_current_process_id() );

        erase_this_reference();

        if ( instanceCounter_[ id ].empty() ) { // No more instances in the process

            if ( cacheInventory_[ cacheLocation_ ][ id ].empty() ) {
                // In theory this can never return cacheInventory_[ cacheLocation_ ].end(), but maybe we should check???
                cacheInventory_[ cacheLocation_ ].erase( cacheInventory_[ cacheLocation_ ].find( id ) );
            }

            // No other processes using this cacheLocation?
            if ( cacheInventory_[ cacheLocation_ ].empty() ) {
                // In theory this can never return cacheInventory.end(), but maybe we should check???
                cacheInventory_.erase( cacheInventory_.find( cacheLocation_ ) );
            }
        }
    }


    void FileCache::releaseFile( const fs::path& path )
    {
        WriteGuard guard( mutex_ );

        // Release a file from this process's cache inventory
        std::set< fs::path >& thisFileInventory( cacheInventory_[ cacheLocation_ ][ ipd::get_current_process_id() ][ reference_ ] );
        std::set< fs::path >::const_iterator it( thisFileInventory.find( path ) );

        if ( thisFileInventory.end() != it ) {
            thisFileInventory.erase( it );
        }
    }


    void FileCache::babble( bool logging )
    {
        WriteGuard guard( mutex_ );

        log_ = logging;
    }


    void FileCache::relocate( const fs::path& where )
    {
        WriteGuard guard( mutex_ );

        relocate_cache( where );
    }


    void FileCache::resize( uintmax_t megaByteSize )
    {
        WriteGuard guard( mutex_ );

        // Megabytes, not Mebibytes :)
        cacheSize_[ cacheLocation_ ] = megaByteSize * 1000000;
    }


    fs::path FileCache::cacheFile( const fs::path& toCache )
    {

        ReadGuard guard( mutex_ );

        try {
            if ( cache_ ) {
                WriteGuard guard( mutex_, boost::adopt_lock );

                fs::path source;

                if ( is_symlink( toCache ) ) {
                    source = read_link( toCache );
                } else {
                    source = toCache;
                }

                fs::path result;

                if ( is_remote( source ) ) {
                    fs::path destination( cached_file_path( source ) );

                    // Does the file exist?
                    if ( fs::exists( destination ) ) {
                        // Is it used by another process?
                        if ( is_used( destination ) ) {
                            // Is it the same as the original?
                            if ( !is_different( source, destination ) || is_used_by_this_cache( destination ) ) {
                                // Best case: destination exists, is used but not different -- mark it
                                DEBUGMSG( "RegisterInCache '" + destination.string() + "' exists in cache and is equal to original or different but already used by this cache instance" );
                                register_file( destination );
                                result = destination;
                            }

                            /* If we ended up here the file
                             * is outdated but used elsewhere,
                             * so we can't update the cache :|
                             */
                        } else if ( is_different( source, destination ) ) {
                            // Destination already exists and isn't used but it is different
                            DEBUGMSG( "Copy2Cache '" + destination.string() + "' exists in cache and is not used but different to original '" + source.string() + "'" );
                            result = copy_to_cache( source, destination );
                        } else {
                            // Best case: destination exists, is not used nor different -- mark it
                            DEBUGMSG( "RegisterInCache '" + destination.string() + "' exists in cache and is equal to original '" + source.string() + "'" );
                            register_file( destination );
                            result = destination;
                        }
                    } else {
                        // Destination doesn't exist
                        DEBUGMSG( "RegisterInCache '" + destination.string() + "' does not exist in cache" );
                        result = copy_to_cache( source, destination );
                    }
                } else {
                    // It's a local file
                    DEBUGMSG( "Ignoring '" + source.string() + "' since it is a local file" );
                }

                return result;
            }
        } catch ( fs::filesystem_error ) {
            // Anything goes wrong we play it safe and return the unalterted path
            message( "File '" + toCache.string() + "' was not cached." );
        }

        DEBUGMSG( "CacheDeactivated '" + toCache.string() + "' ignored" );

        return toCache;
    }


    std::string FileCache::cacheFile( const std::string& toCache )
    {
        WriteGuard guard( mutex_ );
        return cacheFile( fs::path( toCache ) ).string();
    }


    fs::path FileCache::cacheFileForWriting( const fs::path& toCache )
    {
        ReadGuard guard( mutex_ );

        if ( cache_ ) {
            WriteGuard guard( mutex_, boost::adopt_lock );

            fs::path source;

            if ( is_symlink( toCache ) ) {
                source = read_link( toCache );
            } else {
                source = toCache;
            }

            fs::path result;

            if ( is_remote( source ) ) {
                fs::path destination( cached_file_path( source ) );

                // Does the file exist?
                if ( fs::exists( destination ) ) {
                    // Is it used by another process?
                    if ( is_used( destination ) ) {
                        // We can't allow writing to a file that is being used
                        DEBUGMSG( "RegisterInCache '" + destination.string() + "' exists in cache and is being used" );
                        result = source;
                    } else {
                        // Destination exists but is not -- mark it
                        DEBUGMSG( "RegisterInCache '" + destination.string() + "' exists in cache but is not being used" );
                        register_file( destination );
                        result = destination;
                    }
                } else {
                    // Destination doesn't exist
                    result = destination;
                }
            } else {
                DEBUGMSG( "Ignoring local file '" + source.string() + "'" );
                result = source;
            }

            return result;
        }

        return toCache;
    }


    std::string FileCache::cacheFileForWriting( const std::string& toCache )
    {
        WriteGuard guard( mutex_ );
        return cacheFileForWriting( fs::path( toCache ) ).string();
    }


    fs::path FileCache::uncacheFile( const fs::path& fromCache, bool overwrite, bool ifNewer )
    {

        fs::path destination( original_file_path( fromCache ) );

        ReadGuard guard( mutex_ );

        try {
            if ( cache_ ) {
                WriteGuard guard( mutex_, boost::adopt_lock );

                if ( is_used_by_this_cache( fromCache ) ) {
                    if ( fs::exists( destination ) ) {
                        // Check if our destination is outdated
                        if ( !ifNewer ||
                             ( ifNewer &&
                               ( fs::last_write_time( destination ) <
                                       fs::last_write_time( fromCache ) ) ) ) {
                            // Copy from cache
                            if ( overwrite ) {
                                copy_overwrite_file( fromCache, destination );
                            } else {
                                fs::copy_file( fromCache, destination );
                            }
                        } else {
                            message( "File has same or older timestamp." );
                            //throw fs::filesystem_error( std::string( "Original file has same or older timestamp as destination." ), fromCache, destination, boost::system::errc::file_exists  );
                        }
                    } else {
                        copy_overwrite_file( fromCache, destination );
                    }
                } else {
                    message( "File is not registered in this cache instance" );
                    //throw( fs::filesystem_error() );
                }

                return destination;
            }
        } catch ( fs::filesystem_error ) {
            // Anything goes wrong we play it safe and return the unalterted path
            message( "Could not copy file '" + fromCache.string() + "' back to '" + destination.string() + "'" );
        }

        return fromCache;
    }


    std::string FileCache::uncacheFile( const std::string& fromCache, bool overwrite, bool ifNewer )
    {
        WriteGuard guard( mutex_ );

        return uncacheFile( fs::path( fromCache ) ).string();
    }


    void FileCache::init_cache( const fs::path& where, bool activate )
    {
        // Current working directory -- needs to be stored per class instance
        cwd_ = fs::current_path();

        if ( where.empty() ) {
            char* env( std::getenv( "FILECACHE_LOCATION" ) );

            try {
                if ( env ) {
                    cacheLocation_ = env;
                }

                if ( cacheLocation_.empty() ) {
                    //throw( fs::filesystem_error() );
                }
            } catch ( fs::filesystem_error ) {
                cacheLocation_ = "/var/tmp/_cache";
            }
        } else {
            cacheLocation_ = where;
        }

        char* size( getenv( "FILECACHE_SIZE" ) );

        if ( size ) {
            cacheSize_[ cacheLocation_ ] = boost::lexical_cast< intmax_t >( size );
        } else {
            cacheSize_[ cacheLocation_ ] = 0;
        }

        processName_ = get_process_name();

        if ( create_full_path( cacheLocation_ ) ) {
            if ( is_remote( cacheLocation_ ) ) {
                // Caching is useless if the cache itself is remote
                cache_ = false;
                log_ = false;
            } else {
                cache_ = activate;
                log_ = true;
            }
        } else {
            message( "Could not create cache location '" + cacheLocation_.string() + "'" );
            cache_ = false;
            log_ = true;
        }

        // Create a unique reference_ id for this instance under this process
        ipd::OS_process_id_t id( ipd::get_current_process_id() );

        do {
            reference_ = random();
        } while ( instanceCounter_[ id ].end() != instanceCounter_[ id ].find( reference_ ) );

        // Register this instance
        instanceCounter_[ id ].insert( reference_ );
        cacheInventory_[ cacheLocation_ ][ id ][ reference_ ] = std::set< fs::path >();
    }


    void FileCache::relocate_cache( const fs::path& where )
    {
        if ( cacheLocation_ != where ) {
            erase_this_reference();

            cacheLocation_ = where;

            if ( create_full_path( cacheLocation_ ) ) {
                if ( is_remote( cacheLocation_ ) ) {
                    // Caching is useless if the cache itself is remote
                    cache_ = false;
                }
            } else {
                message( "Could not create cache location '" + cacheLocation_.string() + "'" );
                cache_ = false;
            }

            // Register this instance at the new location
            ipd::OS_process_id_t id( ipd::get_current_process_id() );

            instanceCounter_[ id ].insert( reference_ );
            cacheInventory_[ cacheLocation_ ][ id ][ reference_ ] = std::set< fs::path >();
        }
    }


    void FileCache::erase_this_reference()
    {
        ipd::OS_process_id_t id( ipd::get_current_process_id() );

        std::set< unsigned >::const_iterator referenceIt( instanceCounter_[ id ].find( reference_ ) );

        if ( instanceCounter_[ id ].end() != referenceIt ) {
            instanceCounter_[ id ].erase( referenceIt );
        }

        ReferenceInventory::iterator inventoryIt( cacheInventory_[ cacheLocation_ ][ id ].find( reference_ ) );

        if ( cacheInventory_[ cacheLocation_ ][ id ].end() != inventoryIt ) {
            cacheInventory_[ cacheLocation_ ][ id ].erase( inventoryIt );
        }
    }


    void FileCache::copy_overwrite_file( const fs::path& source, const fs::path& destination ) const
    {
        if ( fs::exists( destination ) ) {
            fs::remove( destination );
        }

        fs::copy_file( source, destination );
    }


    fs::path FileCache::cached_file_path( const fs::path& toCache ) const
    {

        fs::path tmpPath;

        if ( toCache.has_root_directory() ) {
            tmpPath = toCache;
        } else {
            tmpPath = cwd_ / toCache; // Prepend by current working dir
        }

        std::string newPathString( tmpPath.string() );
        // The following works on any platform since string() returns the internal
        // representation of a path which always uses '/' as the separator char.
        boost::algorithm::replace_all( newPathString, "/", "%" );

        return cacheLocation_ / fs::path( newPathString, fs::no_check );
    }


    /**
     * Transform cached locations back to the source.
     *
     * @return  the original path name
     */
    fs::path FileCache::original_file_path( const fs::path& fromCache ) const
    {

        // Strip any path in front of file name (the "leaf").
        std::string newPathString( fromCache.leaf() );
        boost::algorithm::replace_all( newPathString, "%", "/" );

        return fs::path( newPathString );
    }


    /**
     * Get a cached location for write caching.
     *
     * @return  the cached path if cache was true, toCache otherwise
     */
    fs::path FileCache::cached_file_name( const fs::path& toCache ) const
    {
        if ( cache_ ) {
            try {
                return cached_file_path( toCache );
            } catch ( fs::filesystem_error ) {
                // Anything goes wrong we play it safe and return the unalterted path.
                message( "Could not form valid cache file name from '" + toCache.string() + "'" );
            }
        }

        return toCache;
    }


    bool FileCache::is_remote( const fs::path& toCache ) const
    {
        fs::path tmpPath( toCache );

        // Resolve symlinks prior to caching
        if ( fs::is_symlink( tmpPath ) ) {
            tmpPath = read_link( tmpPath );
        }

        std::string pathStr( tmpPath.remove_leaf().string() );

        // Check if the file is on a mounted location
        struct statvfs stats;

        if ( !statvfs( pathStr.c_str(), &stats ) ) {
            return NFS_SUPER_MAGIC == stats.f_fsid; // || ( SMB_SUPER_MAGIC == stats.f_type );
        }

        return false;
    }


    bool FileCache::is_different( const fs::path& toCache, const fs::path& destination ) const
    {
        if ( // check if the remote file is newer than the (possibly) existing file in the cache
            ( fs::last_write_time( destination ) < fs::last_write_time( toCache ) ) ||
            // Also check if the size is different
            ( fs::file_size( destination ) != fs::file_size( toCache ) ) ) {
            return true;
        }

        return false;
    }


    bool FileCache::is_used_by_this_cache( const fs::path& path ) const
    {
        std::set< fs::path >& thisCache( cacheInventory_[ cacheLocation_ ][ ipd::get_current_process_id() ][ reference_ ] );

        return( thisCache.count( path ) );
    }


    bool FileCache::is_used( const fs::path& path ) const
    {
        const ProcessInventory& thisProcessInventory( cacheInventory_[ cacheLocation_ ] );

        for ( ProcessInventory::const_iterator it( thisProcessInventory.begin() ); it != thisProcessInventory.end(); ++it ) {
            const ReferenceInventory& thisReferenceInventory( it->second );

            for ( ReferenceInventory::const_iterator jt( thisReferenceInventory.begin() ); jt != thisReferenceInventory.end(); ++jt ) {
                if ( jt->second.count( path ) ) {
                    return true;
                }
            }
        }

        /*for( processInventory::const_iterator it( thisInventory.begin() ); it != thisInventory.end(); ++it ) {
        	//if( it->second.end() != it->second.find( path ) ) {
        	if( it->second.count( path ) ) {
        		return true;
        	}
        }*/
        return false;
    }


    void FileCache::register_file( const fs::path& path )
    {
        // Register the file for the current process
        cacheInventory_[ cacheLocation_ ][ ipd::get_current_process_id() ][ reference_ ].insert( path );
    }


    /**
     * Physically copy a file to the cache
     *
     * @return  the cached path if sucessful, the unaltered original path otherwise
     */
    fs::path FileCache::copy_to_cache( const fs::path& toCache, const fs::path& destination )
    {
        try {
            if ( tidy_up_cache( toCache ) ) {
                copy_overwrite_file( toCache, destination );
                register_file( destination );
                return destination;
            }

            // Cache was full and/or couldn't be tidied up enough...
        } catch ( ... ) {
            // Anything goes wrong we play it safe and return the unalterted path
            message( "Copying '" + toCache.string() + "' to '" + destination.string() + "' failed" );
        }

        return toCache;
    }


    /**
     * Createas a full path.
     *
     * Iteratoes over the path and consecutively creates each directory, if
     * nonexistant.
     *
     * @return  true if successfull, false otherwise
     */
    bool FileCache::create_full_path( const fs::path& createPath ) const
    {
        fs::path create;

        try {
            for ( fs::path::iterator it( createPath.begin() ); it != createPath.end(); ++it ) {
                create /= *it;

                if ( !fs::exists( create ) )
                    { fs::create_directory( create ); }
            }

            return true;
        } catch ( ... ) {
            message( "Creating directory '" + createPath.string() + "' failed" );
        }

        return false;
    }


    /**
     * Get the last access time of the given file
     *
     * @return  the access time if successfull, 0 otherwise
     */
    time_t FileCache::last_access_time( const fs::path& createPath ) const
    {
        struct stat fstats;

        if ( !stat( createPath.string().c_str(), &fstats ) ) {
            return fstats.st_atime;
        }

        return 0; // January 1, 1970 :)
    }


    /**
     * Tidies up the cache inventory by purging all inventory lists whose
     * processes don't exist anymore
     *
     */
    void FileCache::tidy_up_inventory()
    {
        ProcessInventory& thisInventory( cacheInventory_[ cacheLocation_ ] );
        ipd::OS_process_id_t id( ipd::get_current_process_id() );

        for ( ProcessInventory::iterator it( thisInventory.begin() ); it != thisInventory.end(); ++it ) {
            if ( ( id != it->first ) && !( !kill( it->first, 0 ) || ( ESRCH != errno ) ) ) {
                thisInventory.erase( it );
            }
        }
    }


    /**
     * Tidies up the cache
     *
     */
    bool FileCache::tidy_up_cache( const fs::path& toCache )
    {

        if ( cacheSize_[ cacheLocation_ ] ) {
            std::multimap< time_t, fs::path > files;
            std::multimap< time_t, intmax_t > sizes;

            uintmax_t totalSize;

            if ( !toCache.empty() ) {
                totalSize = fs::file_size( toCache );
            } else {
                totalSize = 0;
            }

            // Find all files and their sizes
            fs::directory_iterator end;

            for ( fs::directory_iterator it( cacheLocation_ ); it != end; ++it ) {
                if ( !is_directory( *it ) ) {
                    time_t t( last_access_time( *it ) );
                    files.insert( std::pair< time_t, fs::path >( t, *it ) );

                    intmax_t s( fs::file_size( *it ) );
                    sizes.insert( std::pair< time_t, intmax_t >( t, s ) );

                    totalSize += s;
                }
            }

            // Tidy up our inventory so we don't keep files of other cache-using processes that got killed
            tidy_up_inventory();

            // We need to tidy up the cache
            if ( totalSize > cacheSize_[ cacheLocation_ ] ) {
                std::multimap< time_t, fs::path >::const_iterator f( files.begin() );
                std::multimap< time_t, intmax_t >::const_iterator s( sizes.begin() );

                // Delete oldest files first
                while ( files.end() != f ) {
                    if ( !is_used( f->second ) ) {
                        fs::remove( f->second );
                        totalSize -= s->second;

                        if ( totalSize < cacheSize_[ cacheLocation_ ] ) {
                            // We made enough room
                            return true;
                        }
                    }

                    ++f;
                    ++s;
                }

                // We couldn't make enough room
                return false;
            }

            // The cache is big enough
            return true;

        } else {
            // A zero size cache is unlimited, return success
            return true;
        }
    }


    /**
     * Safe implementation of read_link, requiring no memory care after calling
     *
     * boost::filesystem v3, once released, will have a method for this we can use instead.
     * See http://mysite.verizon.net/beman/v3/v3.html
     *
     * @return  the dereferenced link if successful, the original otherwise
     */
    fs::path FileCache::read_link( const fs::path& link ) const
    {
        enum { GROWBY = 256 }; // How large we will grow strings by

        int bufSize( 0 ), readSize( 0 );
        boost::shared_array< char > buf;

        do {
            buf = boost::shared_array< char >( new char[ bufSize += GROWBY ] );
            readSize = readlink( link.string().c_str(), buf.get(), bufSize );

            if ( -1 == readSize ) {
                return link;
            }
        } while ( bufSize < readSize + 1 );

        buf[ readSize ] = '\0';

        return fs::path( buf.get(), fs::no_check );
    }


    /* Fixed old implementation. The original was leaking memory
     * Needs testing that this is really fixed. In the meantime,
     * above implementation uses boost::shared_array -- possibly
     * (but not neccessarily thanks to good optimizers these days)
     * less fast but guaranteed to do the right thing.
    fs::path FileCache::read_link( const fs::path& link ) const {
    	enum { GROWBY = 256 }; // How large we will grow strings by

    	int bufSize( 0 ), readSize( 0 );
    	char* buf( 0 );

    	do {
            if( !buf ) {
                delete[] buf;
            }
    		buf = new char[ bufSize += GROWBY ];
    		readSize = readlink( link.string().c_str(), buf, bufSize );
    		if( -1 == readSize ) {
                delete[] buf;
    			return link;
    		}
    	} while( bufSize < readSize + 1 );

    	buf[ readSize ] = '\0';
    	fs::path path( buf, fs::no_check );

        delete[] buf;

        return path;
    }*/


    inline void FileCache::message( const std::string& message ) const
    {
        if ( log_ ) {
            WriteGuard guard( messageMutex_ );

            if ( processName_.empty() ) {
                printf( "[FileCache] WARNING: %s\n", message.c_str() );
            } else {
                printf( "[FileCache:%s] WARNING: %s\n", processName_.c_str(), message.c_str() );
            }
        }
    }


#if defined( LINUX ) && defined( USEPROC )
    /**
     * Get the name of the current process via the /proc filesystem
     *
     * Only works on Linux, if /proc is mounted.
     *
     */
    std::string FileCache::get_process_name() const
    {
        pid_t pid( getpid() );
        PROCTAB* procTp( openproc( PROC_FILLBUG ) );

        if ( procTp ) {
            proc_t buf;

            while ( ps_readproc( procTp, &buf ) ) {
                if ( buf.pid == pid ) {
                    return buf.cmdline[ 0 ];
                }
            }
        }

        return std::string();
    }
#else
    std::string FileCache::get_process_name() const
    {
        return std::string();
    }
#endif


} // namespace Jupiter

