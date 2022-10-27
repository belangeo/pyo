echo ====== Build and install liblo. ======
wget https://sourceforge.net/projects/liblo/files/liblo/0.31/liblo-0.31.tar.gz
tar -xzf liblo-0.31.tar.gz
cd liblo-0.31
./configure 1>/dev/null
make 1>/dev/null
make install 1>/dev/null
ldconfig
cd ..

echo ====== Build and install jack2. ======
wget -O jack2-1.9.21.tar.gz https://github.com/jackaudio/jack2/archive/refs/tags/v1.9.21.tar.gz
tar -xzf jack2-1.9.21.tar.gz
cd jack2-1.9.21
python waf configure LDFLAGS="-lstdc++" 1>/dev/null
python waf build 1>/dev/null
python waf install 1>/dev/null
ldconfig
cd ..