#! /bin/sh

# Remove build directory if exist
if cd build; then
    echo
    echo "********** Removing older build directory **********"
    cd ..
    sudo rm -rf build
fi

echo
echo "**************************************************"
echo "****************** build library *****************"
echo "**************************************************"
echo

sudo python setup.py install --use-coreaudio --use-double --use-jack
