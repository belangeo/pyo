if cd /usr/local/include; 
then 
    if cd /usr/local/include/lo;
    then
        sudo scp 132.204.178.49:/Library/WebServer/Documents/pyo_dep_headers/lo/* .
        cd ..;
    else
        sudo mkdir lo
        cd lo 
        sudo scp 132.204.178.49:/Library/WebServer/Documents/pyo_dep_headers/lo/* .;
    fi    
    
else 
    sudo mkdir /usr/local/include; 
        
fi


#cd lo
#sudo cp /Users/olipet/svn/pyo_dep_headers/lo/* .
#cd ..
#sudo cp /Users/olipet/svn/pyo_dep_headers/*.h .
