from pyo import *

tree = OBJECTS_TREE
l = []
for k1 in tree.keys():
    if type(tree[k1]) == type({}):
        for k2 in tree[k1].keys():
            for val in tree[k1][k2]:
                l.append(val)
    else:
        for val in tree[k1]:
            l.append(val)
l.append("PyoObject")
l.append("PyoTableObject")
l.append("PyoMatrixObject")
l.append("Server")

with open("pyo_keywords.txt", "w") as f:
    f.write(" ".join(l))

