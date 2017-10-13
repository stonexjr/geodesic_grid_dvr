# GeodesicGridDVR Overview
GeodesicGridDVR is an open source, GPU based high-performance direct volume rendering of spherical geodesic grid
GeodesicGridDVR core rendering code is based on OpenGL GLSL, and runs on anything from laptop, to workstations.
The rendering algorithm is based on the publication [Interactive Ray Casting of Geodesic Grids](http://vis.cs.ucdavis.edu/~jrxie/img/pub/2013.Interactive.Ray.Casting.of.Geodesic.Grids.pdf)

# Support and Contact
GeodesicGridDVR is under active development. It uses cmake for cross-platform build. We do our best to guarantee stable release versions a certain number of bugs, as-yet-missing features, inconsistencies, or any other issues are still possible.
Should you find any such issues please report them via the [issue tracker](https://github.com/stonexjr/geodesic_grid_dvr/issues)

# Building GeodesicGridDVR from source
GeodesicGridDVR is designed to support Linux, Mac OS X, and Windows. In addition, before you build GeodesicGridDVR from source you need the following prerequisites:

* You need any form of C++11 compiler and build tool [CMake](https://cmake.org/download/)
* You need [glew](http://glew.sourceforge.net/)
* You need [freeglut](http://freeglut.sourceforge.net/) to provide a simple cross-platform window system.
This demo is based on lightweight freeglut, you're encouraged to use other full fledged library such as [Qt](https://www1.qt.io/download/)
* You need [netcdf-c](https://www.unidata.ucar.edu/downloads/netcdf/index.jsp) library to load [netcdf](http://www.unidata.ucar.edu/software/netcdf/docs/netcdf_introduction.html) data file.
You may also find NASA's [Panoply data viewer](https://www.giss.nasa.gov/tools/panoply/) helpful to inspect the netcdf file content.
* Finally, you need [Davinci](https://github.com/stonexjr/davinci), a lightweight object-oriented 3D library that makes programming OpenGL easier.
Note that Davinci also depends on glew and freeglut.


