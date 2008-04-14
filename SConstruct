import sys

def get_platforms_to_build():
    platforms = []
    if sys.platform == "darwin":
        platforms.append( "osx" )
    if sys.platform == "win32":
        platforms.append( "win32" )
    return platforms

for platform in get_platforms_to_build():
    SConscript( ".".join(["SConstruct", platform]) )
