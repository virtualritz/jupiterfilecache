[API docs (Doxygen)](http://jupiterfilecache.googlecode.com/hg/doc/html/index.html)

# How it works #

JupiterFileCache is a Multi location, multi-process, thread-safe file cache class.
It manages local caches that mirror files in remote locations.

The cache looks for two environment variables, FILECACHE\_LOCATION and FILECACHE\_SIZE which have the obvious meanings. The size is specified in in Megabytes (multiples of 1,000,000), not Mebibytes (multiples of 1,048,576).
This also cleans up the shared cached files reference database for this instance. This ensures search times in the database stay as low as possible, even if the filecache library is kept "alive" in memory for weeks or moths on a machine that always runs at least one process using it.

The class keeps a map of cache locations:

  * Each entry points to a map of process IDs
  * Each process ID points to a class instance reference number
  * Each class instance reference number points to a set of paths

These paths are the files cached by the resp. instance of the class. Once cached, they are guaranteed to not be altered by the cache as long as the process is alive and/or hasn't released the files.

This covers e.g. the case where:
  * A render (A) uses a shadow map that got cached.
  * Another render (B) is launched on the machine later, demanding the same map.
  * The map was updated on the server in the meantime.
  * The filecache lib will give render A the cached location and render B the original one
  * If render B requests the file again via the filecache lib after the process of render A has terminated, the file will get updated in the cache.


# Precautions #
The following precautions are taken to ensure the proper operation of the cache:

  1. When used as a read cache, the filecache lib does not access the original file in any dangerous way, only reading is performed on those files.
  1. The cache is kept synchronized with the files it mirrors: if an original file is newer than the cached one (or has a different size), the cache is updated; unless the file is being used by another cache instance on the same machine.
  1. A file is identified by its full path: files that have the same name in different directories do not collide.
  1. Symbolic links are resolved prior to caching, this ensures that a given file is cached only once even if many links point to it.
  1. The filecache is multi-process- and mult-thread safe. Even if many processe are running on the same machine, sharing a cache, the cache is kept in a consistent state: one filecache instance does not remove or update a file used by another instance in a different process and/or thread!
  1. When the cache is used as a write cache, only one instance can use a particular write location in a cache.
  1. When copying write-cached files back, the cache can check if the file already exists at the destination and/or has a newer date. By default, existing files are only overwritten if they are older than the write- cached file.
  1. If, for any reason, the filecache lib is unable to cache a file, it reverts to use the original, and this is the worst case scenario.

## Thread safety ##
The cache always does obtain a mutex lock before accessing or modifying any data.

Cache locations are referenced per class instance. This ensures that there can be more that one cache instance per location per process. The destructor of any instance of a cache in any thread of the same process at the same cache location will release the files of that cache location for this instance only. Other instances of the cache with this location under the same process will keep shared files locked throughout their lifetime.