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

## Running

This project includes two utilities: ``glskelview`` and ``logskel``. The former
is an OpenGL based utility to check your sensor is pointing in the right
direction and to show tracker skeletons. It can also be used to play back
recorded sensor data to check that tracking will succeed. The later is used to
actually export the tracked skeleton information, depth data and per-pixel user
labels to a log.

### glskelview

This utility takes no command-line options aside from an optional path to a
``.oni``-format recording (as produced by ``NiViewer``). If no options are
passed, it attempts to stream from the primesense sensor using the
configuration at the hard-coded path of ``../Data/SamplesConfig.xml``. Sorry,
this is a horrible hack but reproduces the behaviour of the original
NiUserTracker sample.

### logskel

This utility takes a number of command line options which may be examined via
``logskel --help``. The utility can take either the path to an XML
configuration file for live-streaming from the sensor or the path to a
``.oni`` recording. It can log depth a user-label information to HDF5 files and
tracked skeleton joint information to a simple plain-text format.

For example, to playback ``recording.oni`` for 10 seconds logging to
``/tmp/skel.h5`` and ``/tmp/skel.txt``:

```console
$ build/logskel --playback recording.oni --duration 10 --log /tmp/skel
```

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

## Contributing

Pull-requests are very welcome.

## License

This software is Apache licensed like the original OpenNi code. See the
[LICENSE](LICENSE) for more information.
