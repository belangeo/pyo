#! /bin/sh

VERSION=`sw_vers -productVersion | sed -e 's/\.//g'`

if [ $VERSION -ge '1070' ]; then
    echo "Install pyo on OSX 10.7";
else
    echo "Install pyo on OSX 10.6";
fi

# Removed older versions in the python site-packages builtin directories
if cd /Library/Python/2.6/site-packages/; then
    if [ -f pyo.py ]; then
        sudo rm pyo.py;
    fi
    if [ -f pyo64.py ]; then
        sudo rm pyo64.py;
    fi
    if [ -f pyo.pyc ]; then
        sudo rm pyo.pyc;
    fi
    if [ -f pyo64.pyc ]; then
        sudo rm pyo64.pyc;
    fi
    if [ -f _pyo.so ]; then
        sudo rm _pyo.so;
    fi
    if [ -f _pyo64.so ]; then
        sudo rm _pyo64.so;
    fi
    if [ -d pyolib ]; then
        sudo rm -rf pyolib/;
    fi
    ls -1 pyo*-info > /dev/null 2>&1
    if [ "$?" = "0" ]; then
        sudo rm pyo*-info;
    fi
fi

if cd /Library/Python/2.7/site-packages/; then
    if [ -f pyo.py ]; then
        sudo rm pyo.py;
    fi
    if [ -f pyo64.py ]; then
        sudo rm pyo64.py;
    fi
    if [ -f pyo.pyc ]; then
        sudo rm pyo.pyc;
    fi
    if [ -f pyo64.pyc ]; then
        sudo rm pyo64.pyc;
    fi
    if [ -f _pyo.so ]; then
        sudo rm _pyo.so;
    fi
    if [ -f _pyo64.so ]; then
        sudo rm _pyo64.so;
    fi
    if [ -d pyolib ]; then
        sudo rm -rf pyolib/;
    fi
    ls -1 pyo*-info > /dev/null 2>&1
    if [ "$?" = "0" ]; then
        sudo rm pyo*-info;
    fi
fi

# Removed pyo in the python.org site-packages install directories
if cd /Library/Frameworks/Python.framework/Versions/2.5/lib/python2.5/site-packages/; then
    if [ -f pyo.py ]; then
        sudo rm pyo.py;
    fi
    if [ -f pyo64.py ]; then
        sudo rm pyo64.py;
    fi
    if [ -f pyo.pyc ]; then
        sudo rm pyo.pyc;
    fi
    if [ -f pyo64.pyc ]; then
        sudo rm pyo64.pyc;
    fi
    if [ -f _pyo.so ]; then
        sudo rm _pyo.so;
    fi
    if [ -f _pyo64.so ]; then
        sudo rm _pyo64.so;
    fi
    if [ -d pyolib ]; then
        sudo rm -rf pyolib/;
    fi
    ls -1 pyo*-info > /dev/null 2>&1
    if [ "$?" = "0" ]; then
        sudo rm pyo*-info;
    fi
fi

if cd /Library/Frameworks/Python.framework/Versions/2.6/lib/python2.6/site-packages/; then
    if [ -f pyo.py ]; then
        sudo rm pyo.py;
    fi
    if [ -f pyo64.py ]; then
        sudo rm pyo64.py;
    fi
    if [ -f pyo.pyc ]; then
        sudo rm pyo.pyc;
    fi
    if [ -f pyo64.pyc ]; then
        sudo rm pyo64.pyc;
    fi
    if [ -f _pyo.so ]; then
        sudo rm _pyo.so;
    fi
    if [ -f _pyo64.so ]; then
        sudo rm _pyo64.so;
    fi
    if [ -d pyolib ]; then
        sudo rm -rf pyolib/;
    fi
    ls -1 pyo*-info > /dev/null 2>&1
    if [ "$?" = "0" ]; then
        sudo rm pyo*-info;
    fi
fi

