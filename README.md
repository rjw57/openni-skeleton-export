# Skeleton tracking utility

[![Build Status](https://travis-ci.org/rjw57/openni-skeleton-export.svg?branch=master)](https://travis-ci.org/rjw57/openni-skeleton-export)

This is a silly little hack built atop
[OpenNi](https://github.com/OpenNI/OpenNI)'s user tracking example which is
modified to dump skeletal information to standard output.

It's probably useful if you want to quickly get skeletal dumping working.

## Building

This software uses the [CMake](http://www.cmake.org/) build system. It requires
that the OpenNi libraries be installed (along with the NITE binaries; find your
own mirror). In addition, OpenGL and GLUT must be available.

```console
$ git clone https://github.com/rjw57/openni-skeleton-export.git
$ cd openni-skeleton-export
$ mkdir build
$ cd build
$ cmake ..
$ make && ./skeletonexport
```

Note that the path to [SamplesConfig.xml](Data/SamplesConfig.xml) is hard-coded
into the program and so it must be run from the build directory. (Sorry, it's a
hack.)

## Sample data

The
[recordings](https://git.csx.cam.ac.uk/x/eng-sigproc/u/rjw57/misc/openi-data.git)
sub-module links to a git repository with some sample recordings made with the
sensor. If you've not checked out with
``--recursive``, you can fetch the recordings via:

```console
$ git submodule init
$ git submodule update
```

Then you can test the program without having a sensor:

```console
$ cd build
$ make && ./skeletonexport ../recordings/Captured-2014-10-31.oni
```

There is a non-GUI version of the skeleton tracker/exported which is build
called, imaginatively, ``skeletonexport_nongui`` which may also be used to
process recordings/capture live data.

## Contributing

Pull-requests are very welcome.

## License

This software is Apache licensed like the original OpenNi code. See the
[LICENSE](LICENSE) for more information.
