#! /bin/sh

# Make sure older version are not in the System directories
if cd /System/Library/Frameworks/Python.framework/Versions/2.5/lib/python2.5; then
    if [ -f pyo.py ]; then
        echo "removing pyo from /System/Library/Frameworks/Python.framework/Versions/2.5/lib/python2.5"
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

if cd /System/Library/Frameworks/Python.framework/Versions/2.6/lib/python2.6; then
    if [ -f pyo.py ]; then
        echo "removing pyo from /System/Library/Frameworks/Python.framework/Versions/2.5/lib/python2.6"
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

# Removed older versions in the python site-packages builtin directories
if cd /Library/Python/2.5/site-packages/; then
    if [ -f pyo.py ]; then
        echo "removing pyo from /Library/Python/2.5/site-packages/"
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

if cd /Library/Python/2.6/site-packages/; then
    if [ -f pyo.py ]; then
        echo "removing pyo from /Library/Python/2.6/site-packages/"
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
        echo "removing pyo from /Library/Python/2.7/site-packages/"
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

# Removed pyo in the python site-packages python.org install directories
if cd /Library/Frameworks/Python.framework/Versions/2.5/lib/python2.5/site-packages/; then
    if [ -f pyo.py ]; then
        echo "removing pyo from /Library/Frameworks/Python.framework/Versions/2.5/lib/python2.5/site-packages/"
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
        echo "removing pyo from /Library/Frameworks/Python.framework/Versions/2.6/lib/python2.6/site-packages/"
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
        echo "removing pyo from /Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/"
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
