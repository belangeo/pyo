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

python2 setup.py py2app

rm -rf build
mv dist E-Pyo_OSX_py2

if cd E-Pyo_OSX_py2;
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

# Fixed wrong path in Info.plist
cd E-Pyo.app/Contents
awk '{gsub("Library/Frameworks/Python.framework/Versions/2.7/Resources/Python.app/Contents/MacOS/Python", "@executable_path/../Frameworks/Python.framework/Versions/2.7/Python")}1' Info.plist > Info.plist_tmp && mv Info.plist_tmp Info.plist

cd ../../..

rm -rf Resources
