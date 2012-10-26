echo off

echo *** Build installer for python2.6 ***
Compil32 /cc "win_installer_py26.iss"

echo *** Build installer for python2.7 ***
Compil32 /cc "win_installer_py27.iss"
