# node-db-drizzle: Drizzle/MySQL bindings for Node.js #

## LICENSE ##

node-db-drizzle is released under the [MIT License] [license].

## INSTALLATION ##

### Install libdrizzle ###

#### Install Bazaar ####

In *Debian* based systems:

    $ sudo aptitude install bzr

In *Arch*:

    $ sudo pacman -S bzr

#### Download and build ####

If you are on *Arch Linux* you know that the default python binary points to
Python 3. Unfortunately, Drizzle's pre_hook.sh and pandora-plugin files need 
python 2. So change the references to python:

    $ sed -i 's/python/python2/g' config/pre_hook.sh
    $ sed -i 's/python/python2/g' config/pandora-plugin

To download and build:

    $ bzr branch lp:drizzle
    $ cd drizzle/
    $ ./config/autorun.sh
    $ ./configure --without-server --prefix=/usr
    $ make libdrizzle
    $ sudo make install-libdrizzle

### Install node-db-drizzle ###

#### Using npm ####

    $ npm install db-drizzle

#### Using GIT ####

    $ git clone https://github.com/mariano/node-db-drizzle.git
    $ cd node-db-drizzle
    $ node-waf configure build

[license]: http://www.opensource.org/licenses/mit-license.php
