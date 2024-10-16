# TODO
* recursive scan and send
* implement sync semantic
* implement backup as fast as possible
* repo ctl interface
* log client failed part and retry use queue
* authentic, sqlite
* session cookie
* public and private ip address, mac address, system info
* fix self defined toolchain compitible problem with refresh_compile_commands

// TODO(xieyz) add full unit_test
// TODO(xieyz) dump to disk only file or dir change count over 10 thousand
// TODO(xieyz) use multiple cache bucket
// TODO(xieyz) keep user and group
// TODO(xieyz) sync don't use MurmurHash64A to split thread files;
// TODO(xieyz) fix scan remove files num

# final TODO
* kill syncthing
* kill immich
* kill everything
* password manager
* notes generation
* a web system

bazel run @hedron_compile_commands//:refresh_all
bazel run //:refresh_compile_commands
