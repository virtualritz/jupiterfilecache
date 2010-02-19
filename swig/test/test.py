#!/usr/bin/python

import filecache
import os
import stat, time


def test( condition, name ):
	if condition:
		print name + " suceeded."
	else:
		print name + " failed."



cache = filecache.filecache( '' )
cache.resize( 1 ) # 1 MB cache

# Read cache test 1
print "Cache resides at '%s'" % cache.location()
file = "/u/mnm/escher1.jpg"
cachedFile1 = cache.cacheFile( file )
test( file != cachedFile1, "Read cache test 1" )
# Do not release file

# Read cache test 2
cachedFile2 = cache.cacheFile( file ) # Try to cache again
test( file != cachedFile2, "Read cache test 2" )
cache.releaseFile( cachedFile1 ) # Unregister the file in the cache

# Read cache test 3
cachedFile = cache.cacheFile( file )
# 'Modify' file
st = os.stat( file )
os.utime( file, ( st[ stat.ST_MTIME ] + 1, st[ stat.ST_MTIME ] + 1 ) )
# Try to cache again
cachedFile = cache.cacheFile( file )
test( file != cachedFile, "Read cache test 3" )
cache.releaseFile( cachedFile ) # Unregister the file in the cache

# Write cache test
cachedFile = cache.cacheFileForWriting( file )
test( file != cachedFile, "Write cache test 1" )
uncachedFile = cache.uncacheFile( cachedFile )
test( cachedFile != uncachedFile, "Write cache test 2" )

cache2 = filecache.filecache( '/tmp' )
print cache2 == cache






