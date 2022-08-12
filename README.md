# Description

SisDAQ is a software that handles the data acquisition for SIS3316 digitizer.

# Requirements

The following ones can be installed from package manager, eg. on Ubuntu:

```bash
sudo apt install cmake libboost-all-devel qt5-default libjsoncpp-dev
```


A working ROOT installation is needed. Maybe it can be required to change the 
C++ standard in the CMakeLists.txt to the one with which ROOT was compiled, 
otherwise compatibilty errors could arise.

# Build / Installation

To build the repository:

```bash
git clone https://github.com/skowrons94/SisDAQ.git
cd SisDAQ && mkdir build && cd build/
cmake ..
make && sudo make install
```

The binaries will be installed in ```/opt/SisDAQ/```. You can add the ```export PATH=/opt/SisDAQ/${PATH}``` line to your ```bashrc``` file to start it from 
terminal or create a symlink to you ```bin``` directory.


# Features

For now the project is in the very initial stages. Currently the communication 
with the board is under investigation. Additionally, the board addresses and 
settings have been implemented.

# Bugs / To Do

- ReadoutUnit
- Qt interface
