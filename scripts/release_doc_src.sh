#! /bin/bash

#
# 1. change version number
# 2. cd doc; python build.py
# 3. Execute from pyo folder : ./scripts/release_doc_src.sh
#

version=1.1.0
replace=XXX

doc_rep=pyo_XXX-doc
doc_tar=pyo_XXX-doc.tar.bz2

src_rep=pyo_XXX-src
src_tar=pyo_XXX-src.tar.bz2

cp -R ./docs ./dist/${doc_rep/$replace/$version}
cd dist
tar -cjvf ${doc_tar/$replace/$version} ${doc_rep/$replace/$version}
rm -R ${doc_rep/$replace/$version}
cd ..

git checkout-index -a -f --prefix=${src_rep/$replace/$version}/
tar -cjvf ${src_tar/$replace/$version} ${src_rep/$replace/$version}
rm -R ${src_rep/$replace/$version}
