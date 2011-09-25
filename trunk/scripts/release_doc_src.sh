#! /bin/sh

cp -R ./doc/manual ./doc/pyo_0.5.1-doc
cd doc
tar -cjvf pyo_0.5.1-doc.tar.bz2 pyo_0.5.1-doc
rm -R pyo_0.5.1-doc
cd ..

svn export . pyo_0.5.1-src
tar -cjvf pyo_0.5.1-src.tar.bz2 pyo_0.5.1-src
rm -R pyo_0.5.1-src

