
from setuptools import setup, Extension, find_packages

m1 = Extension(
    'mrhttp.internals',
     sources = [ 
      './src/mrhttp/internals/module.c',
      './src/mrhttp/internals/app.c',
      './src/mrhttp/internals/protocol.c',
      './src/mrhttp/internals/mrqprotocol.c',
      './src/mrhttp/internals/mrqclient.c',
      './src/mrhttp/internals/memcachedclient.c',
      './src/mrhttp/internals/memprotocol.c',
      './src/mrhttp/internals/parser.c',
      './src/mrhttp/internals/request.c',
      './src/mrhttp/internals/response.c',
      './src/mrhttp/internals/router.c',
      './src/mrhttp/internals/mrhttpparser.c',
      #'./src/mrhttp/cpp/cpptest.cpp'
      './src/mrhttp/internals/hash/city.c',
      './src/mrhttp/internals/hash/assoc.c',
      './src/mrhttp/internals/utils.c',
     ],
     include_dirs = ['./src/mrhttp/internals'],
     extra_compile_args = ['-msse4.2', '-mavx2', '-mbmi2', '-Wunused-variable','-std=c99','-Wno-discarded-qualifiers', '-Wno-unused-variable','-Wno-unused-function'],
     extra_link_args = [],
     #extra_link_args = ['-lasan'],
     define_macros = [('DEBUG_PRINT',1)]
)

setup(
  name="mrhttp", 
  version="0.2",
  license='MIT',
  description='A python web framework written in C',
  ext_modules = [m1],
  package_dir={'':'src'},
  packages=find_packages('src'),# + ['prof'],
  #package_data={'prof': ['prof.so']},
  install_requires=[
    #'uvloop<0.9.0',
    'uvloop>0.9.0',
  ],
  platforms='x86_64 Linux and MacOS X',
  url='http://github.com/MarkReedZ/mrhttp/',
  author='Mark Reed',
  author_email='markreed99@gmail.com',
  keywords=['web', 'asyncio'],
  classifiers=[
    'Development Status :: 2 - Pre-Alpha',
    'Intended Audience :: Developers',
    'Environment :: Web Environment',
    'License :: OSI Approved :: MIT License',
    'Operating System :: MacOS :: MacOS X',
    'Operating System :: POSIX :: Linux',
    'Programming Language :: C',
    'Programming Language :: Python :: 3.5',
    'Programming Language :: Python :: 3.6',
    'Programming Language :: Python :: Implementation :: CPython',
    'Topic :: Internet :: WWW/HTTP'
   ]
)

