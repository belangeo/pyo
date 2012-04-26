import os, shutil

os.mkdir("Resources")
shutil.copy("PyoDoc.py", "Resources")
shutil.copy("FlatNoteBook.py", "Resources")
shutil.copy("Tutorial_Custom_PyoObject.py", "Resources")
shutil.copy("E-PyoIcon.ico", "Resources")
shutil.copy("E-PyoIconDoc.ico", "Resources")

os.system("svn export ../examples Resources/examples/")
os.system("svn export snippets Resources/snippets/")
os.system("svn export styles Resources/styles/")

os.system("python ..\..\pyinstaller\Configure.py")
os.system('python ..\..\pyinstaller\Makespec.py -F -c --icon=Resources\E-PyoIcon.ico "E-Pyo.py"')
os.system('python ..\..\pyinstaller\Build.py "E-Pyo.spec"')

os.mkdir("E-Pyo")
shutil.copytree("Resources", "E-Pyo/Resources")
shutil.copy("dist/E-Pyo.exe", "E-Pyo")
os.remove("E-Pyo.spec")
shutil.rmtree("build")
shutil.rmtree("dist")
shutil.rmtree("Resources")
for f in os.listdir(os.getcwd()):
    if f.startswith("warn") or f.startswith("logdict"):
        os.remove(f)
