This is Wellspring, an immediate mode font rendering system in C.

About Wellspring
----------------
Wellspring is inspired by the design of Dear ImGui.
It outputs pixel data that you can upload to a texture and vertex buffers that you can render anytime in your 3D application.
This means that you can integrate it easily using the graphics library of your choice.
Wellspring uses stb_truetype to rasterize and pack fonts quickly.

Dependencies
------------
Wellspring depends on the C runtime, but SDL2 can be optionally depended upon instead if your application prefers it.

Building Wellspring
-------------------
For *nix platforms, use Cmake:

	$ mkdir build/
	$ cd build
	$ cmake ../
	$ make

License
-------
Wellspring is licensed under the zlib license. See LICENSE for details.
