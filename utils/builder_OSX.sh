mkdir Resources
cp PyoDoc.py Resources/
cp FlatNoteBook.py Resources/
cp *.icns Resources/
svn export ../examples examples/
cp -R examples Resources/
cp -R snippets Resources/
cp -R styles Resources/

rm -rf build dist
py2applet --make-setup E-Pyo.py Resources/*
python setup.py py2app --plist=info.plist
rm -f setup.py
rm -rf build
mv dist E-Pyo_OSX

if cd E-Pyo_OSX;
then
    find . -name .svn -depth -exec rm -rf {} \
    find . -name *.pyc -depth -exec rm -f {} \
    find . -name .* -depth -exec rm -f {} \;
else
    echo "Something wrong. E-Pyo_OSX not created"
    exit;
fi

ditto --rsrc --arch i386 E-Pyo.app E-Pyo-i386.app
rm -rf E-Pyo.app
mv E-Pyo-i386.app E-Pyo.app

cd ..
cp -R E-Pyo_OSX/E-Pyo.app .
#tar -cjvf E-Pyo_OSX-0.6.1.tar.bz2 E-Pyo.app
rm -rf E-Pyo_OSX
#rm -rf E-Pyo.app
rm -rf Resources
rm -rf examples
