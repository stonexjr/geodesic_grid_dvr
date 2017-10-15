# GeodesicGridDVR Overview
GeodesicGridDVR is an open source, GPU based high-performance direct volume rendering of [spherical geodesic grid](https://en.wikipedia.org/wiki/Geodesic_grid).
The GeodesicGridDVR core rendering code is based on OpenGL GLSL, and runs on anything from laptop, to workstations.
The rendering algorithm is based on the publication [Interactive Ray Casting of Geodesic Grids](http://vis.cs.ucdavis.edu/~jrxie/img/pub/2013.Interactive.Ray.Casting.of.Geodesic.Grids.pdf).
<img src="https://github.com/stonexjr/geodesic_grid_dvr/blob/master/resource/gcrm_28km_dvr_vorticiy_overview.jpg" width="256">
<img src="https://github.com/stonexjr/geodesic_grid_dvr/blob/master/resource/gcrm_28km_dvr_vorticiy_zoomin.jpg?raw=true" width="256">
<img src="https://github.com/stonexjr/geodesic_grid_dvr/blob/master/resource/gcrm_220km_dvr.PNG?raw=true" width="256">
<img src="https://github.com/stonexjr/geodesic_grid_dvr/blob/master/resource/gcrm_220km_hexagon_mesh.PNG?raw=true" width="200">
<img src="https://github.com/stonexjr/geodesic_grid_dvr/blob/master/resource/gcrm_220km_triangle_mesh.PNG?raw=true" width="200">
<img src="https://github.com/stonexjr/geodesic_grid_dvr/blob/master/resource/gcrm_220km_hexagon_mesh_solid.PNG?raw=true" width="200">
<img src="https://github.com/stonexjr/geodesic_grid_dvr/blob/master/resource/gcrm_220km_triangle_mesh_solid.PNG?raw=true" width="200">


# Support and Contact
GeodesicGridDVR is under active development. It uses cmake for cross-platform build. We do our best to guarantee stable release versions. However, a certain number of bugs, as-yet-missing features, inconsistencies, or any other issues are still possible.
Should you find any such issues please report them via the [issue tracker](https://github.com/stonexjr/geodesic_grid_dvr/issues)

# Building GeodesicGridDVR from source
GeodesicGridDVR is designed to support Linux, Mac OS X, and Windows. In addition, before you build GeodesicGridDVR from source you need the following prerequisites:

## Prerequisites
* You need any form of C++11 compiler and build tool [CMake](https://cmake.org/download/)
* You need [glew](http://glew.sourceforge.net/) a cross-platform library to expose OpenGL core and extension functionality.
* You need [freeglut](http://freeglut.sourceforge.net/) to provide a simple cross-platform window system.
This demo is based on lightweight freeglut, you're encouraged to use other full fledged library such as [Qt](https://www1.qt.io/download/)
* You need [netcdf-c](https://www.unidata.ucar.edu/downloads/netcdf/index.jsp) library to load [netcdf](http://www.unidata.ucar.edu/software/netcdf/docs/netcdf_introduction.html) data file.
You may also find NASA's [Panoply data viewer](https://www.giss.nasa.gov/tools/panoply/) helpful to inspect the netcdf file content.
* Finally, you need [Davinci](https://github.com/stonexjr/davinci), a lightweight object-oriented 3D library that makes programming OpenGL easier.
Note that Davinci also depends on glew and freeglut.

Depending on your Linux distribution you can install these dependencies
using `yum` or `apt-get`. Some of these packages might already be
installed or might have slightly different names.

Type the following to install the dependencies using `yum`:

    sudo yum install cmake.x86_64

Type the following to install the dependencies using `apt-get`:

    sudo apt-get install cmake-curses-gui

Under Mac OS X these dependencies can be installed using
[MacPorts](http://www.macports.org/):

    sudo port install cmake

## Compiling GeodesicGridDVR
Building from the source code through CMake is easy:

### 1. Create a build directory
```
        mkdir geodesic_grid_dvr/build
        cd geodesic_grid_dvr/build
```
I do recommend having separate build directory from the source directory, so that you won't accidentally checked in the temporary files generated during the build process.   
From this step beyond you have two options to run cmake, choose one of the following two.

### 2.1 Build through CMake CLI
```
        cmake ../path/to/geodesic_grid_dvr_source
```
In case cmake cannot find the dependencies, you can alway tell cmake where they can be found by defining the root of the library installation as follows
```
        cmake -DGLEW_ROOT_DIR="path/to/glew_root" 
              -DFREEGLUT_ROOT_DIR="path/to/freeglut_root" 
              -DNETCDF_ROOT_DIR="path/to/netcdf_root"
              -DDAVINCI_ROOT_DIR="path/to/davinci_root"

              ../path/to/geodesic_grid_dvr_source
```
CMake would try it best to guess what is the most appropriate build system generator you need based on your machine OS, for example, Unix Makefile for linux OS, Visual Studio for Windows. If you don't like the default build system generator, you can overide it by passing additional cmake parameter `-G"generator name"`
For example, on Windows with Visual Studio 2013 x64:
```
       cmake -G"Visual Studio 12 2013 Win64"
```
on Linux:
```
       cmake -G"Unix Makefiles"
```
Type `cmake --help` for more options.

### 2.2 Build through CMake GUI  
Besides CLI tool, CMake also provides an user friendly GUI to configure your build.
The following screen shot is an example of how to run and configure the cmake through its GUI on Windows machine.
![CMakeGUI](/resource/cmake-gui-win.PNG?raw=true "Screen shot of cmake gui")
In case CMake cannot find the aforementioned dependencies, you can manually specify the root of the installation of each library on the UI just like you can define the cmake variable through its CLI in step 2.1.
Once the pathes are correctly set, hit `Configure`. If this is your first time to configure the project, you will be prompted to select appropriate `generator` for this project.
Click `Generate` to create makefile or Visual Studio sln file based on your selection in the last step.


### 3 Launch build system generator
Lastly, navigate to the build directory and build the project using
```
    make
```
or launch Visual Studio.

# Sanity check

* Data dependency  
The application requires as input a sample test data (stored in `data` folder) and a colormap (transfer function) file (stored in `config` folder).
During launch, it'll look for them in the `data` folder and `config` folder respectively.
The project CMake file is configured to automatically copy those two folders to location where the exectutable is generated.
In case the automation step fails, you need to manually do so.

* Dynamic linkage library dependency
Make sure the dll or so lib files from glew, freeglut, and netcdf can be found by your application.
or else export their path to LD_LIRARY_PATH on linux machine
```
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:path/to/*.so"
```
or add them to the environment variable `Path` for Windows machine

# Functions and Controls
This application provides direct volume rendering of scientific simulation data based on spherical geodesic grid. It also visualizes the structure either as hexagon or its dual triangular mesh.  
Here is a list of keyboard mapping
```
key 'v': toggle between volume rendering and mesh visualization  
key 'h': toggle between hexagon and dual-triangular mesh visualization
key 'f': toggle between solid and wire frame mesh
key 'l': toggle between enabling and disabling lighting for both volume render and mesh visualization
key '+': increase raycasting stepsize (higher frame rate)
key '-': decrease raycasting stepsize (better quality)
key 'w': move camera forward
key 's': move camera backward
key 'a': move camera to the left
key 'd': move camera to the right
key ' ': move camera upward
key 'z': move camera downward
mouse dragging: rotate object
mouse wheel: scale object
```
# Acknowledgments
This work was made possible by the support of the ViDi lab at University of California Davis.  
Data courtesy of Karen Schuchardt at Pacific Northwest NationalLaboratory.

# Citations and Reuse
Diagrams and text are licensed under Creative Commons Attribution [CC-BY 2.0](https://creativecommons.org/licenses/by/2.0/), unless noted otherwise, with the [source available on GitHub](https://github.com/stonexjr/geodesic_grid_dvr).

For attribution in academic contexts, please cite this work as
```
J. Xie, H. Yu, and K.-L. Ma. Interactive ray casting of geodesic grids.
In Proceedings of the 15th Eurographics Conference on Visualization,
EuroVis ’13, pages 481–490, 2013
```
BibTeX citation
```
@inproceedings{xie2013interactive,
  title={Interactive ray casting of geodesic grids},
  author={Xie, Jinrong and Yu, Hongfeng and Ma, Kwan-Liu},
  booktitle={Computer Graphics Forum},
  volume={32},
  number={3pt4},
  pages={481--490},
  year={2013},
  organization={Wiley Online Library}
}
```
# License
MIT

