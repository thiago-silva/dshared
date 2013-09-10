from distutils.core import setup, Extension

module = Extension('dshared',
                   libraries=["rt"],
                   sources = ['src/dsharedmodule.cpp',
                              'src/dshared.cpp'])

setup (name = 'DShared',
       version = '1.0',
       description = 'dicts in shared memory',
       ext_modules = [module])
