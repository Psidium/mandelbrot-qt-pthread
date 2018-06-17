# mandelbrot-qt-pthread
A toy project using Qt to show a Mandelbrot set given the parameters.

# Build

Have `cmake` and `Qt` installed, fetch the path that contains the Qt cmake helpers, then:
```
export PATH_TO_QT_CMAKE=/usr/local/Cellar/qt/5.10.1/lib/cmake/Qt5/
mkdir build
cd build
cmake -DQt5_DIR="$PATH_TO_QT_CMAKE" ..
make
./parallel-mandelbrot
```
