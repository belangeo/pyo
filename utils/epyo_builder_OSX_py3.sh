mkdir Resources
cp PyoDoc.py Resources/
cp Tutorial_01_RingMod.py Resources/
cp Tutorial_02_Flanger.py Resources/
cp Tutorial_03_TriTable.py Resources/
cp *.icns Resources/
cp -R ../examples Resources/
cp -R snippets Resources/
cp -R styles Resources/

rm -rf build dist

python3 setup.py py2app

rm -rf build
mv dist E-Pyo_OSX_py3

if cd E-Pyo_OSX_py3;
then
    find . -name .git -depth -exec rm -rf {} \
    find . -name *.pyc -depth -exec rm -f {} \
    find . -name .* -depth -exec rm -f {} \;
else
    echo "Something wrong. E-Pyo_OSX not created"
    exit;
fi

# keep only 64-bit arch
ditto --rsrc --arch x86_64 E-Pyo.app E-Pyo-x86_64.app
rm -rf E-Pyo.app
mv E-Pyo-x86_64.app E-Pyo.app

# py2app does not handle correctly wxpython phoenix dylib imports, add them manually.
cp /Library/Frameworks/Python.framework/Versions/3.5/lib/python3.5/site-packages/wx/*.dylib E-Pyo.app/Contents/Resources/lib/python3.5/lib-dynload/wx/

# Fixed wrong path in Info.plist
cd E-Pyo.app/Contents
awk '{gsub("/Library/Frameworks/Python.framework/Versions/3.5/bin/python3", "@executable_path/../Frameworks/Python.framework/Versions/3.5/Python")}1' Info.plist > Info.plist_tmp && mv Info.plist_tmp Info.plist

cd ../../..

rm -rf Resources
