echo off

echo *** Build installer for python2.7 ***
Compil32 /cc "win_installer_py27.iss"

echo *** Build installer for python3.5 ***
Compil32 /cc "win_installer_py35.iss"