if cd /Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/; then
    if [ -f pyo.py ]; then
        sudo rm pyo.py;
    fi    
    if [ -f pyo64.py ]; then
        sudo rm pyo64.py;
    fi
    if [ -f pyo.pyc ]; then
        sudo rm pyo.pyc;
    fi 
    if [ -f pyo64.pyc ]; then
        sudo rm pyo64.pyc;
    fi
    if [ -f _pyo.so ]; then
        sudo rm _pyo.so;
    fi
    if [ -f _pyo64.so ]; then
        sudo rm _pyo64.so;
    fi
    if [ -d pyolib ]; then
        sudo rm -rf pyolib/;
    fi    
    ls -1 pyo*-info > /dev/null 2>&1
    if [ "$?" = "0" ]; then
        sudo rm pyo*-info;
    fi    
fi

# Install pyo in the python site-packages builtin directories
if [ $VERSION -ge '1070' ]; then
    if cd /Library/Python/2.7/site-packages/; then
        sudo cp -r /tmp/python27/* .
    else
        sudo mkdir -p /Library/Python/2.7/site-packages/
        cd /Library/Python/2.7/site-packages/
        sudo cp -r /tmp/python27/* .
    fi
else
    if cd /Library/Python/2.6/site-packages/; then
        sudo cp -r /tmp/python26/* .
    else
        sudo mkdir -p /Library/Python/2.6/site-packages/
        cd /Library/Python/2.6/site-packages/
        sudo cp -r /tmp/python26/* .
    fi
fi

# Install pyo in the python.org site-packages directories
if cd /Library/Frameworks/Python.framework/Versions/2.6/lib/python2.6/site-packages/; then
    if [ -f pyo.py ]; then
        sudo rm pyo.py;
    fi    
    if [ -f pyo64.py ]; then
        sudo rm pyo64.py;
    fi    
    if [ -f pyo.pyc ]; then
        sudo rm pyo.pyc;
    fi    
    if [ -f pyo64.pyc ]; then
        sudo rm pyo64.pyc;
    fi    
    if [ -f _pyo.so ]; then
        sudo rm _pyo.so;
    fi    
    if [ -f _pyo64.so ]; then
        sudo rm _pyo64.so;
    fi    
    if [ -d pyolib ]; then
        sudo rm -rf pyolib/;
    fi    
    if [ -f pyo*-info ]; then
        sudo rm pyo*-info;
    fi    
    sudo mv /tmp/python26/* .
else
    sudo mkdir -p /Library/Frameworks/Python.framework/Versions/2.6/lib/python2.6/site-packages/
    cd /Library/Frameworks/Python.framework/Versions/2.6/lib/python2.6/site-packages/
    sudo mv /tmp/python26/* .
fi

if cd /Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/; then
    if [ -f pyo.py ]; then
        sudo rm pyo.py;
    fi    
    if [ -f pyo64.py ]; then
        sudo rm pyo64.py;
    fi    
    if [ -f pyo.pyc ]; then
        sudo rm pyo.pyc;
    fi    
    if [ -f pyo64.pyc ]; then
        sudo rm pyo64.pyc;
    fi    
    if [ -f _pyo.so ]; then
        sudo rm _pyo.so;
    fi    
    if [ -f _pyo64.so ]; then
        sudo rm _pyo64.so;
    fi    
    if [ -d pyolib ]; then
        sudo rm -rf pyolib/;
    fi    
    if [ -f pyo*-info ]; then
        sudo rm pyo*-info;
    fi    
    sudo mv /tmp/python27/* .
else
    sudo mkdir -p /Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/
    cd /Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/
    sudo mv /tmp/python27/* .
fi

# Add /usr/local/lib in .bash_profile if not already done
searchString="/usr/local/lib"

if [ -f ~/.bash_profile ]; then
    if `cat ~/.bash_profile | grep "${searchString}" 1>/dev/null 2>&1`; then
        echo "path already in PATH variable";
    else
        echo "adding path to .bash_profile..."
        echo "export PATH=/usr/local/lib:/usr/local/bin:\$PATH" >> ~/.bash_profile;
    fi
else
    echo "creating .bash_profile and adding path to it..."
    echo "export PATH=/usr/local/lib:/usr/local/bin:\$PATH" > ~/.bash_profile;
fi
