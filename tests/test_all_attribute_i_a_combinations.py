import inspect
from pyo import *

categories = OBJECTS_TREE["PyoObjectBase"]["PyoObject"]
objects = []
[objects.extend(OBJECTS_TREE["PyoObjectBase"]["PyoObject"][cat]) for cat in categories]
objects.extend(OBJECTS_TREE["PyoObjectBase"]["PyoPVObject"])


# no default value
objects.remove("Sig")
objects.remove("SigTo")
objects.remove("Compare")

obj_dict = {}

def filter_doc(doc):
    p1 = doc.find(":Args:")
    if p1 != -1:
        p1 = doc.find("\n", p1) + 1
    else:
        p1 = 0
    doc = doc[p1:]
    count = 0
    for line in doc.splitlines():
        stripped = line.strip()
        if stripped.startswith(":") or stripped.startswith("..") or stripped.startswith(">>>"):
            break
        count += len(line)
    doc = doc[:count]
    return doc

for obj in objects:
    doc = filter_doc(eval(obj).__doc__)
    lines = doc.splitlines()
    args = str(inspect.signature(eval(obj)))
    obj_dict[obj] = {}
    obj_dict[obj]["attributes"] = {}
    obj_dict[obj]["mandatory"] = {}

    mandatory_end = 0

    # search for audio attributes
    for i, line in enumerate(lines):
        if "float or PyoObject" in line:
            line = line.strip()
            att = line[0 : line.find(":")]  # attribute name
            pos1 = args.find(att+"=") + len(att+"=")
            pos2 = args.find(",", pos1)
            obj_dict[obj]["attributes"][att] = eval(args[pos1 : pos2])  # attribute default value
        if "optional" in line and mandatory_end == 0:
            mandatory_end = i

    # search for mandatory attributes
    for i in range(mandatory_end):
        if lines[i].startswith("        ") and lines[i][8] != " ":
            stripped = lines[i].strip()
            att, typ = stripped.split(": ")
            obj_dict[obj]["mandatory"][att] = typ.strip()

    # Remove object without audio attribute from the list
    if not obj_dict[obj]["attributes"] or not obj_dict[obj]["mandatory"]:
        del obj_dict[obj]

#print(obj_dict)
#print(len(obj_dict))

# For each object in obj_dict, launch as many tests as there are combinations of audio/floats attributes.

# mandatory attribute dummys.
dummys = {
    "PyoObject": "Sig(0)",
    "list of PyoObject": "[Sig(0), Sig(0)]",
    "Python callable (function or method)": "lambda: None",
    "list of floats or list of lists of floats": "[0, 0]",
    "string": "SNDS_PATH+'/transparent.aif'",
    "PyoTableObject": "SndTable(SNDS_PATH+'/transparent.aif')",
    "PyoPVObject": "PVAnal(Sig(0))"
}

server = Server(audio="manual")

for obj in obj_dict.keys():
    server.boot()
    d = obj_dict[obj]
    attrs = list(d["attributes"].keys())
    length = len(attrs)

    for i in range(int(pow(2, length))):
        s = "a{}_ = {}(".format(i, obj)
        # handle special case of Vectral
        if obj == "Vectral":
            s = s + "input=FFT(Sig(0))['real'], "
        else:
            for mandat in d["mandatory"]:
                s = s + "{}={}, ".format(mandat, dummys[d["mandatory"][mandat]])
        for j in range(length):
            if i & 1 << j:
                # audio version of the attribute
                s = s + "{}=Sig({}), ".format(attrs[j], d["attributes"][attrs[j]])
            else:
                # float version of the attribute
                s = s + "{}={}, ".format(attrs[j], d["attributes"][attrs[j]])
        s = s[:-2] + ")"
        print(s)
        exec(s)

    server.start()
    print("    process... ", end="")
    for i in range(10):
        server.process()
    print("done!")
    server.stop()
    server.shutdown()

print("=== All {} objects done! ===".format(len(obj_dict)))
