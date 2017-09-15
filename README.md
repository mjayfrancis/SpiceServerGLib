# SpiceServerGLib

make
export LD_LIBRARY_PATH=${PWD}/.libs
export GI_TYPELIB_PATH=${PWD}

./test_server.py

./proxy.py -p listen_port server:port

