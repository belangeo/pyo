if cd /usr/local/include; 
then 
    pwd;
else 
    sudo mkdir /usr/local/include
    cd /usr/local/include;
fi
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/portaudio.h -o "portaudio.h"
sudo curl http://www.iact.umontreal.ca/pyo_dep_headers/portmidi.h -o "portmidi.h"
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
