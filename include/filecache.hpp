/**@file
 *
 * Multi location, multi process, thread safe file cache library.
 *
 * @par License:
 * Copyright (C) 2007 Moritz Moeller
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
 * library filecache (a program to cache files located on remote
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

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/filesystem/path.hpp>
#include <map>
#include <set>

namespace fs = boost::filesystem;
namespace ipd = boost::interprocess::detail;

namespace Jupiter {

/**
 * Multi location, multi process, thread safe file cache class.
 *
 * Manages local caches that mirror files in remote locations.
 *
 * @par Precautions
 * The following precautions are taken to ensure the proper operation of the
 * cache:
 * -# When used as a read cache, the filecache lib does not access the original
 *    file in any dangerous way, only reading is performed on those files.
 * -# The cache is kept synchronized with the files it mirrors: if an original
 *    file is newer than the cached one (or has a different size), the cache is
 *    updated; unless the file is being used by another cache instance on the
 *    same machine.
 * -# A file is identified by its full path: files that have the same name in
 *    different directories do not collide.
 * -# Symbolic links are resolved prior to caching, this ensures that a given
 *    file is cached only once even if many links point to it.
 * -# The filecache is multi-process- and -thread safe. Even if many
 *    processe are running on the same machine, sharing a cache, the cache is
 *    kept in a consistent state: one filecache instance does not remove or
 *    update a file used by another instance in a different process and/or
 *    thread!
 * -# When the cache is used as a write cache, only one instance can use a
 *    particular write location in a cache.
 * -# When copying write-cached files back, the cache can check if the file
 *    already exists at the destination and/or has a newer date. By default,
 *    existing files are only overwritten if they are older than the write-
 *    cached file.
 * -# If, for any reason, the filecache lib is unable to cache a file, it
 *    reverts to use the original, and this is the worst case scenario.
 *
 * @par Thread safety
 * The cache always does obtain a mutex lock before accessing or modifying any
 * data.
 * @par
 * Cache locations are referenced per class instance. This ensures that there
 * can be more that one cache instance per location per process.
 * The destructor of any instance of a cache in any thread of the same process
 * at the same cache location will release the files of that cache location for
 * this instance only. Other instances of the cache with this location under
 * the same process will keep shared files locked throughout their lifetime.
 *
 */
class FileCache {
	public:

		/**
		 * Creates a new cache instance.
		 *
		 * @par
		 * Cache location can be shared bewteen instances. Note that cache sizes are
		 * also shared between instances. I.e. setting the cache size for one instance
		 * sets the cache size for all otehr instances sharing the cache location with
		 * this instance.
		 * @par
		 * The cache looks for two environment variables, FILECACHE_LOCATION and
		 * FILECACHE_SIZE which have the obvious meanings.  The size is specified in
		 * in Megabytes (multiples of 1,000,000), not Mebibytes (multiples of
		 * 1,048,576).
		 *
		 * @param  where  The location for the cache. If this is empty, the cache will
		 *                look for an environment variable called FILECACHE_LOCATION
		 *                and use its value.
		 *                If this variable is not found or its contens is not a valid
		 *                path
		 * @param  activate  If false, tuns the cache off altogether. This is mainly
		 *                   for debug purposes. When switched off, all cache requests
		 *                   will always return the original filename.
		 *
		 */
                      FileCache( bool activate = true );
		              FileCache( const fs::path& where, bool activate = true );
                      FileCache( const std::string& where, bool activate = true );
		/**
		 * Copy constructor.
		 *
		 * <a href="http://en.wikipedia.org/wiki/Rule_of_three_(C++_programming)">Rule of three</a>. :)
		 *
		 */
		              FileCache( const FileCache& fc );
		/**
		 * Assignment operator.
		 *
		 * <a href="http://en.wikipedia.org/wiki/Rule_of_three_(C++_programming)">Rule of three</a>. :)
		 *
		 */
		FileCache&    operator=( const FileCache& fc );
		bool          operator==( const FileCache& fc ) const;
		/**
		 * Destructor.
		 *
		 * <a href="http://en.wikipedia.org/wiki/Rule_of_three_(C++_programming)">Rule of three</a>. :)
		 * @par
		 * This also cleans up the shared cached files reference database for this
		 * instance. This ensures search times in the database stay as low as possible,
		 * even if the filecache library is kept "alive" in memory for weeks or moths
		 * on a machine that always runs at least one process using it.
		 *
		 */
                     ~FileCache();

		/**
		 * Cache a file.
		 *
		 * @par
		 * In the special case where the file exists in the cache and the original was
		 * altered, the cached location is still returned if the current process
		 * already uses the file (has called cacheFile() with the same file name
		 * before).
		 * @par
		 * This only matters in multi-threaded programs where a file is opened for
		 * reading multiple times. If you need to ensure that each thread uses the
		 * latest file and caches that, use multiple caches.
		 *
		 * @param  toCache  the file to cache
		 *
		 * @return  the cached path if sucessful, the unaltered original path otherwise
		 *
		 */
		fs::path      cacheFile( const fs::path& );
		std::string   cacheFile( const std::string& toCache );

		/**
		 * Copy a file back from the cache.
		 *
		 * @par
		 * This requires the file to be owned by the current current cache instance.
		 * The file remains owned by this cache instance until releaseFile() is called on it.
		 *
		 * @see  unCacheFile()
		 *
		 * @param  fromCache  the file to copy back
		 * @param  overwrite  whether to overwrite the destination file if it already exits
		 * @param  ifNewer    whether to only overwrite if the cached file is newer
		 *
		 * @return  the cached path if sucessful, the unaltered original path otherwise
		 *
		 */
		fs::path      uncacheFile( const fs::path& fromCache, bool overwrite = true, bool ifNewer = true );
		std::string   uncacheFile( const std::string& fromCache, bool overwrite = true, bool ifNewer = true );

