#! /bin/bash

#
# 1. change version number
# cd doc-sphinx; python build.py
# 3. Execute from pyo folder : ./scripts/release_doc_src.sh
#

version=0.7.5
replace=XXX

doc_rep=pyo_XXX-doc
doc_tar=pyo_XXX-doc.tar.bz2

src_rep=pyo_XXX-src
src_tar=pyo_XXX-src.tar.bz2

cp -R ./doc-sphinx/build_html ./doc-sphinx/${doc_rep/$replace/$version}
cd doc-sphinx
tar -cjvf ${doc_tar/$replace/$version} ${doc_rep/$replace/$version}
rm -R ${doc_rep/$replace/$version}
cd ..

svn export . ${src_rep/$replace/$version}
tar -cjvf ${src_tar/$replace/$version} ${src_rep/$replace/$version}
rm -R ${src_rep/$replace/$version}
