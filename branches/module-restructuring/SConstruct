import sys

def get_platforms_to_build():
    platforms = []
    if sys.platform == "darwin":
        platforms.append( "osx" )
    return platforms

for platform in get_platforms_to_build():
    SConscript( ".".join(["SConstruct", platform]) )
