# Skeleton tracking utility

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

## License

This software is Apache licensed like the original OpenNi code. See the
[LICENSE](LICENSE) for more information.
