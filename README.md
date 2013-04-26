This is a copy of upstream's [jnotify](http://jnotify.sourceforge.net).

CVS sucks, so I'm taking a snapshot (version 0.94) and building from there.
Since there's been like one patch in the past three years, I'm not really
worried about diverging much from upstream.

To build:

    ant build build_libraries

Your binaries are now in <code>build/</code>.  To clean, either <code>ant clean</code>
or just <code>rm -rf build</code> and run the above command again.
