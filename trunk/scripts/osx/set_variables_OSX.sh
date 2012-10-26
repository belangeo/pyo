#! /bin/sh

# Add /usr/local/lib in .bash_profile if not already done    
searchString="/usr/local/lib"

if [ -f ~/.bash_profile ]; then
    if `cat ~/.bash_profile | grep "${searchString}" 1>/dev/null 2>&1`; then
        echo "path already in PATH variable";
    else
        echo "adding /usr/local/lib to PATH variable in .bash_profile..."
        echo "export PATH=/usr/local/lib:/usr/local/bin:\$PATH" >> ~/.bash_profile;
    fi
else
    echo "creating .bash_profile and adding /usr/local/lib to PATH variable..."
	echo "export PATH=/usr/local/lib:/usr/local/bin:\$PATH" > ~/.bash_profile;
fi	

# Add VERSIONER_PYTHON_PREFER_32_BIT in .bash_profile if not already done    
searchString="VERSIONER_PYTHON_PREFER_32_BIT"

if `cat ~/.bash_profile | grep "${searchString}" 1>/dev/null 2>&1`; then
    echo "Variable VERSIONER_PYTHON_PREFER_32_BIT already set.";
else
    echo "Adding VERSIONER_PYTHON_PREFER_32_BIT=yes in .bash_profile...";
    echo "export VERSIONER_PYTHON_PREFER_32_BIT=yes" >> ~/.bash_profile;
fi
