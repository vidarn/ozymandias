#!/usr/bin/env python

from distutils.core import setup, Extension


ozymandias_module = Extension('_ozymandias',
        sources=['ozymandias_wrap.cxx'],
        libraries = ['ozymandias'],
        extra_compile_args = ['-std=c++11','-Wno-maybe-uninitialized'],)

setup (name = 'ozymandias',
       version = '0.1',
       author      = "Vidar Nelson",
       description = """Path tracer""",
       ext_modules = [ozymandias_module],
       py_modules = ["ozymandias"],
       )
