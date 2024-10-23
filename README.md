# TODO

## repo manager:
* // TODO(xieyz) complete all repo api                                                                p1
* // TODO(xieyz) repo data dump strategy and performance                                              p2

## user manager
* // TODO(xieyz) admin user can delete other user and repo, but not permission check other's file     p3

## scan manager
* // TODO(xieyz) scan and sync add fullly unit_test                                                   p0
* // TODO(xieyz) dump to disk only file count over 10 thousand when scan
* // TODO(xieyz) cache store structure test, 1: use multiple cache bucket, 2: sqlite, 3: design       p2
* // TODO(xieyz) complete sync semantic                                                               p4
* // TODO(xieyz) keep user and group and permission when sync                                         p3

## sync manager
* // TODO(xieyz) don't use MurmurHash64A to split thread files when sync                              p3

## other improvement 
* // TODO(xieyz) increment backup, keep as fast as possible with least used disk                      p5
* // TODO(xieyz) log client failed part and retry                                                     p5

# final TODO
* kill syncthing
* kill immich
* kill everything
* password manager
* notes generation
* a web system

# for develop

```
sudo apt install gcc g++
sudo wget https://github.com/bazelbuild/bazel/releases/download/7.2.0/bazel-7.2.0-linux-x86_64 -O /usr/local/bin
git clone git@github.com:oceandoc/oceanfile.git
cd oceanfile
bazel build //...                                                   # build all target
bazel run //:refresh_compile_commands                               # generate compile_commands.json

./bazel-bin/src/server/server                                       # start server
./bazel-bin/src/client/grpc_client/grpc_file_client_test            # test repo api

```