		/**
		 * Construct a cache path for writing.
		 *
		 * @par
		 * This doesn't copy any data, but just constructs a new file name for write
		 * caching. This turns a cache into a write cache.
		 * @par
		 * The file can be copied back to the source location using unCacheFile().
		 * @par
		 * The file is owned by this cache instance until releaseFile() is called on it.
		 *
		 * @see  unCacheFile()
		 *
		 * @param  toCache  the file to cache
		 *
		 * @return  the cached path if sucessful, the unaltered original path otherwise
		 *
		 */
		fs::path      cacheFileForWriting( const fs::path& toCache );
		std::string   cacheFileForWriting( const std::string& toCache );

		/**
		 * Releases a file from the cache for the current process.
		 *
		 * @par
		 * After release, the file might get deleted when the cache is tidied up next.
		 *
		 * @param  path  the file to release
		 *
		 */
		void          releaseFile( const fs::path& path );
		void          releaseFile( const std::string& path );

		/**
		 * Toggle verbosity messages for this cache instance on/off.
		 *
		 * @param  logging  Swich messages on (true) or off (false)
		 *
		 */
		void          babble( bool logging );

		void          relocate( const fs::path& where );
		void          relocate( const std::string& where );
		/**
		 * Set the cache's new size in Megabytes for this cache's location.
		 *
		 * @par
		 * Note that this will override the size for all cache instances sharing this
		 * cache's location.
		 * @par
		 *
		 * @param size  The new cache size in Megabytes (multiples
		 *              of 1,000,000), not Mebibytes (multiples of 1,048,576).
		 *
		 */
		void          resize( uintmax_t size );

		/**
		 * Query the cache's size.
		 *
		 * @return  The cache's in Megabytes (multiples of 1,000,000), not Mebibytes
		 *          (multiples of 1,048,576).
		 *
		 */
		inline uintmax_t size() const {
            return cacheSize_[ cacheLocation_ ];
        }

		/**
		 * Convert the cache's location to a boost::filesystem::path.
		 *
		 * @return  the cache's location
		 *
		 */
        inline        operator fs::path() const {
            return cacheLocation_;
        }
		/**
		 * Convert the cache's location to a std::string.
		 *
		 * @return  the cache's location
		 *
		 */
        inline        operator std::string() const {
            return cacheLocation_.string();
        }

		/**
		 * Query the cache's location
		 *
		 * @return  the cache's location
		 *
		 */
		inline std::string location() const {
            return cacheLocation_.string();
        }

	private:


        typedef boost::unique_lock< boost::shared_mutex > WriteGuard;
        typedef boost::shared_lock< boost::shared_mutex > ReadGuard;

		typedef std::map< ipd::OS_process_id_t, std::set< unsigned > > ProcessCounterInventory;


		typedef std::map< unsigned, std::set< fs::path > > ReferenceInventory;
		typedef std::map< ipd::OS_process_id_t, ReferenceInventory > ProcessInventory;
		typedef std::map< fs::path, ProcessInventory > Inventory;


		/**
		 * The class keeps a map of cache locations
		 * Each entry points to a map of process IDs
		 * Each process ID points to a class instance reference number
		 * Each class instance reference number points to a set of paths
		 * These paths are the files cached by the resp. instance of the class
		 * Once cached, they are guaranteed to not be altered by the cache as
		 * long as the process is alive and/or hasn't released the files.
		 * This covers the case where e.g.:
		 * - A render (A) uses a shadow map that got cached.
		 * - Another render (B) is launched on the machine later, demanding the
		 *   same map.
		 * - The map was updated on the server in the meantime.
		 * - The filecache lib will give render A the cached loaction and
		 *   render B the original one
		 * - If render B requests the file again via the filecache lib after
		 *   the process of render A has terminated, the file will get updated
		 *   in the cache.
		 */
        typedef std::map< fs::path, uintmax_t > PathSizeMap;

		static ProcessCounterInventory instanceCounter_;
		static Inventory cacheInventory_;
		static PathSizeMap cacheSize_;

		bool cache_, log_;
		fs::path cacheLocation_, cwd_;

		std::string processName_;

		unsigned reference_;

        mutable boost::shared_mutex mutex_;
        mutable boost::shared_mutex messageMutex_;

		void init_cache( const fs::path&, bool );
		void relocate_cache( const fs::path& where );
		// TODO: move all this stuff to a traits class and convert filecache into a template
		fs::path cached_file_path( const fs::path& ) const;
		fs::path original_file_path( const fs::path& ) const;
		fs::path cached_file_name( const fs::path& ) const;
		bool is_remote( const fs::path& ) const;
		bool is_different( const fs::path&, const fs::path& ) const;
		bool is_used( const fs::path& ) const;
		bool is_used_by_this_cache( const fs::path& ) const;
		void register_file( const fs::path& );
		fs::path copy_to_cache( const fs::path&, const fs::path& );
		void copy_overwrite_file( const fs::path&, const fs::path& ) const;
		void erase_this_reference();
		void tidy_up_inventory();
		bool tidy_up_cache( const fs::path& path );
		fs::path read_link( const fs::path& link ) const;

		time_t last_access_time( const fs::path& ) const;

		bool create_full_path( const fs::path& ) const;

		inline void message( const std::string& message ) const;
		std::string get_process_name() const;
};


} // namespace filecache
