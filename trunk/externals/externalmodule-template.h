/************************************************** 
Declare each external object type as "extern" to be sure 
that the pyomodule will be aware of their existence. 
**************************************************/
extern PyTypeObject GainType;

/********************************************************** 
This macro is called at runtime to include external objects 
in "_pyo" module. Add a line calling "module_add_object"
with your object's name and type for each external object.
**********************************************************/
#define EXTERNAL_OBJECTS \
    module_add_object(m, "Gain_base", &GainType); \

