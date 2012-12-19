if cd /usr/local/include; 
then 
    pwd;
else 
    sudo mkdir /usr/local/include
    cd /usr/local/include;
fi
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/portaudio.h -o "portaudio.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/portmidi.h -o "portmidi.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/porttime.h -o "porttime.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/sndfile.h -o "sndfile.h"


if cd lo;
then
    pwd;
else
    sudo mkdir lo
    cd lo;
fi    
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/lo/lo_endian.h -o "lo_endian.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/lo/lo_errors.h -o "lo_errors.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/lo/lo_lowlevel.h -o "lo_lowlevel.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/lo/lo_macros.h -o "lo_macros.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/lo/lo_osc_types.h -o "lo_osc_types.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/lo/lo_throw.h -o "lo_throw.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/lo/lo_types.h -o "lo_types.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/lo/lo.h -o "lo.h"

if cd /usr/local/lib; 
then 
    pwd;
else 
    sudo mkdir /usr/local/lib
    cd /usr/local/lib;
fi
sudo curl http://www.iact.umontreal.ca/pyo_deps/liblo.0.dylib -o "liblo.0.dylib"
sudo curl http://www.iact.umontreal.ca/pyo_deps/libportaudio.2.dylib -o "libportaudio.2.dylib"
sudo curl http://www.iact.umontreal.ca/pyo_deps/libportmidi.dylib -o "libportmidi.dylib"
sudo curl http://www.iact.umontreal.ca/pyo_deps/libsndfile.1.dylib -o "libsndfile.1.dylib"
sudo rm libsndfile.dylib
sudo ln -s libsndfile.1.dylib libsndfile.dylib
sudo rm liblo.dylib
sudo ln -s liblo.0.dylib liblo.dylib
sudo rm libportaudio.dylib
sudo ln -s libportaudio.2.dylib libportaudio.dylib
