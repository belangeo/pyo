echo ====== Build and install liblo. ======
wget https://sourceforge.net/projects/liblo/files/liblo/0.31/liblo-0.31.tar.gz
tar -xzf liblo-0.31.tar.gz
cd liblo-0.31
./configure 1>/dev/null
make 1>/dev/null
make install 1>/dev/null
ldconfig
cd ..
