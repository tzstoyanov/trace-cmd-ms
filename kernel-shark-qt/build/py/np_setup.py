#!/usr/bin/python

import sys, getopt
import numpy
from Cython.Distutils import build_ext

def libs(argv):
   usage = 'usage: np_setup.py build_ext -l <library> -d <libdir> -c <incdir>'
   evlibdir = ''
   evincdir = ''
   trlibdir = ''
   trincdir = ''

   try:
      opts, args = getopt.getopt(argv,"hd:c:e:l:", ["help", "trlibdir=", "trincdir=", "evlibdir=", "evincdir="])
   except getopt.GetoptError:
      print usage
      sys.exit(2)
   for opt, arg in opts:
      if opt in ("-h", "--help"):
         print usage
         sys.exit()
      elif opt in ("-d", "--trlibdir"):
         trlibdir = arg
      elif opt in ("-c", "--trincdir"):
         trincdir = arg
      elif opt in ("-e", "--evlibdir"):
         evlibdir = arg
      elif opt in ("-l", "--evincdir"):
         evincdir = arg

   cmd1 = 1
   for i in xrange(len(sys.argv)):
      if sys.argv[i] == 'build_ext':
         cmd1 = i

   sys.argv = sys.argv[:1] + sys.argv[cmd1:]

   return evlibdir, evincdir, trlibdir, trincdir

def configuration(parent_package='', top_path=None, libs=["tracecmd", "traceevent"], libdirs=['.'], incdirs=['.']):
   """ Function used to build our configuration.
   """
   from numpy.distutils.misc_util import Configuration

   print "libdirs : ", libdirs
   # The configuration object that hold information on all the files
   # to be built.
   config = Configuration('', parent_package, top_path)
   config.add_extension('libkshark_wrapper',
                        sources=['libkshark_wrapper.pyx'],
                        libraries=libs,
                        library_dirs=libdirs,
                        depends=['../../src/libkshark.c'],
                        include_dirs=incdirs)
                        #include_dirs=[numpy.get_include(), '../../src/', incdir])
   return config

def main(argv):
   # Retrieve third-party libraries
   evlibdir, evincdir, trlibdir, trincdir = libs(sys.argv[1:])

   # Retrieve the parameters of our local configuration
   params = configuration(top_path='', libdirs=[trlibdir, evlibdir], incdirs=[trincdir, evincdir]).todict()

   #Override the C-extension building so that it knows about '.pyx'
   #Cython files
   params['cmdclass'] = dict(build_ext=build_ext)

   # Call the actual building/packaging function (see distutils docs)
   from numpy.distutils.core import setup
   setup(**params)

if __name__ == "__main__":
   main(sys.argv[1:])
