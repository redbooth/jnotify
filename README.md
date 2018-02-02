JNotify
=======

JNotify is a Java library that allow applications to listen to filesystem
events, such as:

 - File created
 - File modified
 - File renamed
 - File deleted

This repository is a fork of upstream's [jnotify](http://jnotify.sourceforge.net).

The last upstream release was 0.94, back in April 2012, and the original source repository
was on CVS, which is frankly awful to work with, hence the creation of this fork.

## Noteworthy changes

This fork is *not* a drop-in replacement.

To make the best out of each platform-specific API, the simplified platform-agnostic
abstraction layer was dropped to ease maintenance.

The JAR no longer automatically loads the native libraries. It is up to the calling
application to determine when and how to do that.

The name of the native libraries was changed to reflect the incompatibility.

## Build JAR

The following command will build the main jar (no sources or javadocs) and place it in `build/libs`

```
gradle jar
```

## Build native code

You need the `qmake` build tool from [Qt 5](https://www.qt.io).

```
qmake && make && make clean
```

NB: this does *not* cross-compile, so you need to run this command on every supported
platform you want to build native code for (Linux, macOS, Windows).

NB: on Windows you may need to use `nmake` instead, if your Qt installation is configured to
use MSVC instead of MinGW.

Native libraries are placed in the `native` folder.

## Deploy to Nexus/Maven

To deploy to a private Nexus repository, simply create a `gradle.properties` file with the
following variables:

```
nexusRepo=http://repo.domain.tld
nexusUser=user
nexusPass=password
```

Then run the following command to deploy all jars (main, sources, javadoc) to that repo

```
gradle uploadArchives
```

NB: Native code is *not* deployed to the repo and needs to be bundled separately in your application.

