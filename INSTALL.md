* Install folly and dependencies

For Centos based platforms, you can find pre built package for
folly and dependencies here:

http://springdale.math.ias.edu/data/puias/computational/6/x86_64/

* Generic cmake install

```
cd iterlib
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
make -j
```

* All steps for Ubuntu 16.04:

```
# libgtest-dev oesn't include libgtest.a. Some manual steps needed.
# Consult documentation
sudo apt-get install libgtest-dev
sudo apt-get install libboost-all-dev
sudo apt-get install libflags-dev

Install folly from source https:://github.com/facebook/folly
sudo apt-get install libssl-dev
sudo apt-get install libdouble-conversion-dev

cd iterlib
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
make -j
```
