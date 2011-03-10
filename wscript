####
# Copyright by Mariano Iglesias
# See contributors list in README
#
# See license text in LICENSE file
####

import Options, Utils
from os import unlink, symlink, chdir
from os.path import exists

srcdir = "."
blddir = "build"
VERSION = "1.2.3"

def set_options(opt):
  opt.tool_options("compiler_cxx")
  opt.add_option('--nodeunit', action='store_true', help='Run tests with nodeunit')
  opt.add_option('--debug', action='store_true', help='Run tests with node_g')
  opt.add_option('--warn', action='store_true', help='Enable extra -W* compiler flags')

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  
  # Enables all the warnings that are easy to avoid
  conf.env.append_unique('CXXFLAGS', ["-Wall"])
  if Options.options.warn:
    # Extra warnings
    conf.env.append_unique('CXXFLAGS', ["-Wextra"])
    # Extra warnings, gcc 4.4
    conf.env.append_unique('CXXFLAGS', ["-Wconversion", "-Wshadow", "-Wsign-conversion", "-Wunreachable-code", "-Wredundant-decls", "-Wcast-qual"])
  
  if not conf.check_cxx(lib='drizzle'):
    conf.fatal("Missing libdrizzle from drizzle package")
 
  if not conf.check_cxx(header_name='libdrizzle/drizzle.h'):
    conf.fatal("Missing drizzle.h header from drizzle package")

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.target = "drizzle_bindings"
  obj.source = "./src/drizzle/connection.cc src/drizzle/exception.cc src/drizzle/result.cc ./src/drizzle.cc ./src/drizzle_bindings.cc"
  obj.uselib = "DRIZZLE"

def test(tst):
  test_binary = 'node'
  if Options.options.nodeunit:
    test_binary = 'nodeunit'
  elif Options.options.debug:
    test_binary = 'nodeunit_g'
  
  Utils.exec_command(test_binary + ' tests.js')

def lint(lnt):
  # Bindings C++ source code
  print("Run CPPLint:")
  Utils.exec_command('cpplint --filter=-whitespace/line_length ./src/drizzle/*.h ./src/drizzle/*.cc ./src/*.h ./src/*.cc')
  # Bindings javascript code, docs and tools
  print("Run Nodelint for sources:")
  Utils.exec_command('nodelint ./package.json ./drizzle.js')

def doc(doc):
  description = ('--desc "Drizzle/MySQL bindings for [Node.js](http://nodejs.org) using libdrizzle.\n\n' +
                 'Check out the [Github repo](http://github.com/mariano/node-drizzle) for the source and installation guide.\n\n' +
                 'Extra information: ')
  ribbon = '--ribbon "http://github.com/mariano/node-drizzle" '
  
  downloads = ('--desc-right "' +
               'Latest version ' + VERSION + ':<br/>\n' +
               '<a href="http://github.com/mariano/node-drizzle/zipball/v' + VERSION + '">\n' +
               ' <img width="90" src="http://github.com/images/modules/download/zip.png" />\n' +
               '</a>\n' +
               '<a href="http://github.com/mariano/node-drizzle/tarball/v' + VERSION + '">\n' +
               ' <img width="90" src="http://github.com/images/modules/download/tar.png" />\n' +
               '</a>" ')
  
  print("Parse README.markdown:")
  Utils.exec_command('dox --title "Node-drizzle" ' +
                     description +
                     '[ChangeLog](./changelog.html), [API](./api.html), [Examples](./examples.html), [Wiki](http://github.com/mariano/node-drizzle/wiki)." ' +
                     ribbon +
                     downloads +
                     './README.markdown ' +
                     '> ./doc/index.html')
  print("Parse CHANGELOG.markdown:")
  Utils.exec_command('dox --title "Node-drizzle changelog" ' +
                     description +
                     '[Homepage](./index.html), [API](./api.html), [Examples](./examples.html), [Wiki](http://github.com/mariano/node-drizzle/wiki)." ' +
                     ribbon +
                     './CHANGELOG.markdown ' +
                     '> ./doc/changelog.html')
  print("Parse API documentation:")
  Utils.exec_command('dox --title "Node-drizzle API" ' +
                     description +
                     '[Homepage](./index.html), [ChangeLog](./changelog.html), [Examples](./examples.html), [Wiki](http://github.com/mariano/node-drizzle/wiki)." ' +
                     ribbon +
                     './src/drizzle/connection.h ' +
                     './src/drizzle/connection.cc ' +
                     './src/drizzle/exception.h ' +
                     './src/drizzle/exception.cc ' +
                     './src/drizzle/result.h ' +
                     './src/drizzle/result.cc ' +
                     './src/drizzle.h ' +
                     './src/drizzle.cc ' +
                     './src/drizzle_bindings.h ' +
                     './src/drizzle_bindings.cc ' +
                     './drizzle.js ' +
                     '> ./doc/api.html')

def gh_pages(context):
  Utils.exec_command('./gh_pages.sh')
