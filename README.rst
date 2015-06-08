Jupiter File Cache
==================

JupiterFileCache (JFC) is a general purpose, multi location, multi process, thread safe* file read & write cache library.

This is a C++ library that allows caching files in remote (network) locations onto the local client to speed up future
accesses to these files.
This reduces network traffic but also, more importantly, disk access on the server. Modern network infrastructures are
rarely the problem, but a server's disks often don't cope well when hundreds of clients read large files, often repeatedly,
by querying small pieces of data, all at the same time.

It comes with a `SWIG <http://www.swig.org/>`_ interface to be easily accessible through scripting languages (e.g Lua,
Python) without the need to use a C++ compiler.

C++ API docs are `here <http://jupiterfilecache.googlecode.com/hg/doc/html/index.html>`_.

Use case example
----------------

As a example use-case we provide an `RSL shadeop <http://en.wikipedia.org/wiki/Shadeop>`_ implementation, modeled
after the automatic network cache in `DNA research's 3Delight <http://www.3delight.com/>`_.
It allows caching files like textures, point clouds or other heavy data, on a render farm client, automatically,
by simply specifying the cache location with an environment variable, or in the RIB (with an option).
In your shaders you can use the cacheFile() shadeop.  It should work with any renderer that has an RSL shadeop API
but was tested & used in production with `Pixar's PhotoRealistic RenderMan <http://renderman.pixar.com/>`_.

So, instead of::

  texture( foo )


you just do::

  texture( cacheFile( foo ) )


And *voilà*, it's cached for you :)

Notes
.....

* you can write-cache files that usually stress a server (because they arrive in bits, over extended periods, many at the same time -- e.g. image buckets/tiles from a 3D render).

Future Development
..................

Contributors are welcome! Here's what we would like to do:

* VEX shadeop for use in Houdini's Mantra or directly, in VOPs 
* Ri filter for caching (delayed) RIB archives

JFC is brought to you by `/*jupiter jazz*/ <http://www.jupiter-jazz.com/>`_.
