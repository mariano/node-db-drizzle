# node-drizzle: Drizzle/MySQL bindings for Node.js #

## LICENSE ##

node-drizzle is released under the [MIT License] [license].

## INSTALLATION ##

### Install libdrizzle ###

#### Install Bazaar ####

In *Debian* based systems:

    $ sudo aptitude install bzr

In *Arch*:

    $ sudo pacman -S bzr

#### Download and build ####

##### Arch Linux: Fix reference to Python #####

If you are on *Arch* you know that the default python binary points to
Python 3. Unfortunately, Drizzle's pre_hook.sh and pandora-plugin files need 
python 2. So change the references to python:

    $ sed -i 's/python/python2/g' config/pre_hook.sh
    $ sed -i 's/python/python2/g' config/pandora-plugin

#### Download and build libdrizzle ####

    $ bzr branch lp:drizzle
    $ cd drizzle/
    $ ./config/autorun.sh
    $ ./configure --without-server --prefix=/usr
    $ make libdrizzle
    $ sudo make install-libdrizzle

[license]: http://www.opensource.org/licenses/mit-license.php
