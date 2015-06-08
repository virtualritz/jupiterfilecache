# Introduction #

I wrote this lib in my spare time, at home, while working at Double Negative (DNeg) in London, on "Hellboy II". While the lib was never used on that show, the team working on "The Dark Knight", at the same time, were interested in it.
So I cleaned up the source and added a few bits & bobs. The cleanup was mainly making sure another developer could pick up my code -- i.e. adding comments and cleaning up naming.

Unfortunately, I never made a copy of this changed lib, when I left DNeg. After that, DNeg's Philippe Leprince pushed me to release this as FOSS and tried to recover the last version from my backed up home folder at DNeg, but no luck. :)

So basically, here's the list of todos.

Don't let this list scare you btw. This lib is ready to be used in production, as-is.

# To Do #

  * Add proper exceptions & exception handling (currently methods just return booleans to indicate success/failure).
  * Once [boost::filesystem v3](http://mysite.verizon.net/beman/v3/v3.html) is out, replace some of the custom methods with the new stuff in there, e.g. filecache::read\_link() can be replaced with boost::filesystem::read\_symlink().
  * Find out why some headers are missing on some distros, e.g. Fedora Core was missing linux/nfs\_fs.h which defines the NFS super magic id needed to identify NFS mounts as such.
  * Add some more comments. :)