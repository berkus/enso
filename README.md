# README #

## Requirements ##

The following packages are required for Enso.  Please download and install the packages for your platform at their respective sites:

  * Python 2.5 (http://www.python.org/download/)
  * SCons (http://www.scons.org)
  * Any Subversion client (a list [here](http://subversion.tigris.org/links.html#clients) or [here](http://en.wikipedia.org/wiki/Comparison_of_Subversion_clients))

Please refer to the README for your platform for further system-specific requirements:

  * [Linux README](README_linux.md)
  * [OSX README](README_osx.md)
  * [Windows README](README_win.md)

## Downloading ##

Once you have a subversion client, you can checkout the latest project source code using the following url:
```
http://enso.googlecode.com/svn/trunk/
```

Please consult with your Subversion client's documentation as to how to checkout a project.  Alternatively, those with a command line can use this:
```
#Where enso-read-only is the directory to save the code to
svn checkout http://enso.googlecode.com/svn/trunk/ enso-read-only
```

## Installing Enso ##

Enso has a distutils-based install script that can be used to install Enso system-wide, but it currently only works on Linux.  Installation is not necessary for running Enso.

To install, just run

```
python setup.py install
```

## Running Enso ##

To run Enso, just excecute the following from the root directory of
the source tree:

```
python scripts/run_enso.py
```