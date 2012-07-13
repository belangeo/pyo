#! /bin/sh

cp -R ./doc/manual ./doc/pyo_0.6.2-doc
cd doc
tar -cjvf pyo_0.6.2-doc.tar.bz2 pyo_0.6.2-doc
rm -R pyo_0.6.2-doc
cd ..

svn export . pyo_0.6.2-src
tar -cjvf pyo_0.6.2-src.tar.bz2 pyo_0.6.2-src
rm -R pyo_0.6.2-src
