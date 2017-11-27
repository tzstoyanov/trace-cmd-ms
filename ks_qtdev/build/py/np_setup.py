#!/usr/bin/python

import sys, getopt
import numpy
from Cython.Distutils import build_ext

def libs(argv):
   usage = 'usage: np_setup.py build_ext -l <library> -d <libdir>'
   library = 'tracecmd'
   libdir = ''

   try:
      opts, args = getopt.getopt(argv,"hd:l:", ["help", "library=", "libdir="])
   except getopt.GetoptError:
      print usage
      sys.exit(2)
   for opt, arg in opts:
      if opt in ("-h", "--help"):
         print usage
         sys.exit()
      elif opt in ("-d", "--libdir"):
         libdir = arg
      elif opt in ("-l", "--library"):
         library = arg

   cmd1 = 1
   for i in xrange(len(sys.argv)):
      if sys.argv[i] == 'build_ext':
         cmd1 = i

   sys.argv = sys.argv[:1] + sys.argv[cmd1:]

   return libdir, library

def configuration(parent_package='', top_path=None, library="tracecmd" ,libdir='.'):
   """ Function used to build our configuration.
   """
   from numpy.distutils.misc_util import Configuration

   # The configuration object that hold information on all the files
   # to be built.
   config = Configuration('', parent_package, top_path)
   config.add_extension('libkshark_wrapper',
                        sources=['libkshark_wrapper.pyx'],
                        libraries=[library],
                        library_dirs=[libdir],
                        depends=['../../src/libkshark.c'],
                        include_dirs=[numpy.get_include(),'../../src/', libdir])
   return config

def main(argv):
   # Retrieve third-party libraries
   libdir, lib = libs(sys.argv[1:])

   # Retrieve the parameters of our local configuration
   params = configuration(top_path='', library=lib, libdir=libdir).todict()

   #Override the C-extension building so that it knows about '.pyx'
   #Cython files
   params['cmdclass'] = dict(build_ext=build_ext)

   # Call the actual building/packaging function (see distutils docs)
   from numpy.distutils.core import setup
   setup(**params)

if __name__ == "__main__":
   main(sys.argv[1:])
