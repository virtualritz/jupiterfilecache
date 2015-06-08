# The Why #
You my ask why one would do the caching of textures, point clouds, potentially also RIB archives or custom binary files, that a frame depends on, through a shadeop (or Ri filter).
Why not just copy them to the client before the render job launches and adjust paths in the RIB with an Ri filter or a wrapper script?

To understand the problem well, one must look at the actual possibility of a referenced file being _needed_ at render time. It is a flawed assumption, that any file a RIB merely _references_ will also indeed be _used_ when this RIB is being rendered. Understanding this distinction is the key to understanding the difference between brute force caching (copying all dependecies on the server, _before_ rendering starts) and automatic, on demand caching (copying a file when the _running_ render requests it).

Only the renderer 'knows' which files are true dependencies of the image being rendered and which ones are just referenced by the RIB.
Anyone looking at the RIB can just see which files are referenced. The subset of the referenced ones that are actually being used, at render time, one can only find out by loooking at server file access logs, _after_ the frame has finished rendering.

In other words: a RIB may reference 500 textures and two large point clouds, summing up to many gigabytes of data. But when rendered, only a piece of geometry referencing five of these textures and one of the two point clouds may be visible (and thus requested). A few hundred mageabytes of data.

Now imagine this scenario multiplied by the number of render clients on a typical render farm, multiplied by the number of frames (RIBs) and you get an idea how much network traffic and disk access using this lib may actually save you.

This is also the reason why such a cache system has been available in the prorietary 3Delight renderer, since years. And I may say that it has saved the asses of a few sysadmin crews on feature film VFX productions I worked on, where this renderer got used.
3Delight also has another nice feature, _directory textures_, which split mip levels of texture maps into individual files in a subdirectory. When combined with an automatic network cache, this helps reduce network traffic even further. Implementing directory textures is left as an excercise to the reader. ;)

If you use another renderer that supports RSL plugin shadeops (e.g. PRMan), you can now make use of the same approach.

# How it Works #

The shadeop handles caching transparently. The cache lib itself uses the two environment variables, mentioned in the main [Documentation](Documentation.md) page.

Additionally, a RIB may reference any number of file caches (overwriting aforementioned environment variables locally) using two user attributes:
```
Attribute "user" "string filecachelocation"
```
and
```
Attribute "user" "string filecachesize"
```

Why attributes and not options?
Because it makes no difference to people who only use one central cache (just specify the attribute once, even before WorldBegin, if you like) but it gives additional control to people who need more granularity.
You could e.g. have separate caches for static and moving geometry, you may want to write an Ri filter which implements caching for ReadArchive and DelayedReadArchive and these should not mix with textures/shadows/point clouds etc.

If you prefer an option, just switch the RxAttribute() calls to RxOption() in lines 31 & 38 in [rslplugin/src/filecache.cpp](http://jupiterfilecache.googlecode.com/hg/rslplugin/src/filecache.cpp).

# Usage in RSL #

In your shader, just wrap any use of file names in a call to cacheFile(), before feeding them to the actual shadeop:
```
plugin "filecache";

surface 
paintedplastic(
  float Ks = .5;
  float Kd = .5;
  float Ka = 1;
  float roughness = .1; 
  color specularcolor = 1;
  string texturename = ""; )
{
  normal Nn = normalize( N );
  vector V = -normalize( I );

  color Ct = ( "" != texturename ) ? texture( cacheFile( texturename ) ) : 1;

  Oi = Os;
  Ci = Cs * Ct * ( Ka * ambient() + Kd * diffuse( Nn ) ) + 
                 specularcolor * Ks * specular( Nn, V, roughness );

  Ci *= Oi;
}
```
If the lib fails to cache the file, it will just return the original, unaltered path.

This has the nice side effect that you can add this to your shaders before your pipeline is even ready to use it. If the file cache location is missing (no environment variable defined, no attribute in RIB), the lib will just return the original name.

## Write Caching ##
The cache supports write caching as well. Note that this has neither been tried much nor ever been used in production. Use with care and do some testing before deploying.

The shadeop signature is:
```
string cacheFileForWriting( string filename )
```
When the RSP shadeop DSO unloads (at the end of the render), the destructor copies all write-cached files back to their true destinations.

Note that this means that if the render exits ungracefully (crashes), you will have half-finished files sitting in the write-cache on the resp. client (missing on the server).