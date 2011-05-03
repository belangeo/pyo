#! /bin/sh

cp -R ./doc/manual ./doc/pyo_0.4.0-doc
cd doc
tar -cjvf pyo_0.4.0-doc.tar.bz2 pyo_0.4.0-doc
rm -R pyo_0.4.0-doc
cd ..

svn export . pyo_0.4.0-src
tar -cjvf pyo_0.4.0-src.tar.bz2 pyo_0.4.0-src
rm -R pyo_0.4.0-src

