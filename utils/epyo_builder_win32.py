import os, shutil, sys

version = sys.version_info[:2]

os.mkdir("Resources")
shutil.copy("PyoDoc.py", "Resources")
shutil.copy("Tutorial_01_RingMod.py", "Resources")
shutil.copy("Tutorial_02_Flanger.py", "Resources")
shutil.copy("Tutorial_03_TriTable.py", "Resources")
shutil.copy("E-PyoIcon.ico", "Resources")
shutil.copy("E-PyoIconDoc.ico", "Resources")
shutil.copytree("../examples", "Resources/examples")
shutil.copytree("snippets", "Resources/snippets")
shutil.copytree("styles", "Resources/styles")

if version[0] < 3:
    os.system('C:\Python%d%d\Scripts\pyinstaller --clean -F -c --icon=Resources\E-PyoIcon.ico "E-Pyo.py"' % version)
else:
    os.system('C:\\Users\olivier\AppData\Local\Programs\Python\Python%d%d-32\Scripts\pyinstaller --clean -F -c --icon=Resources\E-PyoIcon.ico "E-Pyo.py"' % version)

os.mkdir("E-Pyo_py%d%d" % version)
shutil.copytree("Resources", "E-Pyo_py%d%d/Resources" % version)
shutil.copy("dist/E-Pyo.exe", "E-Pyo_py%d%d" % version)
os.remove("E-Pyo.spec")
shutil.rmtree("build")
shutil.rmtree("dist")
shutil.rmtree("Resources")
for f in os.listdir(os.getcwd()):
    if f.startswith("warn") or f.startswith("logdict"):
        os.remove(f)
